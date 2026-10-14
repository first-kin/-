#pragma once

#include <string>

#include <json/json.h>

using namespace std;

namespace api
{

// 统一业务错误码：HTTP 状态码恒为 200，业务结果以 code 区分
enum ErrorCode
{
    E_OK = 0,            // 成功
    E_PARAM = 1001,      // 参数错误
    E_NOT_FOUND = 1004,  // 资源不存在
    E_CONFLICT = 1009,   // 资源冲突(重名、被引用等)
    E_UNAUTHORIZED = 401,// 未登录
    E_FORBIDDEN = 403,   // 无权限
    E_DB = 5000,         // 数据库错误
    E_INTERNAL = 5001    // 服务器内部错误
};

// 业务层统一返回体：code + msg + data
struct Result
{
    int code = E_OK;
    string msg = "ok";
    Json::Value data = Json::Value(Json::nullValue);

    bool ok() const { return code == E_OK; }

    static Result success(const Json::Value &d, const string &m = "ok")
    {
        Result r;
        r.code = E_OK;
        r.msg = m;
        r.data = d;
        return r;
    }

    static Result error(int c, const string &m)
    {
        Result r;
        r.code = c;
        r.msg = m;
        r.data = Json::Value(Json::nullValue);
        return r;
    }
};

} // namespace api