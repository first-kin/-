#pragma once
#include <cstdint>
#include <string>

#include <json/json.h>

#include "utils/Result.h"

using std::string;

namespace svc
{

// 教师业务层（接口文档 §7）
//   数据结构约定：
//     - SeqList(手写顺序表) 作为列表分页缓冲，按下标组装 JSON；
//     - MyStack(手写栈) 在删除前收集该教师任教课程名，pop 出栈倒序拼接
//       进错误提示，保证 LIFO 收集顺序与查询顺序可验证。
//   selfTeacherUserId：teacher 角色仅可查看本人；admin 传 0 表示不限。
class TeacherService
{
public:
    // 教师列表（分页 + keyword 按工号/姓名模糊搜索 + deptId 筛选）
    static api::Result list(int64_t selfTeacherUserId, const string &keyword,
                            int64_t deptId, int page, int size);

    // 教师详情
    static api::Result detail(int64_t id, int64_t selfTeacherUserId);

    // 新增教师：事务内同步创建登录账号(users) + 教师档案(teachers)，
    // 用户名=工号，初始密码=工号后 6 位（BCrypt 散列）
    static api::Result create(const Json::Value &body);

    // 编辑教师：工号不可修改
    static api::Result update(int64_t id, const Json::Value &body);

    // 逻辑删除；仍为班主任或存在任教课程时返回 1007
    static api::Result remove(int64_t id);
};

} // namespace svc