#include "services/ScoreService.h"

#include <exception>
#include <mutex>
#include <string>
#include <vector>

#include "structures/Mystack.h"
#include "structures/SeqList.h"
#include "utils/Db.h"
using namespace std;

namespace svc
{

namespace
{

// 成绩修改历史栈（LIFO），跨请求共享，需加锁保护
mutex g_historyMutex;
MyStack<ScoreChange> g_history;

// 保留两位小数（不用 <cmath> 的 round，避免与 using namespace std 引入的全局 round 冲突）
double round2(double v)
{
    return static_cast<double>(static_cast<long long>(v * 100.0 + 0.5)) / 100.0;
}

const char *kScoreSelect =
    "SELECT sc.id, sc.student_id, s.student_no, s.name AS student_name, "
    "       sc.course_id, c.course_name, sc.score, sc.status, "
    "       DATE_FORMAT(sc.exam_time, '%Y-%m-%d') AS exam_time "
    "FROM scores sc "
    "JOIN students s ON sc.student_id = s.id "
    "JOIN courses c ON sc.course_id = c.id ";

model::Score rowToScore(const drogon::orm::Row &row)
{
    model::Score s;
    s.id = row["id"].as<int64_t>();
    s.studentId = row["student_id"].as<int64_t>();
    s.studentNo = row["student_no"].isNull() ? "" : row["student_no"].as<string>();
    s.studentName = row["student_name"].isNull() ? "" : row["student_name"].as<string>();
    s.courseId = row["course_id"].as<int64_t>();
    s.courseName = row["course_name"].isNull() ? "" : row["course_name"].as<string>();
    s.hasScore = !row["score"].isNull();
    s.score = s.hasScore ? row["score"].as<double>() : 0.0;
    s.status = row["status"].as<int>();
    s.examTime = row["exam_time"].isNull() ? "" : row["exam_time"].as<string>();
    return s;
}

} // namespace

api::Result ScoreService::list(int64_t courseId, int64_t studentId, const string &keyword,
                               int page, int size, int64_t teacherScope)
{
    if (page < 1)
        page = 1;
    if (size < 1 || size > 100)
        size = 10;
    const int offset = (page - 1) * size;
    const string pattern = "%" + keyword + "%";

    const char *kWhere =
        "WHERE s.is_deleted = 0 AND c.is_deleted = 0 "
        "AND ($1 = 0 OR sc.course_id = $2) "
        "AND ($3 = 0 OR sc.student_id = $4) "
        "AND ($5 = '' OR s.name LIKE $6 OR s.student_no LIKE $7) "
        "AND ($8 = 0 OR c.teacher_id = $9) ";

    try
    {
        auto client = api::db();

        auto countRows = client->execSqlSync(
            string("SELECT COUNT(*) AS total FROM scores sc "
                        "JOIN students s ON sc.student_id = s.id "
                        "JOIN courses c ON sc.course_id = c.id ") +
                kWhere,
            courseId, courseId, studentId, studentId, keyword, pattern, pattern,
            teacherScope, teacherScope);
        const int64_t total = countRows[0]["total"].as<int64_t>();

        auto rows = client->execSqlSync(
            string(kScoreSelect) + kWhere + "ORDER BY sc.id DESC LIMIT $10 OFFSET $11",
            courseId, courseId, studentId, studentId, keyword, pattern, pattern,
            teacherScope, teacherScope, size, offset);

        Json::Value arr(Json::arrayValue);
        for (const auto &row : rows)
            arr.append(model::scoreToJson(rowToScore(row)));

        Json::Value data(Json::objectValue);
        data["total"] = static_cast<Json::Int64>(total);
        data["page"] = page;
        data["size"] = size;
        data["list"] = arr;
        return api::Result::success(data);
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询成绩列表失败: ") + e.what());
    }
}

api::Result ScoreService::myScores(int64_t userId, const string &semester)
{
    const char *kSql =
        "SELECT sc.id, sc.student_id, s.student_no, s.name AS student_name, "
        "       sc.course_id, c.course_name, sc.score, sc.status, "
        "       DATE_FORMAT(sc.exam_time, '%Y-%m-%d') AS exam_time "
        "FROM scores sc "
        "JOIN students s ON sc.student_id = s.id "
        "JOIN courses c ON sc.course_id = c.id "
        "WHERE s.user_id = $1 AND s.is_deleted = 0 AND c.is_deleted = 0 "
        "AND ($2 = '' OR IFNULL(c.semester, '') = $3) "
        "ORDER BY c.semester DESC, sc.id DESC";

    try
    {
        auto rows = api::db()->execSqlSync(kSql, userId, semester, semester);

        Json::Value arr(Json::arrayValue);
        for (const auto &row : rows)
            arr.append(model::scoreToJson(rowToScore(row)));

        Json::Value data(Json::objectValue);
        data["total"] = static_cast<Json::Int64>(rows.size());
        data["list"] = arr;
        return api::Result::success(data);
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询本人成绩失败: ") + e.what());
    }
}

api::Result ScoreService::save(const model::Score &s, int64_t teacherScope)
{
    try
    {
        auto client = api::db();

        auto course = client->execSqlSync(
            "SELECT teacher_id FROM courses WHERE id = $1 AND is_deleted = 0", s.courseId);
        if (course.size() == 0)
            return api::Result::error(api::E_PARAM, "课程不存在");

        if (teacherScope > 0)
        {
            const int64_t tid =
                course[0]["teacher_id"].isNull() ? 0 : course[0]["teacher_id"].as<int64_t>();
            if (tid != teacherScope)
                return api::Result::error(api::E_FORBIDDEN, "无权录入该课程的成绩");
        }

        auto stu = client->execSqlSync(
            "SELECT id FROM students WHERE id = $1 AND is_deleted = 0", s.studentId);
        if (stu.size() == 0)
            return api::Result::error(api::E_PARAM, "学生不存在");

        // 变更前状态：用于留痕与撤销
        auto old = client->execSqlSync(
            "SELECT id, score, status FROM scores WHERE student_id = $1 AND course_id = $2",
            s.studentId, s.courseId);

        ScoreChange rec;
        rec.studentId = s.studentId;
        rec.courseId = s.courseId;
        rec.existed = (old.size() > 0);
        if (rec.existed)
        {
            rec.scoreId = old[0]["id"].as<int64_t>();
            rec.hadScore = !old[0]["score"].isNull();
            rec.oldScore = rec.hadScore ? old[0]["score"].as<double>() : 0.0;
            rec.oldStatus = old[0]["status"].as<int>();
        }

        // exam_time 为空时写 NULL，故使用 IF() 表达式
        if (s.hasScore)
        {
            client->execSqlSync(
                "INSERT INTO scores (student_id, course_id, score, status, exam_time) "
                "VALUES ($1, $2, $3, $4, IF($5 = '', NULL, $6)) "
                "ON DUPLICATE KEY UPDATE score = VALUES(score), status = VALUES(status), "
                "exam_time = VALUES(exam_time)",
                s.studentId, s.courseId, s.score, s.status, s.examTime, s.examTime);
        }
        else
        {
            client->execSqlSync(
                "INSERT INTO scores (student_id, course_id, score, status, exam_time) "
                "VALUES ($1, $2, NULL, $3, IF($4 = '', NULL, $5)) "
                "ON DUPLICATE KEY UPDATE score = NULL, status = VALUES(status), "
                "exam_time = VALUES(exam_time)",
                s.studentId, s.courseId, s.status, s.examTime, s.examTime);
        }

        // 压入修改历史栈（LIFO）
        {
            lock_guard<mutex> lock(g_historyMutex);
            g_history.push(rec);
        }

        return api::Result::success(Json::Value(Json::objectValue),
                                    rec.existed ? "成绩修改成功" : "成绩录入成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("成绩保存失败: ") + e.what());
    }
}

api::Result ScoreService::batchSave(int64_t courseId, const vector<model::Score> &items,
                                    int64_t teacherScope)
{
    if (courseId <= 0)
        return api::Result::error(api::E_PARAM, "courseId 非法");
    if (items.empty())
        return api::Result::error(api::E_PARAM, "成绩列表为空");

    int okCount = 0;
    int failCount = 0;
    Json::Value errors(Json::arrayValue);

    for (const auto &item : items)
    {
        model::Score s = item;
        s.courseId = courseId; // 以请求中的 courseId 为准
        api::Result r = save(s, teacherScope);
        if (r.ok())
        {
            ++okCount;
        }
        else
        {
            ++failCount;
            Json::Value e(Json::objectValue);
            e["studentId"] = static_cast<Json::Int64>(s.studentId);
            e["msg"] = r.msg;
            errors.append(e);
        }
    }

    Json::Value data(Json::objectValue);
    data["success"] = okCount;
    data["failed"] = failCount;
    data["errors"] = errors;
    return api::Result::success(data, failCount == 0 ? "批量录入完成" : "批量录入存在失败项");
}

api::Result ScoreService::statistics(int64_t courseId, int64_t teacherScope)
{
    try
    {
        auto client = api::db();

        auto course = client->execSqlSync(
            "SELECT course_name, teacher_id FROM courses WHERE id = $1 AND is_deleted = 0",
            courseId);
        if (course.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "课程不存在");

        if (teacherScope > 0)
        {
            const int64_t tid =
                course[0]["teacher_id"].isNull() ? 0 : course[0]["teacher_id"].as<int64_t>();
            if (tid != teacherScope)
                return api::Result::error(api::E_FORBIDDEN, "无权查看该课程统计");
        }

        auto agg = client->execSqlSync(
            "SELECT COUNT(*) AS total, "
            "       SUM(CASE WHEN score IS NOT NULL THEN 1 ELSE 0 END) AS scored, "
            "       AVG(score) AS avg_score, MAX(score) AS max_score, MIN(score) AS min_score, "
            "       SUM(CASE WHEN score >= 60 THEN 1 ELSE 0 END) AS pass_cnt, "
            "       SUM(CASE WHEN status = 1 THEN 1 ELSE 0 END) AS absent_cnt, "
            "       SUM(CASE WHEN status = 2 THEN 1 ELSE 0 END) AS deferred_cnt "
            "FROM scores WHERE course_id = $1",
            courseId);

        const auto &row = agg[0];
        const int64_t total = row["total"].as<int64_t>();
        const int64_t scored = row["scored"].isNull() ? 0 : row["scored"].as<int64_t>();
        const int64_t passCnt = row["pass_cnt"].isNull() ? 0 : row["pass_cnt"].as<int64_t>();
        const double avg = row["avg_score"].isNull() ? 0.0 : row["avg_score"].as<double>();
        const double maxScore = row["max_score"].isNull() ? 0.0 : row["max_score"].as<double>();
        const double minScore = row["min_score"].isNull() ? 0.0 : row["min_score"].as<double>();

        // 分数段分布：手写顺序表 SeqList<int> 作 5 个计数桶
        SeqList<int> bins;
        for (int i = 0; i < 5; ++i)
            bins.pushBack(0);

        auto scoreRows = client->execSqlSync(
            "SELECT score FROM scores WHERE course_id = $1 AND score IS NOT NULL", courseId);
        for (const auto &r : scoreRows)
        {
            const double v = r["score"].as<double>();
            int idx;
            if (v < 60)
                idx = 0;
            else if (v < 70)
                idx = 1;
            else if (v < 80)
                idx = 2;
            else if (v < 90)
                idx = 3;
            else
                idx = 4;
            bins[idx] += 1;
        }

        const char *labels[5] = {"<60", "60-69", "70-79", "80-89", "90-100"};
        Json::Value dist(Json::arrayValue);
        for (size_t i = 0; i < bins.size(); ++i)
        {
            Json::Value item(Json::objectValue);
            item["range"] = labels[i];
            item["count"] = bins[i];
            dist.append(item);
        }

        Json::Value data(Json::objectValue);
        data["courseId"] = static_cast<Json::Int64>(courseId);
        data["courseName"] = course[0]["course_name"].as<string>();
        data["total"] = static_cast<Json::Int64>(total);
        data["scored"] = static_cast<Json::Int64>(scored);
        data["absent"] = row["absent_cnt"].isNull() ? 0 : row["absent_cnt"].as<int>();
        data["deferred"] = row["deferred_cnt"].isNull() ? 0 : row["deferred_cnt"].as<int>();
        data["avg"] = scored > 0 ? round2(avg) : 0.0;
        data["max"] = scored > 0 ? maxScore : 0.0;
        data["min"] = scored > 0 ? minScore : 0.0;
        data["passRate"] =
            scored > 0 ? round2(static_cast<double>(passCnt) * 100.0 / scored) : 0.0;
        data["distribution"] = dist;
        return api::Result::success(data);
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("成绩统计失败: ") + e.what());
    }
}

api::Result ScoreService::undoLast()
{
    ScoreChange rec;
    {
        lock_guard<mutex> lock(g_historyMutex);
        if (g_history.empty())
            return api::Result::error(api::E_NOT_FOUND, "没有可撤销的成绩修改记录");
        rec = g_history.top();
        g_history.pop();
    }

    try
    {
        auto client = api::db();
        if (rec.existed)
        {
            if (rec.hadScore)
                client->execSqlSync(
                    "UPDATE scores SET score = $1, status = $2 WHERE id = $3",
                    rec.oldScore, rec.oldStatus, rec.scoreId);
            else
                client->execSqlSync(
                    "UPDATE scores SET score = NULL, status = $1 WHERE id = $2",
                    rec.oldStatus, rec.scoreId);
        }
        else
        {
            client->execSqlSync(
                "DELETE FROM scores WHERE student_id = $1 AND course_id = $2",
                rec.studentId, rec.courseId);
        }

        Json::Value data(Json::objectValue);
        data["studentId"] = static_cast<Json::Int64>(rec.studentId);
        data["courseId"] = static_cast<Json::Int64>(rec.courseId);
        return api::Result::success(data, "已撤销最近一次成绩修改");
    }
    catch (const exception &e)
    {
        // 撤销失败则把记录放回栈顶，保证历史不丢
        {
            lock_guard<mutex> lock(g_historyMutex);
            g_history.push(rec);
        }
        return api::Result::error(api::E_DB, string("撤销失败: ") + e.what());
    }
}

api::Result ScoreService::history(int limit)
{
    if (limit < 1 || limit > 50)
        limit = 10;

    vector<ScoreChange> snapshot;
    size_t total = 0;
    {
        lock_guard<mutex> lock(g_historyMutex);
        total = g_history.size();
        MyStack<ScoreChange> tmp;
        while (!g_history.empty() && snapshot.size() < static_cast<size_t>(limit))
        {
            snapshot.push_back(g_history.top()); // 最近一次修改排在前面
            tmp.push(g_history.top());
            g_history.pop();
        }
        // 原样还栈
        while (!tmp.empty())
        {
            g_history.push(tmp.top());
            tmp.pop();
        }
    }

    Json::Value arr(Json::arrayValue);
    for (const auto &c : snapshot)
    {
        Json::Value item(Json::objectValue);
        item["studentId"] = static_cast<Json::Int64>(c.studentId);
        item["courseId"] = static_cast<Json::Int64>(c.courseId);
        item["existed"] = c.existed;
        item["oldScore"] = c.hadScore ? Json::Value(c.oldScore) : Json::Value(Json::nullValue);
        item["oldStatus"] = c.oldStatus;
        arr.append(item);
    }

    Json::Value data(Json::objectValue);
    data["total"] = static_cast<Json::Int64>(total);
    data["list"] = arr;
    return api::Result::success(data);
}

} // namespace svc