#pragma once
#include <cstdint>
#include <string>

#include <json/json.h>

#include "utils/Result.h"

using std::string;

namespace svc
{

// 院系与班级业务层（接口文档 §6）
//   数据结构约定：
//     - MyQueue(手写循环队列) 暂存院系列表行，FIFO 出队组装 JSON，天然保序；
//     - LinkedList(手写单链表) 暂存班级查询行，头插后 forEach 倒序回填。
//   院系删除前校验教师/专业引用，班级删除前校验学生引用（1007 规则冲突）。
class DeptService
{
public:
    // 院系列表（登录用户可查，非分页）
    static api::Result deptList();
    // 专业列表（用于班级选择所属专业，非分页）
    static api::Result majorList();
    // 新增/编辑/删除院系（admin）
    static api::Result deptCreate(const Json::Value &body);
    static api::Result deptUpdate(int64_t id, const Json::Value &body);
    static api::Result deptRemove(int64_t id);

    // 班级列表（可选 majorId 筛选，非分页）
    static api::Result classList(int64_t majorId);
    // 新增/编辑/删除班级（admin）
    static api::Result classCreate(const Json::Value &body);
    static api::Result classUpdate(int64_t id, const Json::Value &body);
    static api::Result classRemove(int64_t id);
};

} // namespace svc
