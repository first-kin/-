#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <drogon/HttpController.h>

#include "models/Score.h"
#include "services/ScoreService.h"
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

bool requireAnyRole(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &callback,
                    const vector<string> &roles, const string &msg)
{
    const string role = api::currentRole(req);
    for (const auto &r : roles)
    {
        if (role == r)
            return true;
    }
    callback(api::toResponse(api::Result::error(api::E_FORBIDDEN, msg)));
    return false;
}

} // namespace

// 成绩管理控制器（C 部分）
//   管理员：全部成绩的查看、录入、修改、统计、撤销
//   教师  ：仅本人任教课程的成绩录入/修改/统计
//   学生  ：仅本人成绩
class ScoreController : public drogon::HttpController<ScoreController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ScoreController::list, "/api/scores", Get, "JwtFilter");
    ADD_METHOD_TO(ScoreController::myScores, "/api/scores/my", Get, "JwtFilter");
    ADD_METHOD_TO(ScoreController::statistics, "/api/scores/statistics", Get, "JwtFilter");
    ADD_METHOD_TO(ScoreController::history, "/api/scores/history", Get, "JwtFilter");
    ADD_METHOD_TO(ScoreController::save, "/api/scores", Post, "JwtFilter");
    ADD_METHOD_TO(ScoreController::batchSave, "/api/scores/batch", Post, "JwtFilter");
    ADD_METHOD_TO(ScoreController::undo, "/api/scores/undo", Post, "JwtFilter");
    METHOD_LIST_END

    // GET /api/scores?courseId=1&studentId=2&keyword=张&page=1&size=10
    void list(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAnyRole(req, callback, {"admin", "teacher"}, "无权查看成绩列表"))
            return;

        const string role = api::currentRole(req);
        const int64_t uid = api::currentUid(req);
        const int64_t courseId = toInt64(req->getParameter("courseId"), 0);
        const int64_t studentId = toInt64(req->getParameter("studentId"), 0);
        const string keyword = req->getParameter("keyword");
        const int page = toInt(req->getParameter("page"), 1);
        const int size = toInt(req->getParameter("size"), 10);

        const int64_t teacherScope = (role == "teacher") ? uid : 0;
        callback(api::toResponse(
            svc::ScoreService::list(courseId, studentId, keyword, page, size, teacherScope)));
    }

    // GET /api/scores/my?semester=2025-2026-1  （按登录用户取本人成绩）
    void myScores(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        const int64_t uid = api::currentUid(req);
        if (uid <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_UNAUTHORIZED, "未登录")));
            return;
        }
        callback(api::toResponse(
            svc::ScoreService::myScores(uid, req->getParameter("semester"))));
    }

    // GET /api/scores/statistics?courseId=1
    void statistics(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAnyRole(req, callback, {"admin", "teacher"}, "无权查看成绩统计"))
            return;

        const int64_t courseId = toInt64(req->getParameter("courseId"), 0);
        if (courseId <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "courseId 非法")));
            return;
        }
        const int64_t teacherScope = (api::currentRole(req) == "teacher") ? api::currentUid(req) : 0;
        callback(api::toResponse(svc::ScoreService::statistics(courseId, teacherScope)));
    }

    // GET /api/scores/history?limit=10  （查看成绩修改留痕）
    void history(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAnyRole(req, callback, {"admin"}, "仅管理员可查看成绩修改记录"))
            return;
        callback(api::toResponse(
            svc::ScoreService::history(toInt(req->getParameter("limit"), 10))));
    }

    // POST /api/scores  { studentId, courseId, score, status, examTime }
    void save(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAnyRole(req, callback, {"admin", "teacher"}, "无权录入成绩"))
            return;

        auto body = req->getJsonObject();
        if (!body)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        model::Score s;
        string err;
        if (!model::scoreFromJson(*body, s, err))
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, err)));
            return;
        }
        const int64_t teacherScope = (api::currentRole(req) == "teacher") ? api::currentUid(req) : 0;
        callback(api::toResponse(svc::ScoreService::save(s, teacherScope)));
    }

    // POST /api/scores/batch  { courseId, items: [ { studentId, score, status, examTime }, ... ] }
    void batchSave(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAnyRole(req, callback, {"admin", "teacher"}, "无权录入成绩"))
            return;

        auto body = req->getJsonObject();
        if (!body || !body->isObject())
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "请求体不是合法 JSON")));
            return;
        }
        const int64_t courseId = (*body).isMember("courseId") ? (*body)["courseId"].asInt64() : 0;
        if (courseId <= 0)
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "缺少或非法 courseId")));
            return;
        }
        if (!(*body).isMember("items") || !(*body)["items"].isArray())
        {
            callback(api::toResponse(api::Result::error(api::E_PARAM, "items 必须为数组")));
            return;
        }

        vector<model::Score> items;
        const Json::Value &arr = (*body)["items"];
        for (Json::ArrayIndex i = 0; i < arr.size(); ++i)
        {
            model::Score s;
            string err;
            if (!model::scoreFromJson(arr[i], s, err))
            {
                callback(api::toResponse(api::Result::error(
                    api::E_PARAM, "第 " + to_string(i + 1) + " 条数据非法: " + err)));
                return;
            }
            s.courseId = courseId;
            items.push_back(s);
        }

        const int64_t teacherScope = (api::currentRole(req) == "teacher") ? api::currentUid(req) : 0;
        callback(api::toResponse(svc::ScoreService::batchSave(courseId, items, teacherScope)));
    }

    // POST /api/scores/undo  （撤销最近一次成绩修改）
    void undo(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) const
    {
        if (!requireAnyRole(req, callback, {"admin"}, "仅管理员可撤销成绩修改"))
            return;
        callback(api::toResponse(svc::ScoreService::undoLast()));
    }
};