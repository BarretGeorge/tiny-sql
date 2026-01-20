#include "tiny_sql/auth/authenticator.h"
#include "tiny_sql/common/logger.h"
#include <openssl/sha.h>
#include <cstring>

namespace tiny_sql {

bool Authenticator::authenticate(const std::string& username,
                                const std::vector<uint8_t>& auth_response,
                                const std::array<uint8_t, 20>& auth_plugin_data) {
    // 获取用户的密码哈希 SHA1(SHA1(password))
    std::vector<uint8_t> password_hash = getPasswordHash(username);

    if (password_hash.empty()) {
        LOG_WARN("User not found: " << username);
        return false;
    }

    // 如果auth_response为空，说明客户端没有密码（或空密码）
    if (auth_response.empty()) {
        // 对于空密码，password_hash应该是SHA1(SHA1(""))
        std::vector<uint8_t> sha1_empty = sha1("");
        std::vector<uint8_t> expected_hash = sha1(sha1_empty.data(), sha1_empty.size());
        return password_hash == expected_hash;
    }

    if (auth_response.size() != 20) {
        LOG_ERROR("Invalid auth response size: " << auth_response.size());
        return false;
    }

    // 计算 SHA1(auth_plugin_data + password_hash)
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), auth_plugin_data.begin(), auth_plugin_data.end());
    combined.insert(combined.end(), password_hash.begin(), password_hash.end());

    std::vector<uint8_t> hash1 = sha1(combined.data(), combined.size());

    // 计算 SHA1(password) = auth_response XOR hash1
    std::vector<uint8_t> sha1_password(20);
    for (size_t i = 0; i < 20; ++i) {
        sha1_password[i] = auth_response[i] ^ hash1[i];
    }

    // 计算 SHA1(SHA1(password))
    std::vector<uint8_t> computed_hash = sha1(sha1_password.data(), sha1_password.size());

    // 比较计算的哈希和存储的哈希
    bool result = (computed_hash == password_hash);

    if (result) {
        LOG_INFO("Authentication successful for user: " << username);
    } else {
        LOG_WARN("Authentication failed for user: " << username);
    }

    return result;
}

std::vector<uint8_t> Authenticator::computeAuthResponse(
    const std::string& password,
    const std::array<uint8_t, 20>& auth_plugin_data) {

    if (password.empty()) {
        return std::vector<uint8_t>(); // 空密码返回空响应
    }

    // 1. SHA1(password)
    std::vector<uint8_t> sha1_pass = sha1(password);

    // 2. SHA1(SHA1(password))
    std::vector<uint8_t> sha1_sha1_pass = sha1(sha1_pass.data(), sha1_pass.size());

    // 3. SHA1(auth_plugin_data + SHA1(SHA1(password)))
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), auth_plugin_data.begin(), auth_plugin_data.end());
    combined.insert(combined.end(), sha1_sha1_pass.begin(), sha1_sha1_pass.end());

    std::vector<uint8_t> hash = sha1(combined.data(), combined.size());

    // 4. SHA1(password) XOR SHA1(auth_plugin_data + SHA1(SHA1(password)))
    std::vector<uint8_t> result(20);
    for (size_t i = 0; i < 20; ++i) {
        result[i] = sha1_pass[i] ^ hash[i];
    }

    return result;
}

std::vector<uint8_t> Authenticator::getPasswordHash(const std::string& username) {
    // 这是一个简化的实现，实际应该从数据库中查询
    // 目前硬编码几个测试用户

    // root用户，密码为空
    if (username == "root" || username == "tiny") {
        // 空密码的SHA1(SHA1(""))
        std::vector<uint8_t> sha1_empty = sha1("");
        return sha1(sha1_empty.data(), sha1_empty.size());
    }

    // test用户，密码为"test"
    if (username == "test") {
        // SHA1(SHA1("test"))
        std::vector<uint8_t> sha1_pass = sha1("test");
        return sha1(sha1_pass.data(), sha1_pass.size());
    }

    // admin用户，密码为"admin123"
    if (username == "admin") {
        // SHA1(SHA1("admin123"))
        std::vector<uint8_t> sha1_pass = sha1("admin123");
        return sha1(sha1_pass.data(), sha1_pass.size());
    }

    // 用户不存在
    return std::vector<uint8_t>();
}

std::vector<uint8_t> Authenticator::sha1(const uint8_t* data, size_t len) {
    std::vector<uint8_t> hash(SHA_DIGEST_LENGTH);
    SHA1(data, len, hash.data());
    return hash;
}

std::vector<uint8_t> Authenticator::sha1(const std::string& str) {
    return sha1(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

} // namespace tiny_sql
