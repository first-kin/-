#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <string>

#include "services/StudentService.h"
#include "utils/HttpUtil.h"
#include "utils/Db.h"
#include "utils/Response.h"
using namespace std;
using namespace drogon;

namespace ctrl
{

// 学生管理接口（接口文档 §5）
// Controller 仅做参数解析、角色判断、调用 Service、组装响应
class StudentController : public HttpController<StudentController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(StudentController::list, "/api/students", Get, "JwtFilter");
    ADD_METHOD_TO(StudentController::detail, "/api/students/{id}", Get, "JwtFilter");
    ADD_METHOD_TO(StudentController::create, "/api/students", Post, "JwtFilter");
    ADD_METHOD_TO(StudentController::update, "/api/students/{id}", Put, "JwtFilter");
    ADD_METHOD_TO(StudentController::remove, "/api/students/{id}", Delete, "JwtFilter");
    ADD_METHOD_TO(StudentController::importCsv, "/api/students/import", Post, "JwtFilter");
    ADD_METHOD_TO(StudentController::exportCsv, "/api/students/export", Get, "JwtFilter");
    METHOD_LIST_END

    // GET /api/students —— admin 全部、teacher 仅任教班级
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
        const int64_t classId = util::toInt(req->getParameter("classId"));
        const int page = static_cast<int>(util::toInt(req->getParameter("page"), 1));
        const int size = static_cast<int>(util::toInt(req->getParameter("size"), 10));

        auto result = svc::StudentService::list(selfTeacherUserId, keyword, classId,
                                                page, size);
        callback(api::toResponse(result));
    }

    // GET /api/students/{id} —— admin、teacher（teacher 仅任教班级）；
    // student 仅可查看本人档案
    void detail(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        const string role = api::currentRole(req);
        if (role == "student")
        {
            // 学生只能查本人：请求 id 必须是自己的学生档案 id
            auto self = api::db()->execSqlSync(
                "SELECT id FROM students WHERE user_id = $1 AND is_deleted = 0 LIMIT 1",
                api::currentUid(req));
            if (self.empty() || self[0]["id"].as<int64_t>() != id)
            {
                callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
                return;
            }
            auto result = svc::StudentService::detail(id, 0);
            callback(api::toResponse(result));
            return;
        }
        if (!util::isAdminOrTeacher(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        const int64_t selfTeacherUserId = util::isAdmin(req) ? 0 : api::currentUid(req);
        auto result = svc::StudentService::detail(id, selfTeacherUserId);
        callback(api::toResponse(result));
    }

    // POST /api/students —— 仅 admin
    void create(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::StudentService::create(req->getJsonObject() ? *req->getJsonObject()
                                                                      : Json::Value());
        callback(api::toResponse(result));
    }

    // PUT /api/students/{id} —— 仅 admin
    void update(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::StudentService::update(
            id, req->getJsonObject() ? *req->getJsonObject() : Json::Value());
        callback(api::toResponse(result));
    }

    // DELETE /api/students/{id} —— 仅 admin
    void remove(const HttpRequestPtr &req,
                function<void(const HttpResponsePtr &)> &&callback, int64_t id)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto result = svc::StudentService::remove(id);
        callback(api::toResponse(result));
    }

    // POST /api/students/import —— 仅 admin，multipart/form-data 字段名 file
    void importCsv(const HttpRequestPtr &req,
                   function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        auto file = req->getUploadedFile("file");
        if (file == nullptr || file->fileContent().empty())
        {
            callback(api::toResponse(util::fail(util::kParam, "请上传要导入的文件")));
            return;
        }
        auto result = svc::StudentService::importCsv(file->fileContent());
        callback(api::toResponse(result));
    }

    // GET /api/students/export —— 仅 admin，返回 CSV 文件流
    void exportCsv(const HttpRequestPtr &req,
                   function<void(const HttpResponsePtr &)> &&callback)
    {
        if (!util::isAdmin(req))
        {
            callback(api::toResponse(util::fail(util::kNoPerm, "无权限访问该资源")));
            return;
        }
        const string &keyword = req->getParameter("keyword");
        const int64_t classId = util::toInt(req->getParameter("classId"));

        auto result = svc::StudentService::exportCsv(0, keyword, classId);
        if (!result.ok())
        {
            callback(api::toResponse(result));
            return;
        }

        // data.content 为带 BOM 的 CSV 文本，直接下发文件流
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setContentTypeCode(CT_CUSTOM);
        resp->addHeader("Content-Type", "text/csv; charset=utf-8");
        resp->addHeader("Content-Disposition",
                        "attachment; filename=" + result.data["filename"].asString());
        resp->setBody(result.data["content"].asString());
        callback(resp);
    }
};

} // namespace ctrl