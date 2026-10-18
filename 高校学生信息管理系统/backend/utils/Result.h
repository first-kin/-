#pragma once

#include <string>

#include <json/json.h>

using std::string;

namespace api
{

// 统一业务错误码：HTTP 状态码恒为 200，业务结果以 code 区分
// 与 docs/接口文档.md §1.4 错误码表一一对应
enum ErrorCode
{
    E_OK = 0,             // 成功
    E_PARAM = 1001,       // 参数校验失败
    E_LOGIN_FAILED = 1002,// 用户名或密码错误
    E_TOKEN_INVALID = 1003, // Token 无效或过期/未登录
    E_UNAUTHORIZED = 1003,  // 未登录（别名，同 E_TOKEN_INVALID）
    E_FORBIDDEN = 1004,   // 无权限访问该资源
    E_NOT_FOUND = 1005,   // 资源不存在
    E_DUPLICATE = 1006,   // 数据重复（学号/工号/课程名唯一冲突）
    E_CONFLICT = 1006,    // 资源冲突（别名，同 E_DUPLICATE）
    E_BIZ_RULE = 1007,    // 业务规则冲突（重名、被引用等）
    E_LOCKED = 2001,      // 账号被锁定（连续输错 5 次锁定 10 分钟）
    E_DB = 5000,          // 数据库错误
    E_INTERNAL = 5000     // 服务器内部错误
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