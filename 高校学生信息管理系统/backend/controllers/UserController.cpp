// d:\git\-\高校学生信息管理系统\backend\controllers\UserController.cpp
#include <exception>
#include <functional>
#include <string>

#include <drogon/HttpController.h>

#include "utils/Db.h"
#include "utils/HttpUtil.h"
#include "utils/Response.h"

using namespace std;
using namespace drogon;

// 个人中心控制器：查看/编辑个人资料、修改密码（USR-01~04）
// 所有接口挂 JwtFilter，uid/role 由 JwtFilter 写入请求属性
class UserController : public drogon::HttpController<UserController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::me, "/api/users/me", Get, "JwtFilter");
    ADD_METHOD_TO(UserController::updateMe, "/api/users/me", Put, "JwtFilter");
    ADD_METHOD_TO(UserController::changePassword, "/api/users/me/password", Post, "JwtFilter");
    METHOD_LIST_END

    // GET /api/users/me —— USR-01
    void me(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
    {
        const int64_t uid = api::currentUid(req);
        try
        {
            auto rows = api::db()->execSqlSync(
                "SELECT u.id, u.username, u.role, "
                "       COALESCE(t.name, s.name, u.username) AS name, "
                "       COALESCE(t.phone, s.phone) AS phone, "
                "       COALESCE(t.email, s.email) AS email, "
                "       t.teacher_no AS teacher_no, s.student_no AS student_no, "
                "       DATE_FORMAT(u.last_login_time, '%Y-%m-%d %H:%i:%s') AS last_login_time "
                "FROM users u "
                "LEFT JOIN teachers t ON t.user_id = u.id AND t.is_deleted = 0 "
                "LEFT JOIN students s ON s.user_id = u.id AND s.is_deleted = 0 "
                "WHERE u.id = ? AND u.is_deleted = 0",
                uid);
            if (rows.empty())
            {
                callback(api::toResponse(api::Result::error(api::E_NOT_FOUND, "用户不存在")));
                return;
            }
            const auto &r = rows[0];
            Json::Value d(Json::objectValue);
            d["id"] = static_cast<Json::Int64>(r["id"].as<int64_t>());
            d["username"] = r["username"].as<string>();
            d["role"] = r["role"].as<string>();
            d["name"] = r["name"].as<string>();
            if (!r["phone"].isNull())
                d["phone"] = r["phone"].as<string>();
            if (!r["email"].isNull())
                d["email"] = r["email"].as<string>();
            if (!r["teacher_no"].isNull())
                d["teacherNo"] = r["teacher_no"].as<string>();
            if (!r["student_no"].isNull())
                d["studentNo"] = r["student_no"].as<string>();
            if (!r["last_login_time"].isNull())
                d["lastLoginTime"] = r["last_login_time"].as<string>();
            callback(api::toResponse(api::Result::success(d)));
        }
        catch (const exception &e)
        {
            LOG_ERROR << "users/me error: " << e.what();
            callback(api::toResponse(api::Result::error(api::E_INTERNAL, "服务器内部错误")));
        }
    }

    // PUT /api/users/me  { phone, email } —— USR-03
    // 按角色更新对应扩展表；admin 无扩展资料直接成功
    void updateMe(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
    {
        const string role = api::currentRole(req);
        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        if (role == "admin")
        {
            callback(api::toResponse(api::Result::success(Json::Value(Json::nullValue), "管理员账号无需维护联系资料")));
            return;
        }

        // 空串置 NULL（NULLIF），与 optStr 的"空即 NULL"约定一致
        const string phone = body->isMember("phone") && (*body)["phone"].isString()
                                 ? (*body)["phone"].asString() : "";
        const string email = body->isMember("email") && (*body)["email"].isString()
                                 ? (*body)["email"].asString() : "";

        try
        {
            if (role == "teacher")
                api::db()->execSqlSync(
                    "UPDATE teachers SET phone = NULLIF(?,''), email = NULLIF(?,'') "
                    "WHERE user_id = ? AND is_deleted = 0",
                    api::currentUid(req), phone, email);
            else
                api::db()->execSqlSync(
                    "UPDATE students SET phone = NULLIF(?,''), email = NULLIF(?,'') "
                    "WHERE user_id = ? AND is_deleted = 0",
                    api::currentUid(req), phone, email);
            callback(api::toResponse(api::Result::success(Json::Value(Json::nullValue), "资料更新成功")));
        }
        catch (const exception &e)
        {
            LOG_ERROR << "update me error: " << e.what();
            callback(api::toResponse(api::Result::error(api::E_INTERNAL, "服务器内部错误")));
        }
    }

    // POST /api/users/me/password  { oldPassword, newPassword } —— USR-02/04
    void changePassword(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback)
    {
        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        string oldPwd, newPwd, err;
        if (!util::requireStr(*body, "oldPassword", oldPwd, err) ||
            !util::requireStr(*body, "newPassword", newPwd, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }
        if (newPwd.size() < 6)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "新密码长度不能少于6位")));
            return;
        }

        try
        {
            auto rows = api::db()->execSqlSync(
                "SELECT password FROM users WHERE id = ? AND is_deleted = 0",
                api::currentUid(req));
            if (rows.empty() ||
                !util::verifyPassword(oldPwd, rows[0]["password"].as<string>()))
            {
                callback(api::toResponse(api::Result::error(api::E_LOGIN_FAILED, "原密码错误")));
                return;
            }
            api::db()->execSqlSync(
                "UPDATE users SET password = ? WHERE id = ?",
                api::currentUid(req), util::hashPassword(newPwd));
            callback(api::toResponse(api::Result::success(Json::Value(Json::nullValue), "密码修改成功")));
        }
        catch (const exception &e)
        {
            LOG_ERROR << "change password error: " << e.what();
            callback(api::toResponse(api::Result::error(api::E_INTERNAL, "服务器内部错误")));
        }
    }
};
