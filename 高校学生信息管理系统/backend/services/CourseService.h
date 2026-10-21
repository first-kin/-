#pragma once

#include <cstdint>
#include <string>

#include "models/Course.h"
#include "utils/Result.h"

using std::string;

namespace svc
{

// 课程业务层
//   职责：课程的增删改查、分页/关键字/学期筛选
//   数据结构：LinkedList(手写单链表, 头插) 承载列表查询结果集
class CourseService
{
public:
    // 分页列表：keyword 匹配课程名/教师名；teacherId>0 时仅看该教师课程；
    // classId>0 时仅看该班级开设的课程（学生端按本班过滤用）
    static api::Result list(const string &keyword, int page, int size,
                            int64_t teacherId, const string &semester, int64_t classId);

    static api::Result detail(int64_t id);
    static api::Result create(const model::Course &c);
    static api::Result update(int64_t id, const model::Course &c);
    static api::Result remove(int64_t id);
};

} // namespace svc