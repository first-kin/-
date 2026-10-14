#pragma once

#include <cstdint>
#include <string>

#include <json/json.h>

using namespace std;

namespace model
{

// 成绩实体，对应 scores 表；studentNo / studentName / courseName 为联表展示字段
// hasScore 标识 score 列是否为空（缺考/缓考时可为空）
struct Score
{
    int64_t id = 0;
    int64_t studentId = 0;
    string studentNo;
    string studentName;
    int64_t courseId = 0;
    string courseName;
    bool hasScore = false;
    double score = 0.0;
    int status = 0;  // 0 正常 / 1 缺考 / 2 缓考
    string examTime;
};

// 解析成绩请求 JSON 并校验：
//   studentId 必填；courseId 可选（批量录入时由外层统一指定）；
//   status 可选，默认 0；status==0(正常)时 score 必填且 0~100，
//   status==1/2(缺考/缓考)时 score 可为空。examTime 可选。
inline bool scoreFromJson(const Json::Value &j, Score &s, string &err)
{
    if (!j.isObject())
    {
        err = "请求体必须是 JSON 对象";
        return false;
    }

    if (!j.isMember("studentId") || j["studentId"].isNull() || !j["studentId"].isIntegral())
    {
        err = "缺少或非法 studentId";
        return false;
    }
    s.studentId = j["studentId"].asInt64();

    // courseId 可选：批量录入场景由外层 body 统一指定
    if (j.isMember("courseId") && !j["courseId"].isNull())
    {
        if (!j["courseId"].isIntegral())
        {
            err = "courseId 必须为整数";
            return false;
        }
        s.courseId = j["courseId"].asInt64();
    }

    int status = 0;
    if (j.isMember("status") && !j["status"].isNull())
    {
        if (!j["status"].isIntegral())
        {
            err = "status 必须为整数";
            return false;
        }
        status = j["status"].asInt();
    }
    if (status != 0 && status != 1 && status != 2)
    {
        err = "status 只能为 0(正常)/1(缺考)/2(缓考)";
        return false;
    }
    s.status = status;

    if (status == 0)
    {
        // 正常考试必须有分数
        if (!j.isMember("score") || j["score"].isNull() || !j["score"].isNumeric())
        {
            err = "正常考试必须提供 0~100 的 score";
            return false;
        }
        s.score = j["score"].asDouble();
        if (s.score < 0.0 || s.score > 100.0)
        {
            err = "score 必须在 0~100 之间";
            return false;
        }
        s.hasScore = true;
    }
    else if (j.isMember("score") && !j["score"].isNull())
    {
        // 缺考/缓考允许带分数(如缓考后补录)
        if (!j["score"].isNumeric())
        {
            err = "score 必须为数字";
            return false;
        }
        s.score = j["score"].asDouble();
        if (s.score < 0.0 || s.score > 100.0)
        {
            err = "score 必须在 0~100 之间";
            return false;
        }
        s.hasScore = true;
    }
    else
    {
        s.score = 0.0;
        s.hasScore = false;
    }

    if (j.isMember("examTime") && j["examTime"].isString())
        s.examTime = j["examTime"].asString();

    return true;
}

inline Json::Value scoreToJson(const Score &s)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(s.id);
    j["studentId"] = static_cast<Json::Int64>(s.studentId);
    j["studentNo"] = s.studentNo;
    j["studentName"] = s.studentName;
    j["courseId"] = static_cast<Json::Int64>(s.courseId);
    j["courseName"] = s.courseName;
    j["score"] = s.hasScore ? Json::Value(s.score) : Json::Value(Json::nullValue);
    j["status"] = s.status;
    j["examTime"] = s.examTime;
    return j;
}

} // namespace model