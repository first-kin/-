// d:\git\-\高校学生信息管理系统\backend\utils\JwtUtil.h
#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include <jwt-cpp/jwt.h>
// jwt-cpp 0.7 起 JSON traits 参数化：这里选用 jsoncpp 后端
// （Drogon 自带依赖，项目内已全局使用 Json::Value，无需额外安装 picojson）
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>

using std::string;

namespace api
{

// JWT 签名密钥：与 JwtFilter 保持同一来源（custom.jwt.secret）
inline string jwtSecret()
{
    const Json::Value &cfg = drogon::app().getCustomConfig();
    if (cfg.isMember("jwt") && cfg["jwt"].isMember("secret") &&
        cfg["jwt"]["secret"].isString() && !cfg["jwt"]["secret"].asString().empty())
        return cfg["jwt"]["secret"].asString();
    return "student-sys-dev-secret";
}

// 签发 Token：claims 与 JwtFilter 校验端严格一致 —— uid(int64) + role(string)
inline string signToken(int64_t uid, const string &role, int64_t expiresIn = 86400)
{
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_issuer("student-sys")
        .set_type("JWS")
        .set_payload_claim("uid", jwt::claim(uid))
        .set_payload_claim("role", jwt::claim(role))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds{expiresIn})
        .sign(jwt::algorithm::hs256{jwtSecret()});
}

} // namespace api
