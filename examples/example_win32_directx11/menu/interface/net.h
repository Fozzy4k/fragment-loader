#pragma once

#include <cstddef>
#include <string>

// Minimal WinHTTP + bcrypt helpers used by the loader. Mirrors the protocol in
// fragment's source/auth/key_auth.cpp so both binaries speak to the same worker.
namespace net
{
    struct response
    {
        bool transport_ok = false;
        unsigned long status = 0;
        std::string body;
    };

    response post_json(const std::wstring& url, const std::string& body);
    response get(const std::wstring& url);

    std::string sha256_hex(const std::string& data);
    std::string hmac_sha256_hex(const std::string& key, const std::string& data);
    std::string sha256_hex_file(const std::wstring& path);
    std::string random_hex(std::size_t byte_count);
    std::string machine_guid();

    // Same value fragment computes: sha256(machine_guid + "|" + volume_serial).
    std::string machine_hwid();

    std::wstring api_base();
}
