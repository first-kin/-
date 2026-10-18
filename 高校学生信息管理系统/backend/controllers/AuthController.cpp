// d:\git\-\高校学生信息管理系统\backend\controllers\AuthController.cpp
#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <ctime>

#include <drogon/HttpController.h>

#include "utils/Db.h"
#include "utils/HttpUtil.h"
#include "utils/JwtUtil.h"
#include "utils/Response.h"

using namespace std;
using namespace drogon;

namespace
{
// AUTH-04：连续输错 5 次锁定 10 分钟。
// 进程内存实现（重启清零）——users 表无锁定字段，课程项目够用，
// 生产环境应加 failed_attempts / locked_until 表字段。
constexpr int kMaxFail = 5;
constexpr int kLockSeconds = 600;
constexpr int kTokenTtl = 86400; // Token 有效期 24h（AUTH-02）

struct LoginGuard
{
    static map<string, pair<int, time_t>> &fails()
    {
        static map<string, pair<int, time_t>> m; // username -> {次数, 锁定截止时间}
        return m;
    }
    static mutex &mtx()
    {
        static mutex m;
        return m;
    }
    static void record(const string &username)
    {
        lock_guard<mutex> lk(mtx());
        auto &item = fails()[username];
        item.first += 1;
        if (item.first >= kMaxFail)
            item.second = time(nullptr) + kLockSeconds;
    }
    static bool locked(const string &username)
    {
        lock_guard<mutex> lk(mtx());
        auto it = fails().find(username);
        return it != fails().end() && it->second.second > time(nullptr);
    }
    static void clear(const string &username)
    {
        lock_guard<mutex> lk(mtx());
        fails().erase(username);
    }
};
} // namespace

// 认证控制器（A 部分）：登录 / 登出
class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    // login 不挂 JwtFilter（白名单）；logout 挂载，保证只有登录用户能调
    ADD_METHOD_TO(AuthController::login, "/api/auth/login", Post);
    ADD_METHOD_TO(AuthController::logout, "/api/auth/logout", Post, "JwtFilter");
    METHOD_LIST_END

    // POST /api/auth/login  { username, password }
    void login(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
    {
        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        string username, password, err;
        if (!util::requireStr(*body, "username", username, err) ||
            !util::requireStr(*body, "password", password, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }

        // ① 锁定检查（AUTH-04）
        if (LoginGuard::locked(username))
        {
            callback(api::toResponse(api::Result::error(api::E_LOCKED, "失败次数过多，账号已锁定，请10分钟后再试")));
            return;
        }

        try
        {
            // ② 查用户：LEFT JOIN 取教师/学生姓名（接口文档 §3.1 返回 name 字段）
            auto rows = api::db()->execSqlSync(
                "SELECT u.id, u.password, u.role, u.status, "
                "       COALESCE(t.name, s.name, u.username) AS name "
                "FROM users u "
                "LEFT JOIN teachers t ON t.user_id = u.id AND t.is_deleted = 0 "
                "LEFT JOIN students s ON s.user_id = u.id AND s.is_deleted = 0 "
                "WHERE u.username = ? AND u.is_deleted = 0",
                username);

            // 用户不存在：文案与密码错误一致，避免撞库探测
            if (rows.empty())
            {
                LoginGuard::record(username);
                callback(api::toResponse(api::Result::error(api::E_LOGIN_FAILED, "用户名或密码错误")));
                return;
            }
            const auto &row = rows[0];

            // ③ 禁用检查
            if (row["status"].as<int>() == 0)
            {
                callback(api::toResponse(api::Result::error(api::E_LOCKED, "账号已被禁用，请联系管理员")));
                return;
            }

            // ④ 校验密码（PBKDF2-HMAC-SHA256，与 init.sql 种子散列兼容）
            if (!util::verifyPassword(password, row["password"].as<string>()))
            {
                LoginGuard::record(username);
                callback(api::toResponse(api::Result::error(api::E_LOGIN_FAILED, "用户名或密码错误")));
                return;
            }

            const int64_t uid = row["id"].as<int64_t>();
            const string role = row["role"].as<string>();

            // ⑤ 签发 JWT + 组装响应
            Json::Value d(Json::objectValue);
            d["token"] = api::signToken(uid, role, kTokenTtl);
            d["role"] = role;
            d["name"] = row["name"].as<string>();
            d["expiresIn"] = kTokenTtl;

            // ⑥ 登录成功：清失败计数 + 更新最后登录时间
            LoginGuard::clear(username);
            api::db()->execSqlSync(
                "UPDATE users SET last_login_time = NOW() WHERE id = ?", uid);

            callback(api::toResponse(api::Result::success(d, "登录成功")));
        }
        catch (const exception &e)
        {
            LOG_ERROR << "login error: " << e.what();
            callback(api::toResponse(api::Result::error(api::E_INTERNAL, "服务器内部错误")));
        }
    }

    // POST /api/auth/logout —— JWT 无状态，前端删除本地 Token 即可
    void logout(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
    {
        callback(api::toResponse(api::Result::success(Json::Value(Json::nullValue), "退出成功")));
    }
};
