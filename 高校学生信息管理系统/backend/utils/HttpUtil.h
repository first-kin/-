#pragma once
// B 模块公共工具：统一错误码(对齐接口文档 1.4)、响应构造、角色守卫、
// LIKE 转义、JSON 参数提取、BCrypt 密码散列、分页体组装。
// 供 Student/Teacher/Dept 三个 Controller 与三个 Service 共用。

#include <cstdint>
#include <string>

#include <drogon/HttpRequest.h>
#include <drogon/utils/Utilities.h>
#include <json/json.h>

#include "utils/Result.h"

using namespace std;

namespace util
{

// ---- 错误码（对齐《接口文档》1.4 全局错误码表）----
const int kParam = 1001;        // 参数校验失败
const int kNoPerm = 1004;       // 无权限访问该资源
const int kNotFound = 1005;     // 资源不存在
const int kDuplicate = 1006;    // 数据重复
const int kRuleConflict = 1007; // 业务规则冲突
const int kDb = 5000;           // 服务器内部错误

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

// ---- BCrypt（Drogon 内置 crypt_blowfish，与 init.sql 中 $2y$ 散列兼容）----
inline string hashPassword(const string &plain) { return drogon::utils::getBcryptHash(plain); }

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