#pragma once
// B 模块公共工具：统一错误码(对齐接口文档 1.4)、响应构造、角色守卫、
// LIKE 转义、JSON 参数提取、PBKDF2 密码散列、分页体组装。
// 供 Student/Teacher/Dept 三个 Controller 与三个 Service 共用。

#include <cstdint>
#include <cstdlib>
#include <string>

#include <drogon/HttpRequest.h>
#include <json/json.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "utils/Result.h"
#include "utils/Response.h"

using std::string;

namespace util
{

// ---- 错误码：引用 Result.h 的 api::ErrorCode（接口文档 1.4 的唯一代码映射）----
const int kParam = api::E_PARAM;           // 参数校验失败 1001
const int kNoPerm = api::E_FORBIDDEN;      // 无权限访问该资源 1004
const int kNotFound = api::E_NOT_FOUND;    // 资源不存在 1005
const int kDuplicate = api::E_DUPLICATE;   // 数据重复 1006
const int kRuleConflict = api::E_BIZ_RULE; // 业务规则冲突 1007
const int kDb = api::E_INTERNAL;           // 服务器内部错误 5000

// ---- 统一响应构造 ----
inline api::Result okData(const Json::Value &data, const string &msg = "成功")
{
    return api::Result::success(data, msg);
}
inline api::Result okMsg(const string &msg) { return api::Result::success(Json::Value(Json::nullValue), msg); }
inline api::Result fail(int code, const string &msg) { return api::Result::error(code, msg); }

// ---- 角色守卫（role 来自 JwtFilter 写入的 attribute）----
inline bool isAdmin(const drogon::HttpRequestPtr &req) { return api::currentRole(req) == "admin"; }
inline bool isAdminOrTeacher(const drogon::HttpRequestPtr &req)
{
    const string r = api::currentRole(req);
    return r == "admin" || r == "teacher";
}

// ---- LIKE 转义：转义 \ % _，SQL 中需配合 ESCAPE '!' 使用 ----
inline string escapeLike(const string &kw)
{
    string out;
    out.reserve(kw.size() * 2);
    for (char c : kw)
    {
        if (c == '\\' || c == '%' || c == '_')
            out += '!';
        out += c;
    }
    return out;
}
inline string likeAll(const string &kw) { return "%" + escapeLike(kw) + "%"; }

// ---- Query 参数转整数，失败返回默认值 ----
inline int64_t toInt(const string &s, int64_t def = 0)
{
    try
    {
        return stoll(s);
    }
    catch (...)
    {
        return def;
    }
}

// ---- JSON 参数提取 ----
// 必填字符串：缺失/非字符串/空串均判为参数错误
inline bool requireStr(const Json::Value &body, const string &key, string &out, string &err)
{
    if (!body.isMember(key) || !body[key].isString() || body[key].asString().empty())
    {
        err = "参数缺失或不合法: " + key;
        return false;
    }
    out = body[key].asString();
    return true;
}
// 必填整数
inline bool requireInt(const Json::Value &body, const string &key, int64_t &out, string &err)
{
    if (!body.isMember(key) ||
        !(body[key].isInt() || body[key].isInt64() || body[key].isUInt()))
    {
        err = "参数缺失或不合法: " + key;
        return false;
    }
    out = body[key].asInt64();
    return true;
}
// 可选字符串：字段存在且为非空字符串时返回 true；
// 返回 false 表示该列应写入 NULL（避免空串写入非字符串列）
inline bool optStr(const Json::Value &body, const string &key, string &out)
{
    if (body.isMember(key) && body[key].isString())
    {
        out = body[key].asString();
        return !out.empty();
    }
    out.clear();
    return false;
}

// gender 只允许 男 / 女
inline bool validGender(const string &g) { return g == "男" || g == "女"; }

// ---- 密码散列：PBKDF2-HMAC-SHA256（复用已链接的 OpenSSL）----
// 说明：Drogon 1.9.x 未内置 BCrypt 工具函数，故用 OpenSSL 的 PKCS5_PBKDF2_HMAC
// 实现。存储格式: pbkdf2$<iterations>$<salt_hex>$<hash_hex>，最长约 110 字符，
// users.password 列宽需 >= VARCHAR(120)。
constexpr int kPbkdf2Iter = 10000; // 迭代次数：课程项目兼顾登录耗时
constexpr int kPbkdf2Salt = 16;    // 随机盐字节数
constexpr int kPbkdf2Hash = 32;    // SHA-256 摘要字节数

inline string toHex(const unsigned char *buf, int len)
{
    static const char *digits = "0123456789abcdef";
    string out;
    out.reserve(static_cast<size_t>(len) * 2);
    for (int i = 0; i < len; ++i)
    {
        out += digits[buf[i] >> 4];
        out += digits[buf[i] & 0x0F];
    }
    return out;
}

// hex 解码：长度或字符非法返回 false
inline bool fromHex(const string &hex, unsigned char *out, int len)
{
    if (hex.size() != static_cast<size_t>(len) * 2)
        return false;
    auto nibble = [](char c) -> int
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (int i = 0; i < len; ++i)
    {
        const int hi = nibble(hex[2 * i]), lo = nibble(hex[2 * i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out[i] = static_cast<unsigned char>((hi << 4) | lo);
    }
    return true;
}

// 生成密码散列（每次随机盐，同一明文得到不同存储串，属正常现象）
inline string hashPassword(const string &plain)
{
    unsigned char salt[kPbkdf2Salt];
    RAND_bytes(salt, kPbkdf2Salt);
    unsigned char digest[kPbkdf2Hash];
    PKCS5_PBKDF2_HMAC(plain.data(), static_cast<int>(plain.size()),
                      salt, kPbkdf2Salt, kPbkdf2Iter, EVP_sha256(),
                      kPbkdf2Hash, digest);
    return "pbkdf2$" + std::to_string(kPbkdf2Iter) + "$" +
           toHex(salt, kPbkdf2Salt) + "$" + toHex(digest, kPbkdf2Hash);
}

// 校验密码：按存储串中的迭代次数与盐重算，恒定时间比较防时序攻击
inline bool verifyPassword(const string &plain, const string &stored)
{
    const size_t p1 = stored.find('$');
    const size_t p2 = p1 == string::npos ? string::npos : stored.find('$', p1 + 1);
    const size_t p3 = p2 == string::npos ? string::npos : stored.find('$', p2 + 1);
    if (p3 == string::npos || stored.compare(0, p1, "pbkdf2") != 0)
        return false;

    const int iter = atoi(stored.substr(p1 + 1, p2 - p1 - 1).c_str());
    if (iter <= 0 || iter > 1000000)
        return false;

    unsigned char salt[kPbkdf2Salt];
    unsigned char expect[kPbkdf2Hash];
    if (!fromHex(stored.substr(p2 + 1, p3 - p2 - 1), salt, kPbkdf2Salt) ||
        !fromHex(stored.substr(p3 + 1), expect, kPbkdf2Hash))
        return false;

    unsigned char actual[kPbkdf2Hash];
    PKCS5_PBKDF2_HMAC(plain.data(), static_cast<int>(plain.size()),
                      salt, kPbkdf2Salt, iter, EVP_sha256(),
                      kPbkdf2Hash, actual);
    return CRYPTO_memcmp(actual, expect, kPbkdf2Hash) == 0;
}

// ---- 分页响应体 { total, page, size, list } ----
inline Json::Value pageData(int64_t total, int page, int size, const Json::Value &list)
{
    Json::Value d(Json::objectValue);
    d["total"] = static_cast<Json::Int64>(total);
    d["page"] = page;
    d["size"] = size;
    d["list"] = list;
    return d;
}

} // namespace util
