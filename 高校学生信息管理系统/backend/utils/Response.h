#pragma once

#include <cstdint>
#include <string>

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

#include "utils/Result.h"

using std::string;

namespace api
{

// 把业务 Result 统一封装为 { code, msg, data } 的 JSON 响应，
// HTTP 状态码恒为 200，调用方只需关心 body.code。
inline drogon::HttpResponsePtr toResponse(const Result &r)
{
    Json::Value body(Json::objectValue);
    body["code"] = r.code;
    body["msg"] = r.msg;
    body["data"] = r.data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(drogon::k200OK);
    return resp;
}

// 读取 JwtFilter 写入请求属性中的登录用户 id（int64_t），缺失返回 0
// 约定：JwtFilter 校验通过后 attributes()->insert("uid", <int64_t>)
inline int64_t currentUid(const drogon::HttpRequestPtr &req)
{
    auto attrs = req->attributes();
    if (attrs)
        return attrs->get<int64_t>("uid");
    return 0;
}

// 读取 JwtFilter 写入请求属性中的角色（string），缺失返回空串
// 约定：JwtFilter 校验通过后 attributes()->insert("role", <string>)
inline string currentRole(const drogon::HttpRequestPtr &req)
{
    auto attrs = req->attributes();
    if (attrs)
        return attrs->get<string>("role");
    return "";
}

inline bool hasRole(const drogon::HttpRequestPtr &req, const string &role)
{
    return currentRole(req) == role;
}

} // namespace api