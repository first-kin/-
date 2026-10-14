#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/orm/DbClient.h>

namespace api
{

// 获取默认数据库客户端（在 config/app.json 中配置，名称为 default）
inline drogon::orm::DbClientPtr db()
{
    return drogon::app().getDbClient();
}

} // namespace api