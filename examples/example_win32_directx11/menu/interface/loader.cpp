#include "loader.h"

#include "net.h"
#include "../helpers/json.hpp"
#include "elements_manager.h"
#include "menu_i.h"
#include "../../resource.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace loader
{
    namespace
    {
        // Must match RESPONSE_SECRET in fragment's source/auth/key_auth.cpp and on
        // the worker: responses signed with anything else are rejected.
        constexpr const char* k_response_secret = "c41a2f7305a38e37656778bdb8472aa12804323184d79774681a48c448bdcbd1";

        // Emitted by fragment.exe once it is attached and the overlay is live.
        constexpr const char* k_ready_marker = "fragment ready";

        constexpr std::int64_t k_session_refresh_margin_seconds = 60;

        struct shared_state
        {
            std::mutex mutex;
            std::string error;
            std::string plan;
            std::int64_t expires_at = 0;
            std::atomic<bool> signed_in{ false };
            std::atomic<bool> auth_busy{ false };
            std::atomic<bool> restoring{ true };
            std::atomic<bool> load_active{ false };
            std::atomic<bool> load_finished{ false };
            std::atomic<bool> load_ok{ false };
            std::atomic<bool> close_when_done{ false };
            std::string load_stage;
            std::chrono::steady_clock::time_point close_at{};
            bool close_scheduled = false;
        };

        shared_state g;
        std::string g_session;

        std::wstring loader_directory()
        {
            wchar_t path[MAX_PATH]{};
            if (::GetModuleFileNameW(nullptr, path, MAX_PATH) == 0)
            {
                return {};
            }
            return std::filesystem::path(path).parent_path().wstring();
        }

        // The overlay lives with the rest of fragment's data, not beside the
        // loader, so the loader itself stays a single self-contained file.
        std::filesystem::path fragment_root()
        {
            wchar_t base[MAX_PATH]{};
            const DWORD length = ::GetEnvironmentVariableW(L"APPDATA", base, MAX_PATH);
            const std::filesystem::path root = (length > 0) ? std::filesystem::path(base) : std::filesystem::temp_directory_path();
            return root / L"fragment";
        }

        std::filesystem::path fragment_exe_path()
        {
            return fragment_root() / L"fragment.exe";
        }

        // Writes the payload embedded in this exe to disk when it is missing or
        // does not match what we were built with.
        bool extract_embedded_payload()
        {
            const HMODULE module = ::GetModuleHandleW(nullptr);
            const HRSRC resource = ::FindResourceW(module, MAKEINTRESOURCEW(IDR_PAYLOAD), RT_RCDATA);
            if (!resource)
            {
                return false;
            }
            const DWORD size = ::SizeofResource(module, resource);
            const HGLOBAL loaded = ::LoadResource(module, resource);
            const void* bytes = loaded ? ::LockResource(loaded) : nullptr;
            if (!bytes || size == 0)
            {
                return false;
            }

            const std::string embedded(static_cast<const char*>(bytes), size);
            const auto target = fragment_exe_path();
            std::error_code ec;
            if (std::filesystem::exists(target, ec) && std::filesystem::file_size(target, ec) == size && !ec)
            {
                // Same size is not proof, but hashing 12 MB on every launch is
                // wasted work; the update check below still compares hashes.
                return true;
            }

            std::filesystem::create_directories(target.parent_path(), ec);
            std::ofstream output(target, std::ios::binary | std::ios::trunc);
            if (!output.good())
            {
                return false;
            }
            output.write(embedded.data(), static_cast<std::streamsize>(embedded.size()));
            output.close();
            return output.good();
        }

        std::filesystem::path session_path()
        {
            return fragment_root() / L"loader_session.dat";
        }

        // XOR with a key derived from the machine guid; the same idea fragment
        // uses for its own session file. The server-side HWID binding is the real lock.
        std::string session_mask()
        {
            const std::string guid = net::machine_guid();
            std::string key;
            key.reserve(16);
            for (int round = 0; round < 16; ++round)
            {
                std::uint64_t h = 14695981039346656037ull;
                const std::string seed = guid + "|" + std::to_string(round);
                for (const char ch : seed)
                {
                    h ^= static_cast<std::uint8_t>(ch);
                    h *= 1099511628211ull;
                }
                key.push_back(static_cast<char>((h >> 32) & 0xFF));
            }
            return key;
        }

        std::string apply_mask(std::string data)
        {
            const std::string key = session_mask();
            if (key.empty())
            {
                return data;
            }
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                data[i] ^= key[i % key.size()];
            }
            return data;
        }

        void store_session(const std::string& token)
        {
            g_session = token;
            if (token.empty())
            {
                std::error_code ec;
                std::filesystem::remove(session_path(), ec);
                return;
            }
            std::error_code ec;
            std::filesystem::create_directories(session_path().parent_path(), ec);
            std::ofstream output(session_path(), std::ios::binary | std::ios::trunc);
            if (output.good())
            {
                const std::string masked = apply_mask(token);
                output.write(masked.data(), static_cast<std::streamsize>(masked.size()));
            }
        }

        std::string load_session()
        {
            std::ifstream input(session_path(), std::ios::binary);
            if (!input.good())
            {
                return {};
            }
            std::string masked((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            std::string token = apply_mask(masked);
            while (!token.empty() && (token.back() == '\n' || token.back() == '\r' || token.back() == ' '))
            {
                token.pop_back();
            }
            return token.size() >= 16 ? token : std::string{};
        }

        struct verify_result
        {
            bool ok = false;
            bool definitive = false;   // server denied: the stored session is dead
            std::string error;
            std::string status;
            std::string plan;
            std::int64_t expires_at = 0;
            std::string session;
        };

        std::string json_string(const nlohmann::json& node, const char* key)
        {
            const auto it = node.find(key);
            return (it != node.end() && it->is_string()) ? it->get<std::string>() : std::string{};
        }

        verify_result verify(const std::string& mode, const std::string& value, const std::string& hwid, const std::string& nonce)
        {
            verify_result out;

            nlohmann::json request{
                { "mode", mode },
                { "hwid", hwid },
                { "nonce", nonce },
            };
            request[mode == "key" ? "key" : "session"] = value;

            const auto reply = net::post_json(net::api_base() + L"/api/verify", request.dump());
            if (!reply.transport_ok)
            {
                out.error = "cannot reach the auth server";
                return out;
            }

            const nlohmann::json response = nlohmann::json::parse(reply.body, nullptr, false);
            if (response.is_discarded() || !response.is_object())
            {
                out.error = "invalid server response";
                return out;
            }

            out.status = json_string(response, "status");
            out.plan = json_string(response, "plan");
            out.session = json_string(response, "session");
            out.expires_at = response.contains("expires_at") && response["expires_at"].is_number()
                ? response["expires_at"].get<std::int64_t>()
                : 0;

            const std::int64_t ts = response.contains("ts") && response["ts"].is_number() ? response["ts"].get<std::int64_t>() : 0;
            const std::string echoed = json_string(response, "nonce");
            const std::string signature = json_string(response, "sig");

            if (out.status.empty() || signature.empty())
            {
                out.error = "invalid server response";
                return out;
            }

            const std::string canonical = std::to_string(ts) + "|" + echoed + "|" + out.status + "|" + out.plan + "|" + std::to_string(out.expires_at) + "|" + hwid;
            if (net::hmac_sha256_hex(k_response_secret, canonical) != signature)
            {
                out.error = "response signature mismatch";
                return out;
            }
            if (echoed != nonce)
            {
                out.error = "stale response rejected";
                return out;
            }
            const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
            if (ts != 0 && (now > ts ? now - ts : ts - now) > 120)
            {
                out.error = "system clock is out of sync";
                return out;
            }

            if (out.status != "ok")
            {
                // Denials that mean the stored session can never work again.
                out.definitive = out.status == "bad_key" || out.status == "banned" || out.status == "expired"
                    || out.status == "hwid_mismatch" || out.status == "session_invalid" || out.status == "bad_format";

                if (out.status == "bad_format") out.error = "invalid key format";
                else if (out.status == "bad_key") out.error = "key not found";
                else if (out.status == "expired") out.error = "this key has expired";
                else if (out.status == "banned") out.error = "this key has been banned";
                else if (out.status == "hwid_mismatch") out.error = "key is bound to another machine";
                else if (out.status == "locked") out.error = "too many attempts - try again shortly";
                else if (out.status == "rate_limited") out.error = "rate limited - try again shortly";
                else if (out.status == "session_invalid") out.error = "saved session expired";
                else out.error = "verification failed (" + out.status + ")";
                return out;
            }

            out.ok = true;
            return out;
        }

        void set_error(const std::string& text)
        {
            std::lock_guard lock(g.mutex);
            g.error = text;
        }

        void set_stage(const std::string& text)
        {
            std::lock_guard lock(g.mutex);
            g.load_stage = text;
        }

        bool process_running(const wchar_t* name)
        {
            HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
            {
                return false;
            }
            PROCESSENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            bool found = false;
            for (BOOL ok = ::Process32FirstW(snapshot, &entry); ok; ok = ::Process32NextW(snapshot, &entry))
            {
                if (_wcsicmp(entry.szExeFile, name) == 0)
                {
                    found = true;
                    break;
                }
            }
            ::CloseHandle(snapshot);
            return found;
        }

        // Brings fragment.exe to the manifest's build. Comparing hashes means the
        // loader never needs a version resource in the target.
        bool ensure_fragment_updated()
        {
            const auto manifest_reply = net::post_json(net::api_base() + L"/api/latest", "{}");
            if (!manifest_reply.transport_ok)
            {
                return true; // offline: carry on with whatever is on disk
            }

            const nlohmann::json manifest = nlohmann::json::parse(manifest_reply.body, nullptr, false);
            if (manifest.is_discarded() || !manifest.is_object())
            {
                return true;
            }
            const std::string wanted = json_string(manifest, "sha256");
            if (wanted.empty())
            {
                return true;
            }

            const auto target = fragment_exe_path();
            std::error_code ec;
            if (std::filesystem::exists(target, ec))
            {
                const std::string local = net::sha256_hex_file(target.wstring());
                if (!local.empty() && local == wanted)
                {
                    return true;
                }
            }

            set_stage("Downloading update...");
            nlohmann::json body{ { "token", g_session } };
            const auto download = net::post_json(net::api_base() + L"/api/download", body.dump());
            if (!download.transport_ok || download.status != 200 || download.body.empty())
            {
                set_error("update download failed");
                return false;
            }
            if (net::sha256_hex(download.body) != wanted)
            {
                set_error("downloaded update failed its checksum");
                return false;
            }

            const auto temporary = target.wstring() + L".new";
            {
                std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
                if (!output.good())
                {
                    set_error("could not write the update");
                    return false;
                }
                output.write(download.body.data(), static_cast<std::streamsize>(download.body.size()));
            }
            std::filesystem::remove(target, ec);
            std::filesystem::rename(temporary, target, ec);
            if (ec)
            {
                set_error("could not replace fragment.exe");
                return false;
            }
            return true;
        }

        // Launches fragment.exe. Silent mode hides the console and watches stdout
        // for the ready marker; debug mode shows the console and detaches.
        bool launch_fragment(bool debug)
        {
            const auto target = fragment_exe_path();
            if (!std::filesystem::exists(target))
            {
                set_error("fragment.exe is missing next to the loader");
                return false;
            }

            std::wstring command = L"\"" + target.wstring() + L"\"";
            if (!g_session.empty())
            {
                command += L" --session=" + std::wstring(g_session.begin(), g_session.end());
            }
            command += L" --no-update";

            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION process{};

            if (debug)
            {
                startup.dwFlags = STARTF_USESHOWWINDOW;
                startup.wShowWindow = SW_SHOWNORMAL;
                if (!::CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, fragment_root().c_str(), &startup, &process))
                {
                    set_error("could not start fragment.exe");
                    return false;
                }
            }
            else
            {
                SECURITY_ATTRIBUTES attributes{};
                attributes.nLength = sizeof(attributes);
                attributes.bInheritHandle = TRUE;

                HANDLE read_pipe = nullptr;
                HANDLE write_pipe = nullptr;
                if (!::CreatePipe(&read_pipe, &write_pipe, &attributes, 0))
                {
                    set_error("could not create the output pipe");
                    return false;
                }
                ::SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

                startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                startup.wShowWindow = SW_HIDE;
                startup.hStdOutput = write_pipe;
                startup.hStdError = write_pipe;
                startup.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);

                const BOOL started = ::CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE,
                    CREATE_NO_WINDOW, nullptr, fragment_root().c_str(), &startup, &process);
                ::CloseHandle(write_pipe);
                if (!started)
                {
                    ::CloseHandle(read_pipe);
                    set_error("could not start fragment.exe");
                    return false;
                }

                {
                    std::lock_guard lock(g.mutex);
                    g.load_stage = "Injecting...";
                }

                // Watch for the ready line with a bounded timeout so a silent
                // failure (bad session, wrong client version) still reports.
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(25);
                std::string collected;
                char buffer[512];
                DWORD available = 0;
                while (std::chrono::steady_clock::now() < deadline)
                {
                    if (!::PeekNamedPipe(read_pipe, nullptr, 0, nullptr, &available, nullptr))
                    {
                        break;
                    }
                    if (available == 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }
                    DWORD read = 0;
                    if (!::ReadFile(read_pipe, buffer, sizeof(buffer) - 1, &read, nullptr) || read == 0)
                    {
                        break;
                    }
                    buffer[read] = '\0';
                    collected.append(buffer, read);
                    if (collected.find(k_ready_marker) != std::string::npos)
                    {
                        ::CloseHandle(read_pipe);
                        ::CloseHandle(process.hThread);
                        ::CloseHandle(process.hProcess);
                        return true;
                    }
                }
                // Marker not seen. A still-running fragment is the outcome we want
                // (older payloads never print it), so only a dead process is a
                // real failure.
                DWORD exit_code = 0;
                const bool alive = ::GetExitCodeProcess(process.hProcess, &exit_code) != 0 && exit_code == STILL_ACTIVE;

                ::CloseHandle(read_pipe);
                ::CloseHandle(process.hThread);
                ::CloseHandle(process.hProcess);

                if (alive)
                {
                    return true;
                }
                set_error("fragment exited before it finished loading - try Load with debugger");
                return false;
            }

            ::CloseHandle(process.hThread);
            ::CloseHandle(process.hProcess);
            return true;
        }

        void run_load(bool debug)
        {
            set_stage("Preparing...");

            if (!roblox_running())
            {
                set_error("Roblox is not running - join a game first");
                g.load_ok.store(false);
                g.load_finished.store(true);
                return;
            }

            if (!ensure_fragment_updated())
            {
                g.load_ok.store(false);
                g.load_finished.store(true);
                return;
            }
            set_stage("Starting fragment...");

            const bool ok = launch_fragment(debug);
            g.load_ok.store(ok);
            g.load_finished.store(true);
            if (ok && !debug)
            {
                std::lock_guard lock(g.mutex);
                g.close_when_done.store(true);
                g.close_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(2500);
            }
        }
    }

    void initialize()
    {
        extract_embedded_payload();

        std::thread([]()
        {
            const std::string hwid = net::machine_hwid();
            const std::string saved = load_session();
            if (hwid.empty() || saved.empty())
            {
                g.restoring.store(false);
                return;
            }
            const auto result = verify("session", saved, hwid, net::random_hex(12));
            if (result.ok)
            {
                if (!result.session.empty())
                {
                    store_session(result.session);
                }
                std::lock_guard lock(g.mutex);
                g.plan = result.plan;
                g.expires_at = result.expires_at;
                g.signed_in.store(true);
            }
            else if (result.definitive)
            {
                // Only a signed denial invalidates the stored session. A network
                // blip or clock skew must not force the user to retype the key.
                store_session({});
            }
            g.restoring.store(false);
        }).detach();
    }

    void poll()
    {
        std::lock_guard lock(g.mutex);
        if (g.close_when_done.load() && std::chrono::steady_clock::now() >= g.close_at)
        {
            g.close_when_done.store(false);
            ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    }

    void sign_in_async(const std::string& key)
    {
        if (g.auth_busy.exchange(true))
        {
            return;
        }
        std::thread([key]()
        {
            std::string hwid = net::machine_hwid();
            if (hwid.empty())
            {
                set_error("could not build a hardware id");
                g.auth_busy.store(false);
                return;
            }
            const auto result = verify("key", key, hwid, net::random_hex(12));
            if (result.ok)
            {
                store_session(result.session);
                std::lock_guard lock(g.mutex);
                g.error.clear();
                g.plan = result.plan;
                g.expires_at = result.expires_at;
                g.signed_in.store(true);
            }
            else
            {
                set_error(result.error);
            }
            g.auth_busy.store(false);
        }).detach();
    }

    bool sign_in_busy() { return g.auth_busy.load(); }
    bool restoring_session() { return g.restoring.load(); }
    bool signed_in() { return g.signed_in.load(); }

    std::string auth_error()
    {
        std::lock_guard lock(g.mutex);
        return g.error;
    }

    std::string plan_text()
    {
        std::lock_guard lock(g.mutex);
        if (g.plan == "1d") return "1 day";
        if (g.plan == "7d") return "7 days";
        if (g.plan == "30d") return "30 days";
        if (g.plan == "lifetime") return "lifetime";
        return g.plan.empty() ? "active" : g.plan;
    }

    std::string remaining_text()
    {
        std::lock_guard lock(g.mutex);
        if (g.plan == "lifetime" || g.expires_at == 0)
        {
            return "lifetime";
        }
        const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
        const std::int64_t left = g.expires_at - now;
        if (left <= 0)
        {
            return "expired";
        }
        const std::int64_t days = left / 86400;
        const std::int64_t hours = (left % 86400) / 3600;
        const std::int64_t minutes = (left % 3600) / 60;
        if (days > 0) return std::to_string(days) + "d " + std::to_string(hours) + "h";
        if (hours > 0) return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
        return std::to_string(minutes) + "m";
    }

    std::string last_updated_text()
    {
        std::error_code ec;
        const auto target = fragment_exe_path();
        if (!std::filesystem::exists(target, ec))
        {
            return "not installed";
        }
        const auto stamp = std::filesystem::last_write_time(target, ec);
        if (ec)
        {
            return "unknown";
        }
        const auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            stamp - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        const std::time_t as_time = std::chrono::system_clock::to_time_t(system_time);
        std::tm local{};
        if (::localtime_s(&local, &as_time) != 0)
        {
            return "unknown";
        }
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", local.tm_mday, local.tm_mon + 1, local.tm_year + 1900);
        return buffer;
    }

    std::string fragment_version()
    {
        // The manifest is the only place the release version is published.
        const auto reply = net::post_json(net::api_base() + L"/api/latest", "{}");
        if (!reply.transport_ok)
        {
            return {};
        }
        const nlohmann::json manifest = nlohmann::json::parse(reply.body, nullptr, false);
        return manifest.is_discarded() ? std::string{} : json_string(manifest, "version");
    }

    std::string hwid_text()
    {
        const std::string hwid = net::machine_hwid();
        return hwid.size() >= 16 ? hwid.substr(0, 16) + "..." : hwid;
    }

    bool roblox_running()
    {
        return process_running(L"RobloxPlayerBeta.exe");
    }

    bool fragment_present()
    {
        std::error_code ec;
        return std::filesystem::exists(fragment_exe_path(), ec);
    }

    void begin(int game_index, bool debug)
    {
        if (g.load_active.exchange(true))
        {
            return;
        }
        if (!signed_in())
        {
            set_error("sign in first");
            MGR->status = E_ERROR;
            MGR->notify_desc = "Sign in with your licence key first.";
            g.load_active.store(false);
            return;
        }
        g.load_finished.store(false);
        g.load_ok.store(false);
        {
            std::lock_guard lock(g.mutex);
            g.error.clear();
            g.close_when_done.store(false);
            g.close_scheduled = false;
        }
        MGR->status = E_LOADING;
        MGR->loading_module = "Preparing...";
        std::thread([debug]() { run_load(debug); }).detach();
    }

    void tick(int& game_index)
    {
        if (!g.load_active.load())
        {
            return;
        }

        if (!g.load_finished.load())
        {
            MGR->status = E_LOADING;
            std::lock_guard lock(g.mutex);
            if (!g.load_stage.empty())
            {
                MGR->loading_module = g.load_stage;
            }
            return;
        }

        if (g.load_ok.load())
        {
            MGR->status = E_SUCCESS;
            MGR->notify_desc = "Took a moment to load.";
            MGR->loading_module = "Ready.";
        }
        else
        {
            MGR->status = E_ERROR;
            std::lock_guard lock(g.mutex);
            MGR->notify_desc = g.error.empty() ? "Load failed." : g.error;
        }
        g.load_active.store(false);
    }
}
