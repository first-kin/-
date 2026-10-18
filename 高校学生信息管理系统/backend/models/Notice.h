#pragma once

#include <cstdint>
#include <string>

#include <json/json.h>

using std::string;

namespace model
{

// 通知公告实体，对应 notices 表；publisherName 为联表展示字段
struct Notice
{
    int64_t id = 0;
    string title;
    string content;
    int64_t publisherId = 0;
    string publisherName;
    bool isTop = false;
    string createTime;
    string updateTime;
};

// 解析公告请求：title / content 必填；isTop 可选(默认 false)
inline bool noticeFromJson(const Json::Value &j, Notice &n, string &err)
{
    if (!j.isObject())
    {
        err = "请求体必须是 JSON 对象";
        return false;
    }

    if (!j.isMember("title") || !j["title"].isString() || j["title"].asString().empty())
    {
        err = "title 不能为空";
        return false;
    }
    n.title = j["title"].asString();
    if (n.title.size() > 200)
    {
        err = "title 长度不能超过 200";
        return false;
    }

    if (!j.isMember("content") || !j["content"].isString() || j["content"].asString().empty())
    {
        err = "content 不能为空";
        return false;
    }
    n.content = j["content"].asString();

    if (j.isMember("isTop") && !j["isTop"].isNull())
    {
        if (j["isTop"].isBool())
            n.isTop = j["isTop"].asBool();
        else if (j["isTop"].isIntegral())
            n.isTop = (j["isTop"].asInt() != 0);
        else
        {
            err = "isTop 必须为布尔值";
            return false;
        }
    }

    return true;
}

inline Json::Value noticeToJson(const Notice &n)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(n.id);
    j["title"] = n.title;
    j["content"] = n.content;
    j["publisherId"] = static_cast<Json::Int64>(n.publisherId);
    j["publisherName"] = n.publisherName;
    j["isTop"] = n.isTop;
    j["createTime"] = n.createTime;
    j["updateTime"] = n.updateTime;
    return j;
}

} // namespace model