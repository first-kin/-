#include <cstdint>
#include <functional>
#include <string>

#include <drogon/HttpController.h>

#include "models/Notice.h"
#include "services/NoticeService.h"
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

// 只有管理员能写公告；读取类接口对所有已登录用户开放
bool requireAdmin(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &callback,
                  const string &msg)
{
    if (api::currentRole(req) != "admin")
    {
        callback(api::toResponse(api::Result::error(api::E_FORBIDDEN, msg)));
        return false;
    }
    return true;
}

} // namespace

// 通知公告控制器（C 部分）
//   管理员：发布/编辑/删除/置顶
//   全体登录用户：查看列表、详情、首页最新公告
class NoticeController : public drogon::HttpController<NoticeController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(NoticeController::list, "/api/notices", Get, "JwtFilter");
    ADD_METHOD_TO(NoticeController::latest, "/api/notices/latest", Get, "JwtFilter");
    ADD_METHOD_TO(NoticeController::detail, "/api/notices/{id}", Get, "JwtFilter");
    ADD_METHOD_TO(NoticeController::create, "/api/notices", Post, "JwtFilter");
    ADD_METHOD_TO(NoticeController::update, "/api/notices/{id}", Put, "JwtFilter");
    ADD_METHOD_TO(NoticeController::remove, "/api/notices/{id}", Delete, "JwtFilter");
    ADD_METHOD_TO(NoticeController::setTop, "/api/notices/{id}/top", Put, "JwtFilter");
    METHOD_LIST_END

    // GET /api/notices?page=1&size=10&keyword=选课
    void list(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        const string keyword = req->getParameter("keyword");
        const int page = toInt(req->getParameter("page"), 1);
        const int size = toInt(req->getParameter("size"), 10);
        callback(api::toResponse(svc::NoticeService::list(keyword, page, size)));
    }

    // GET /api/notices/latest?limit=5
    void latest(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        callback(api::toResponse(
            svc::NoticeService::latest(toInt(req->getParameter("limit"), 5))));
    }

    // GET /api/notices/{id}
    void detail(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        const int64_t id = toInt64(req->getParameter("id"), 0);
        if (id <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "id 非法")));
            return;
        }
        callback(api::toResponse(svc::NoticeService::detail(id)));
    }

    // POST /api/notices  { title, content, isTop }
    void create(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAdmin(req, callback, "仅管理员可发布公告"))
            return;

        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        model::Notice n;
        string err;
        if (!model::noticeFromJson(*body, n, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }
        callback(api::toResponse(svc::NoticeService::create(n, api::currentUid(req))));
    }

    // PUT /api/notices/{id}  { title, content, isTop }
    void update(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAdmin(req, callback, "仅管理员可修改公告"))
            return;

        const int64_t id = toInt64(req->getParameter("id"), 0);
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
        model::Notice n;
        n.id = id;
        string err;
        if (!model::noticeFromJson(*body, n, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }
        callback(api::toResponse(svc::NoticeService::update(id, n)));
    }

    // DELETE /api/notices/{id}
    void remove(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAdmin(req, callback, "仅管理员可删除公告"))
            return;

        const int64_t id = toInt64(req->getParameter("id"), 0);
        if (id <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "id 非法")));
            return;
        }
        callback(api::toResponse(svc::NoticeService::remove(id)));
    }

    // PUT /api/notices/{id}/top  { isTop: true|false }
    void setTop(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAdmin(req, callback, "仅管理员可置顶公告"))
            return;

        const int64_t id = toInt64(req->getParameter("id"), 0);
        if (id <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "id 非法")));
            return;
        }

        bool isTop = true;
        auto body = req->getJsonObject();
        if (body && (*body).isMember("isTop"))
        {
            if ((*body)["isTop"].isBool())
                isTop = (*body)["isTop"].asBool();
            else if ((*body)["isTop"].isIntegral())
                isTop = ((*body)["isTop"].asInt() != 0);
        }
        callback(api::toResponse(svc::NoticeService::setTop(id, isTop)));
    }
};