#include <cstdint>
#include <functional>
#include <string>

#include <drogon/HttpController.h>

#include "models/Course.h"
#include "services/CourseService.h"
#include "utils/Db.h"
#include "utils/Response.h"
using namespace std;

using namespace drogon;

namespace
{

int toInt(const string &s, int def)
{
    if (s.empty())
        return def;
    try
    {
        return stoi(s);
    }
    catch (...)
    {
        return def;
    }
}

int64_t toInt64(const string &s, int64_t def)
{
    if (s.empty())
        return def;
    try
    {
        return stoll(s);
    }
    catch (...)
    {
        return def;
    }
}

// 角色守卫：不满足时直接回写错误响应并返回 false
bool requireRole(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &callback,
                 const string &role, const string &msg)
{
    if (api::currentRole(req) != role)
    {
        callback(api::toResponse(api::Result::error(api::E_FORBIDDEN, msg)));
        return false;
    }
    return true;
}

} // namespace

// 课程管理控制器（C 部分）
//   权限：管理员增删改查；教师仅查看本人任教课程；学生仅查看
class CourseController : public drogon::HttpController<CourseController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CourseController::list, "/api/courses", Get, "JwtFilter");
    ADD_METHOD_TO(CourseController::detail, "/api/courses/{id}", Get, "JwtFilter");
    ADD_METHOD_TO(CourseController::create, "/api/courses", Post, "JwtFilter");
    ADD_METHOD_TO(CourseController::update, "/api/courses/{id}", Put, "JwtFilter");
    ADD_METHOD_TO(CourseController::remove, "/api/courses/{id}", Delete, "JwtFilter");
    METHOD_LIST_END

    // GET /api/courses?page=1&size=10&keyword=张&semester=2025-2026-1
    void list(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        const string role = api::currentRole(req);
        const int64_t uid = api::currentUid(req);
        const string keyword = req->getParameter("keyword");
        const int page = toInt(req->getParameter("page"), 1);
        const int size = toInt(req->getParameter("size"), 10);
        const string semester = req->getParameter("semester");

        // CRS-04：教师只看本人课程；管理员可传 teacherId 过滤
        // 注意：JWT 里的 uid 是 users.id，但 courses.teacher_id 存的是 teachers.id
        int64_t teacherFilter = 0;
        if (role == "teacher")
        {
            auto rows = api::db()->execSqlSync(
                "SELECT id FROM teachers WHERE user_id = ? AND is_deleted = 0 LIMIT 1", uid);
            teacherFilter = rows.empty() ? -1 : rows[0]["id"].as<int64_t>();
        }
        else
            teacherFilter = toInt64(req->getParameter("teacherId"), 0);

        // 学生只能看到本班开设的课程：users.id -> students.class_id
        int64_t classFilter = 0;
        if (role == "student")
        {
            auto rows = api::db()->execSqlSync(
                "SELECT class_id FROM students WHERE user_id = ? AND is_deleted = 0 LIMIT 1", uid);
            // 未分班的学生用 -1 过滤，查不到任何课程
            classFilter = (!rows.empty() && !rows[0]["class_id"].isNull())
                              ? rows[0]["class_id"].as<int64_t>()
                              : -1;
        }
        else
        {
            classFilter = toInt64(req->getParameter("classId"), 0);
        }

        callback(api::toResponse(
            svc::CourseService::list(keyword, page, size, teacherFilter, semester, classFilter)));
    }

    // GET /api/courses/{id}
    void detail(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback,
                int64_t id) const
    {
        if (id <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "id 非法")));
            return;
        }
        callback(api::toResponse(svc::CourseService::detail(id)));
    }

    // POST /api/courses
    void create(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireRole(req, callback, "admin", "仅管理员可新增课程"))
            return;

        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        model::Course c;
        string err;
        if (!model::courseFromJson(*body, c, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }
        callback(api::toResponse(svc::CourseService::create(c)));
    }

    // PUT /api/courses/{id}（全量更新）
    void update(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback,
                int64_t id) const
    {
        if (!requireRole(req, callback, "admin", "仅管理员可修改课程"))
            return;

        if (id <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "id 非法")));
            return;
        }
        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        model::Course c;
        c.id = id;
        string err;
        if (!model::courseFromJson(*body, c, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }
        callback(api::toResponse(svc::CourseService::update(id, c)));
    }

    // DELETE /api/courses/{id}
    void remove(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback,
                int64_t id) const
    {
        if (!requireRole(req, callback, "admin", "仅管理员可删除课程"))
            return;

        if (id <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "id 非法")));
            return;
        }
        callback(api::toResponse(svc::CourseService::remove(id)));
    }
};