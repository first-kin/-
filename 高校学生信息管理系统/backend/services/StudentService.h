#pragma once
#include <cstdint>
#include <string>

#include <json/json.h>

#include "utils/Result.h"

using std::string;

namespace svc
{

// 学生业务层（接口文档 §5）
//   数据结构约定：
//     - LinkedList(手写单链表) 暂存查询结果行，头插后 forEach 倒序回填；
//     - SeqList(手写顺序表) 作为分页缓冲，按下标切片组装 JSON。
//   selfTeacherUserId：非 admin 访问时传入当前教师登录 user_id，用于限定
//   “仅任教班级”；admin 传 0 表示不限。
class StudentService
{
public:
    // 学生列表（分页 + keyword 按学号/姓名/班级模糊搜索 + classId 筛选）
    static api::Result list(int64_t selfTeacherUserId, const string &keyword,
                            int64_t classId, int page, int size);

    // 学生详情
    static api::Result detail(int64_t id, int64_t selfTeacherUserId);

    // 新增学生：事务内同步创建登录账号(users) + 学生档案(students)，
    // 用户名=学号，初始密码=学号后 6 位（BCrypt 散列）
    static api::Result create(const Json::Value &body);

    // 编辑学生：学号不可修改；空串可空列写入 NULL；affectedRows=0 视为不存在
    static api::Result update(int64_t id, const Json::Value &body);

    // 逻辑删除（is_deleted=1）
    static api::Result remove(int64_t id);

    // CSV 批量导入：逐行校验学号重复/性别/班级，返回 total/success/failed/errors
    static api::Result importCsv(const string &csvText);

    // 导出：data.content 为带 BOM 的 CSV 文本，data.filename 为下载文件名
    static api::Result exportCsv(int64_t selfTeacherUserId, const string &keyword,
                                 int64_t classId);
};

} // namespace svc