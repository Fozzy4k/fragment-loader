#include "net.h"

#include <Windows.h>
#include <winhttp.h>
#include <bcrypt.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")

namespace net
{
    namespace
    {
        std::string to_hex(const std::uint8_t* data, std::size_t size)
        {
            static constexpr char digits[] = "0123456789abcdef";
            std::string out;
            out.reserve(size * 2);
            for (std::size_t i = 0; i < size; ++i)
            {
                out.push_back(digits[data[i] >> 4]);
                out.push_back(digits[data[i] & 0x0F]);
            }
            return out;
        }

        response request(const std::wstring& url, const char* verb, const std::string* body)
        {
            response result;

            URL_COMPONENTS parts{};
            parts.dwStructSize = sizeof(parts);
            std::wstring host(256, L'\0');
            std::wstring path(2048, L'\0');
            parts.lpszHostName = host.data();
            parts.dwHostNameLength = static_cast<DWORD>(host.size());
            parts.lpszUrlPath = path.data();
            parts.dwUrlPathLength = static_cast<DWORD>(path.size());

            if (!::WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts))
            {
                return result;
            }
            host.resize(parts.dwHostNameLength);
            path.resize(parts.dwUrlPathLength);

            HINTERNET session = ::WinHttpOpen(L"fragment-loader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!session)
            {
                return result;
            }
            ::WinHttpSetTimeouts(session, 8000, 8000, 8000, 8000);

            const wchar_t* wide_verb = (verb != nullptr && verb[0] == 'P') ? L"POST" : L"GET";
            HINTERNET connect = ::WinHttpConnect(session, host.c_str(), parts.nPort, 0);
            HINTERNET handle = nullptr;
            if (connect)
            {
                const DWORD flags = (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
                handle = ::WinHttpOpenRequest(connect, wide_verb, path.empty() ? L"/" : path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            }

            if (handle)
            {
                const std::wstring headers = L"Content-Type: application/json\r\n";
                const bool sent = body != nullptr
                    ? ::WinHttpSendRequest(handle, headers.c_str(), static_cast<DWORD>(headers.size()), const_cast<char*>(body->data()), static_cast<DWORD>(body->size()), static_cast<DWORD>(body->size()), 0)
                    : ::WinHttpSendRequest(handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

                if (sent && ::WinHttpReceiveResponse(handle, nullptr))
                {
                    result.transport_ok = true;

                    DWORD status = 0;
                    DWORD size = sizeof(status);
                    ::WinHttpQueryHeaders(handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
                    result.status = status;

                    std::string chunk(4096, '\0');
                    DWORD read = 0;
                    while (::WinHttpReadData(handle, chunk.data(), static_cast<DWORD>(chunk.size()), &read) && read > 0)
                    {
                        result.body.append(chunk.data(), read);
                    }
                }
                ::WinHttpCloseHandle(handle);
            }
            if (connect)
            {
                ::WinHttpCloseHandle(connect);
            }
            ::WinHttpCloseHandle(session);
            return result;
        }
    }

    response post_json(const std::wstring& url, const std::string& body)
    {
        return request(url, "POST", &body);
    }

    response get(const std::wstring& url)
    {
        return request(url, "GET", nullptr);
    }

    std::string sha256_hex(const std::string& data)
    {
        BCRYPT_ALG_HANDLE alg = nullptr;
        if (::BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        {
            return {};
        }
        std::uint8_t digest[32]{};
        bool ok = false;
        BCRYPT_HASH_HANDLE hash = nullptr;
        if (::BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0)
        {
            if (::BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())), static_cast<ULONG>(data.size()), 0) == 0 &&
                ::BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0)
            {
                ok = true;
            }
            ::BCryptDestroyHash(hash);
        }
        ::BCryptCloseAlgorithmProvider(alg, 0);
        return ok ? to_hex(digest, sizeof(digest)) : std::string{};
    }

    std::string hmac_sha256_hex(const std::string& key, const std::string& data)
    {
        BCRYPT_ALG_HANDLE alg = nullptr;
        if (::BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
        {
            return {};
        }
        std::uint8_t digest[32]{};
        bool ok = false;
        BCRYPT_HASH_HANDLE hash = nullptr;
        if (::BCryptCreateHash(alg, &hash, nullptr, 0,
                reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())), static_cast<ULONG>(key.size()), 0) == 0)
        {
            if (::BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())), static_cast<ULONG>(data.size()), 0) == 0 &&
                ::BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0)
            {
                ok = true;
            }
            ::BCryptDestroyHash(hash);
        }
        ::BCryptCloseAlgorithmProvider(alg, 0);
        return ok ? to_hex(digest, sizeof(digest)) : std::string{};
    }

    std::string sha256_hex_file(const std::wstring& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
        {
            return {};
        }
        BCRYPT_ALG_HANDLE alg = nullptr;
        if (::BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        {
            return {};
        }

        std::uint8_t digest[32]{};
        bool ok = false;
        BCRYPT_HASH_HANDLE hash = nullptr;
        if (::BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0)
        {
            std::vector<char> buffer(64 * 1024);
            ok = true;
            while (input.good() && ok)
            {
                input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
                const std::streamsize got = input.gcount();
                if (got > 0 &&
                    ::BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(got), 0) != 0)
                {
                    ok = false;
                }
            }
            if (ok && ::BCryptFinishHash(hash, digest, sizeof(digest), 0) != 0)
            {
                ok = false;
            }
            ::BCryptDestroyHash(hash);
        }
        ::BCryptCloseAlgorithmProvider(alg, 0);
        return ok ? to_hex(digest, sizeof(digest)) : std::string{};
    }

    std::string random_hex(std::size_t byte_count)
    {
        std::vector<std::uint8_t> buffer(byte_count);
        if (::BCryptGenRandom(nullptr, buffer.data(), static_cast<ULONG>(buffer.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        {
            return {};
        }
        return to_hex(buffer.data(), buffer.size());
    }

    std::string machine_guid()
    {
        char buffer[128]{};
        DWORD size = sizeof(buffer);
        if (::RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid",
                RRF_RT_REG_SZ, nullptr, buffer, &size) == ERROR_SUCCESS)
        {
            return std::string(buffer);
        }
        return {};
    }

    std::string machine_hwid()
    {
        const std::string guid = machine_guid();
        if (guid.empty())
        {
            return {};
        }

        char drive[8]{};
        const DWORD length = ::GetEnvironmentVariableA("SystemDrive", drive, sizeof(drive));
        const std::string root = (length > 0 && length < 6) ? std::string(drive) + "\\" : "C:\\";

        DWORD serial = 0;
        const std::string serial_text = ::GetVolumeInformationA(root.c_str(), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)
            ? std::to_string(serial)
            : std::string{};

        return sha256_hex(guid + "|" + serial_text);
    }

    std::wstring api_base()
    {
        // Matches fragment's resolve_auth_url().
        return L"https://fragment-site.dcrfozzy.workers.dev";
    }
}
