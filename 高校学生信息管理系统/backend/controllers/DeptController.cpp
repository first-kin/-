#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <string>

#include "services/DeptService.h"
#include "utils/HttpUtil.h"
#include "utils/Response.h"
using namespace std;
using namespace drogon;

namespace ctrl
{

// 院系与班级管理接口（接口文档 §6）
// Controller 仅做参数解析、角色判断、调用 Service、组装响应
class DeptController : public HttpController<DeptController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DeptController::deptList, "/api/departments", Get, "JwtFilter");
    ADD_METHOD_TO(DeptController::deptCreate, "/api/departments", Post, "JwtFilter");
    ADD_METHOD_TO(DeptController::deptUpdate, "/api/departments/{id}", Put, "JwtFilter");
    ADD_METHOD_TO(DeptController::deptRemove, "/api/departments/{id}", Delete, "JwtFilter");
    ADD_METHOD_TO(DeptController::majorList, "/api/majors", Get, "JwtFilter");
    ADD_METHOD_TO(DeptController::classList, "/api/classes", Get, "JwtFilter");
    ADD_METHOD_TO(DeptController::classCreate, "/api/classes", Post, "JwtFilter");
    ADD_METHOD_TO(DeptController::classUpdate, "/api/classes/{id}", Put, "JwtFilter");
    ADD_METHOD_TO(DeptController::classRemove, "/api/classes/{id}", Delete, "JwtFilter");
    METHOD_LIST_END

    // GET /api/departments —— 登录用户均可查看
    void deptList(const HttpRequestPtr &req,
                  function<void(const HttpResponsePtr &)> &&callback)
    {
        callback(api::toResponse(svc::DeptService::deptList()));
    }

    // GET /api/majors —— admin 用于选择班级所属专业
    void majorList(const HttpRequestPtr &req,
                   function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        callback(api::toResponse(svc::DeptService::majorList()));
    }

    // POST /api/departments —— 仅 admin
    void deptCreate(const HttpRequestPtr &req,
                    function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::DeptService::deptCreate(req->getJsonObject() ? *req->getJsonObject()
                                                                        : Json::Value());
        callback(api::toResponse(result));
    }

    // PUT /api/departments/{id} —— 仅 admin
    void deptUpdate(const HttpRequestPtr &req,
                    function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::DeptService::deptUpdate(
            id, req->getJsonObject() ? *req->getJsonObject() : Json::Value());
        callback(api::toResponse(result));
    }

    // DELETE /api/departments/{id} —— 仅 admin
    void deptRemove(const HttpRequestPtr &req,
                    function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::DeptService::deptRemove(id);
        callback(api::toResponse(result));
    }

    // GET /api/classes —— admin、teacher
    void classList(const HttpRequestPtr &req,
                   function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdminOrTeacher(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        const int64_t majorId = util::toInt(req->getParameter("majorId"));
        auto result = svc::DeptService::classList(majorId);
        callback(api::toResponse(result));
    }

    // POST /api/classes —— 仅 admin
    void classCreate(const HttpRequestPtr &req,
                     function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::DeptService::classCreate(req->getJsonObject() ? *req->getJsonObject()
                                                                         : Json::Value());
        callback(api::toResponse(result));
    }

    // PUT /api/classes/{id} —— 仅 admin
    void classUpdate(const HttpRequestPtr &req,
                     function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::DeptService::classUpdate(
            id, req->getJsonObject() ? *req->getJsonObject() : Json::Value());
        callback(api::toResponse(result));
    }

    // DELETE /api/classes/{id} —— 仅 admin
    void classRemove(const HttpRequestPtr &req,
                     function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::DeptService::classRemove(id);
        callback(api::toResponse(result));
    }
};

} // namespace ctrl
