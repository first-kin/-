#include <jwt-cpp/jwt.h>
// jwt-cpp 0.7 起 JSON traits 参数化：这里选用 jsoncpp 后端
// （Drogon 自带依赖，项目内已全局使用 Json::Value，无需额外安装 picojson）
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpFilter.h>

#include <string>

#include "utils/Response.h"
#include "utils/Result.h"

using namespace std;
using namespace drogon;

namespace
{
// JWT 签名密钥：优先读 config/app.json 的 custom.jwt.secret，未配置时回退开发默认值
const string &jwtSecret()
{
    static const string kSecret = [] {
        const Json::Value &cfg = app().getCustomConfig();
        if (cfg.isMember("jwt") && cfg["jwt"].isMember("secret") &&
            cfg["jwt"]["secret"].isString() && !cfg["jwt"]["secret"].asString().empty())
        {
            return cfg["jwt"]["secret"].asString();
        }
        return string("student-sys-dev-secret");
    }();
    return kSecret;
}

// 鉴权失败统一返回业务码 1003（HTTP 恒 200，见接口文档 1.3 / 1.4）
HttpResponsePtr unauthorized(const string &msg)
{
    return api::toResponse(api::Result::error(api::E_TOKEN_INVALID, msg));
}
} // namespace

// 全局 JWT 鉴权过滤器：Controller 通过 "JwtFilter" 名称挂载；
// 登录路由不挂载本过滤器，天然构成白名单。
// 校验通过后把 uid / role 写入请求属性，
// 由 Response.h 的 currentUid() / currentRole() 与 HttpUtil.h 的角色守卫消费。
class JwtFilter : public HttpFilter<JwtFilter>
{
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override
    {
        const string &auth = req->getHeader("Authorization");

        // 1. 头格式校验：Authorization: Bearer <token>
        if (auth.rfind("Bearer ", 0) != 0 || auth.size() <= 7)
        {
            fcb(unauthorized("未登录或缺少 Token"));
            return;
        }
        const string token = auth.substr(7);

        try
        {
            // 2. 验签 + 过期校验（HS256）
            auto decoded = jwt::decode(token);
            jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{jwtSecret()})
                .verify(decoded);

            // 3. 提取关键 claims，缺失视为非法 Token
            if (!decoded.has_payload_claim("uid") || !decoded.has_payload_claim("role"))
            {
                fcb(unauthorized("Token 无效"));
                return;
            }
            // jwt-cpp 的 claim 取值 API：as_integer() / as_string()（没有 as<T>() 泛型）
            int64_t uid = decoded.get_payload_claim("uid").as_integer();
            string role = decoded.get_payload_claim("role").as_string();

            // 4. 写入请求属性，供下游 Controller / Service 读取
            req->attributes()->insert("uid", uid);
            req->attributes()->insert("role", role);

            fccb();
        }
        catch (const jwt::error::token_verification_exception &e)
        {
            // jwt-cpp 0.7：过期是 token_verification_exception 的一种错误码
            if (e.code().value() ==
                static_cast<int>(jwt::error::token_verification_error::token_expired))
                fcb(unauthorized("Token 已过期，请重新登录"));
            else
                fcb(unauthorized("Token 无效"));
        }
        catch (const std::exception &)
        {
            fcb(unauthorized("Token 无效"));
        }
    }
};

// 无需 REGISTER_FILTER 宏：HttpFilter<JwtFilter> 默认 AutoCreation=true，
// Drogon 启动时自动创建并注册本过滤器（此版本 drogon 已移除该宏）
