#pragma once

#include <cstdint>
#include <string>

#include <json/json.h>

using std::string;

namespace model
{

// 课程实体，对应 courses 表；teacherName 为联表展示字段(非表字段)
struct Course
{
    int64_t id = 0;
    string courseName;
    double credit = 0.0;
    int64_t teacherId = 0;  // 0 表示未指定授课教师
    string teacherName;
    int64_t classId = 0;    // 0 表示未指定开设班级
    string className;
    string semester;
    string courseTime;
    string room;
    int status = 1;  // 0 未开课 1 开课中 2 已结课
    string createTime;
};

// 解析请求 JSON 并做字段校验（新增、修改均要求 courseName / credit，
// 即 PUT 按全量更新处理）。校验失败返回 false，err 带出原因。
inline bool courseFromJson(const Json::Value &j, Course &c, string &err)
{
    if (!j.isObject())
    {
        err = "请求体必须是 JSON 对象";
        return false;
    }

    if (j.isMember("courseName") && !j["courseName"].isNull())
    {
        if (!j["courseName"].isString() || j["courseName"].asString().empty())
        {
            err = "courseName 必须为非空字符串";
            return false;
        }
        c.courseName = j["courseName"].asString();
        if (c.courseName.size() > 100)
        {
            err = "courseName 长度不能超过 100";
            return false;
        }
    }
    else
    {
        err = "缺少 courseName";
        return false;
    }

    if (j.isMember("credit") && !j["credit"].isNull())
    {
        if (!j["credit"].isNumeric())
        {
            err = "credit 必须为数字";
            return false;
        }
        c.credit = j["credit"].asDouble();
        if (c.credit < 0.5 || c.credit > 10.0)
        {
            err = "credit(学分)必须在 0.5~10 之间";
            return false;
        }
    }
    else
    {
        err = "缺少 credit";
        return false;
    }

    if (j.isMember("teacherId"))
    {
        if (j["teacherId"].isNull())
            c.teacherId = 0;
        else if (j["teacherId"].isIntegral())
            c.teacherId = j["teacherId"].asInt64();
        else
        {
            err = "teacherId 必须为整数";
            return false;
        }
    }

    if (j.isMember("classId"))
    {
        if (j["classId"].isNull())
            c.classId = 0;
        else if (j["classId"].isIntegral())
            c.classId = j["classId"].asInt64();
        else
        {
            err = "classId 必须为整数";
            return false;
        }
    }

    if (j.isMember("semester") && j["semester"].isString())
        c.semester = j["semester"].asString();

    if (j.isMember("courseTime") && j["courseTime"].isString())
        c.courseTime = j["courseTime"].asString();

    if (j.isMember("room") && j["room"].isString())
    {
        c.room = j["room"].asString();
        if (c.room.size() > 100)
        {
            err = "room(上课地点)长度不能超过 100";
            return false;
        }
    }

    if (j.isMember("status") && !j["status"].isNull())
    {
        if (!j["status"].isIntegral())
        {
            err = "status(课程状态)必须为整数 0/1/2";
            return false;
        }
        c.status = j["status"].asInt();
        if (c.status < 0 || c.status > 2)
        {
            err = "status(课程状态)只能是 0 未开课、1 开课中、2 已结课";
            return false;
        }
    }

    return true;
}

inline Json::Value courseToJson(const Course &c)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(c.id);
    j["courseName"] = c.courseName;
    j["credit"] = c.credit;
    j["teacherId"] = static_cast<Json::Int64>(c.teacherId);
    j["teacherName"] = c.teacherName;
    j["classId"] = static_cast<Json::Int64>(c.classId);
    j["className"] = c.className;
    j["semester"] = c.semester;
    j["courseTime"] = c.courseTime;
    j["room"] = c.room;
    j["status"] = c.status;
    j["createTime"] = c.createTime;
    return j;
}

} // namespace model