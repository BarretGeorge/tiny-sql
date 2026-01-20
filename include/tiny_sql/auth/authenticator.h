#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>

namespace tiny_sql {

/**
 * MySQL认证器
 * 实现mysql_native_password认证方法
 */
class Authenticator {
public:
    /**
     * 验证客户端的认证响应
     * @param username 用户名
     * @param auth_response 客户端发送的认证响应
     * @param auth_plugin_data 服务器发送的挑战数据（20字节）
     * @return 认证是否成功
     */
    static bool authenticate(const std::string& username,
                           const std::vector<uint8_t>& auth_response,
                           const std::array<uint8_t, 20>& auth_plugin_data);

    /**
     * 计算mysql_native_password认证响应
     * SHA1( password ) XOR SHA1( "20-bytes random data from server" <concat> SHA1( SHA1( password ) ) )
     *
     * @param password 明文密码
     * @param auth_plugin_data 服务器发送的挑战数据（20字节）
     * @return 认证响应（20字节）
     */
    static std::vector<uint8_t> computeAuthResponse(
        const std::string& password,
        const std::array<uint8_t, 20>& auth_plugin_data);

    /**
     * 获取用户的密码哈希（SHA1(SHA1(password))）
     * 在实际应用中，这应该从数据库中查询
     *
     * @param username 用户名
     * @return 密码的双重SHA1哈希（20字节），如果用户不存在返回空
     */
    static std::vector<uint8_t> getPasswordHash(const std::string& username);

    /**
     * 计算SHA1哈希
     */
    static std::vector<uint8_t> sha1(const uint8_t* data, size_t len);

    /**
     * 计算字符串的SHA1哈希
     */
    static std::vector<uint8_t> sha1(const std::string& str);

private:
    Authenticator() = default;
};

} // namespace tiny_sql
