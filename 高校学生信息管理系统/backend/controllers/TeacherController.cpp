#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <string>

#include "services/TeacherService.h"
#include "utils/HttpUtil.h"
#include "utils/Response.h"
using namespace std;
using namespace drogon;

namespace ctrl
{

// 教师管理接口（接口文档 §7）
// Controller 仅做参数解析、角色判断、调用 Service、组装响应
class TeacherController : public HttpController<TeacherController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TeacherController::list, "/api/teachers", Get, "JwtFilter");
    ADD_METHOD_TO(TeacherController::detail, "/api/teachers/{id}", Get, "JwtFilter");
    ADD_METHOD_TO(TeacherController::create, "/api/teachers", Post, "JwtFilter");
    ADD_METHOD_TO(TeacherController::update, "/api/teachers/{id}", Put, "JwtFilter");
    ADD_METHOD_TO(TeacherController::remove, "/api/teachers/{id}", Delete, "JwtFilter");
    METHOD_LIST_END

    // GET /api/teachers —— admin 全部、teacher 仅本人
    void list(const HttpRequestPtr &req,
              function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdminOrTeacher(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        const int64_t selfTeacherUserId = util::isAdmin(req) ? 0 : api::currentUid(req);
        const string &keyword = req->getParameter("keyword");
        const int64_t deptId = util::toInt(req->getParameter("deptId"));
        const int page = static_cast<int>(util::toInt(req->getParameter("page"), 1));
        const int size = static_cast<int>(util::toInt(req->getParameter("size"), 10));

        auto result = svc::TeacherService::list(selfTeacherUserId, keyword, deptId,
                                                page, size);
        callback(api::toResponse(result));
    }

    // GET /api/teachers/{id} —— admin、teacher（仅本人）
    void detail(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdminOrTeacher(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        const int64_t selfTeacherUserId = util::isAdmin(req) ? 0 : api::currentUid(req);
        auto result = svc::TeacherService::detail(id, selfTeacherUserId);
        callback(api::toResponse(result));
    }

    // POST /api/teachers —— 仅 admin
    void create(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::TeacherService::create(req->getJsonObject() ? *req->getJsonObject()
                                                                       : Json::Value());
        callback(api::toResponse(result));
    }

    // PUT /api/teachers/{id} —— 仅 admin
    void update(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::TeacherService::update(
            id, req->getJsonObject() ? *req->getJsonObject() : Json::Value());
        callback(api::toResponse(result));
    }

    // DELETE /api/teachers/{id} —— 仅 admin
    void remove(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::TeacherService::remove(id);
        callback(api::toResponse(result));
    }
};

} // namespace ctrl