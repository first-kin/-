#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "models/Score.h"
#include "utils/Result.h"
using namespace std;

namespace svc
{

// 一次成绩变更的记录，用于「修改留痕 + 撤销」
struct ScoreChange
{
    int64_t scoreId = 0;
    int64_t studentId = 0;
    int64_t courseId = 0;
    bool existed = false;  // 变更前是否已有成绩记录
    bool hadScore = false; // 变更前是否有分数
    double oldScore = 0.0;
    int oldStatus = 0;
};

// 成绩业务层
//   职责：成绩录入/修改、列表查询、学生查本人、成绩统计
//   数据结构：
//     - MyStack(手写栈, LIFO) 记录成绩修改历史，支持「撤销最近一次修改」；
//     - SeqList(手写顺序表)  作为分数段分布统计的计数桶。
//   说明：修改历史保存在进程内存中，仅用于课设演示数据结构应用；
//         生产环境应落库为 scores_history 表。
class ScoreService
{
public:
    // 成绩列表：按课程/学生/关键字筛选；teacherScope > 0 时仅限该教师任教课程
    static api::Result list(int64_t courseId, int64_t studentId, const string &keyword,
                            int page, int size, int64_t teacherScope);

    // 学生按 user_id 查本人成绩，可按学期筛选
    static api::Result myScores(int64_t userId, const string &semester);

    // 录入或修改单条成绩（存在则更新）
    static api::Result save(const model::Score &s, int64_t teacherScope);

    // 批量录入（逐条保存，返回成功/失败明细）
    static api::Result batchSave(int64_t courseId, const vector<model::Score> &items,
                                 int64_t teacherScope);

    // 单科成绩统计：平均分/最高/最低/及格率/分数段分布
    static api::Result statistics(int64_t courseId, int64_t teacherScope);

    // 撤销最近一次成绩修改（栈顶出栈并恢复）
    static api::Result undoLast();

    // 查看最近的成绩修改记录（只读，不改变栈）
    static api::Result history(int limit);
};

} // namespace svc