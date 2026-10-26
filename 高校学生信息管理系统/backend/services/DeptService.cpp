#include "services/DeptService.h"

#include <exception>
#include <string>

#include "structures/LinkedList.h"
#include "structures/MyQueue.h"
#include "utils/Db.h"
#include "utils/HttpUtil.h"
using namespace std;

namespace svc
{
namespace
{

// 一行院系记录
struct DeptRow
{
    int64_t id = 0;
    string deptName;
};

struct MajorRow
{
    int64_t id = 0;
    int64_t deptId = 0;
    string majorName;
    string deptName;
};

// 一行班级记录（headTeacherId = 0 表示数据库 NULL）
struct ClassRow
{
    int64_t id = 0;
    string className;
    int64_t majorId = 0;
    string majorName;
    string deptName;
    int grade = 0;
    int64_t headTeacherId = 0;
};

Json::Value deptToJson(const DeptRow &d)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(d.id);
    j["deptName"] = d.deptName;
    return j;
}

Json::Value majorToJson(const MajorRow &m)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(m.id);
    j["majorName"] = m.majorName;
    j["deptId"] = static_cast<Json::Int64>(m.deptId);
    j["deptName"] = m.deptName;
    return j;
}

Json::Value classToJson(const ClassRow &c)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(c.id);
    j["className"] = c.className;
    j["majorId"] = static_cast<Json::Int64>(c.majorId);
    j["majorName"] = c.majorName;
    j["deptName"] = c.deptName;
    j["grade"] = c.grade;
    if (c.headTeacherId > 0)
        j["headTeacherId"] = static_cast<Json::Int64>(c.headTeacherId);
    return j;
}

// 班主任可选参数：传 0 或缺省写 NULL
int64_t optHeadTeacherId(const Json::Value &body)
{
    if (body.isMember("headTeacherId") &&
        (body["headTeacherId"].isInt() || body["headTeacherId"].isInt64() ||
         body["headTeacherId"].isUInt()))
        return body["headTeacherId"].asInt64();
    return 0;
}

} // namespace

// ---------- 院系 ----------

api::Result DeptService::deptList()
{
    try
    {
        auto rows = api::db()->execSqlSync(
            "SELECT id, dept_name FROM departments ORDER BY id");

        // MyQueue(FIFO) 暂存院系行，出队顺序与查询顺序一致，天然保序
        MyQueue<DeptRow> q;
        for (const auto &row : rows)
        {
            DeptRow d;
            d.id = row["id"].as<int64_t>();
            d.deptName = row["dept_name"].as<string>();
            q.push(d);
        }

        Json::Value list(Json::arrayValue);
        while (!q.empty())
        {
            list.append(deptToJson(q.front()));
            q.pop();
        }
        return util::okData(list);
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询院系列表失败: ") + e.what());
    }
}

api::Result DeptService::majorList()
{
    try
    {
        auto rows = api::db()->execSqlSync(
            "SELECT m.id, m.major_name, m.dept_id, d.dept_name "
            "FROM majors m JOIN departments d ON d.id = m.dept_id "
            "ORDER BY d.id, m.id");

        Json::Value list(Json::arrayValue);
        for (const auto &row : rows)
        {
            MajorRow major;
            major.id = row["id"].as<int64_t>();
            major.majorName = row["major_name"].as<string>();
            major.deptId = row["dept_id"].as<int64_t>();
            major.deptName = row["dept_name"].as<string>();
            list.append(majorToJson(major));
        }
        return util::okData(list);
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询专业列表失败: ") + e.what());
    }
}

api::Result DeptService::deptCreate(const Json::Value &body)
{
    string deptName, err;
    if (!util::requireStr(body, "deptName", deptName, err))
        return util::fail(util::kParam, err);

    try
    {
        auto client = api::db();

        auto dup = client->execSqlSync(
            "SELECT id FROM departments WHERE dept_name = ? LIMIT 1", deptName);
        if (!dup.empty())
            return util::fail(util::kDuplicate, "院系名称已存在");

        auto res = client->execSqlSync(
            "INSERT INTO departments (dept_name) VALUES (?)", deptName);

        Json::Value data(Json::objectValue);
        data["id"] = static_cast<Json::Int64>(res.insertId());
        return util::okData(data, "新增成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("新增院系失败: ") + e.what());
    }
}

api::Result DeptService::deptUpdate(int64_t id, const Json::Value &body)
{
    string deptName, err;
    if (!util::requireStr(body, "deptName", deptName, err))
        return util::fail(util::kParam, err);

    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM departments WHERE id = ? LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "院系不存在");

        auto dup = client->execSqlSync(
            "SELECT id FROM departments WHERE dept_name = ? AND id != ? LIMIT 1",
            deptName, id);
        if (!dup.empty())
            return util::fail(util::kDuplicate, "院系名称已存在");

        auto res = client->execSqlSync(
            "UPDATE departments SET dept_name = ? WHERE id = ?", id, deptName);
        if (res.affectedRows() == 0)
            return util::fail(util::kNotFound, "院系不存在");
        return util::okMsg("保存成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("编辑院系失败: ") + e.what());
    }
}

api::Result DeptService::deptRemove(int64_t id)
{
    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM departments WHERE id = ? LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "院系不存在");

        // 有教师挂在该院系下则禁止删除
        auto tea = client->execSqlSync(
            "SELECT id FROM teachers WHERE dept_id = ? AND is_deleted = 0 LIMIT 1", id);
        if (!tea.empty())
            return util::fail(util::kRuleConflict, "该院系仍有关联教师，无法删除");

        // 有专业挂在该院系下则禁止删除（专业表无逻辑删除列，直接查引用）
        auto maj = client->execSqlSync(
            "SELECT id FROM majors WHERE dept_id = ? LIMIT 1", id);
        if (!maj.empty())
            return util::fail(util::kRuleConflict, "该院系仍有关联专业，无法删除");

        auto res = client->execSqlSync(
            "DELETE FROM departments WHERE id = ?", id);
        if (res.affectedRows() == 0)
            return util::fail(util::kNotFound, "院系不存在");
        return util::okMsg("删除成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("删除院系失败: ") + e.what());
    }
}

// ---------- 班级 ----------

api::Result DeptService::classList(int64_t majorId)
{
    try
    {
        auto rows = api::db()->execSqlSync(
            "SELECT c.id, c.class_name, c.major_id, m.major_name, d.dept_name, c.grade, c.head_teacher_id "
            "FROM classes c "
            "JOIN majors m ON m.id = c.major_id "
            "JOIN departments d ON d.id = m.dept_id "
            "WHERE (? = 0 OR c.major_id = ?) ORDER BY c.id",
            majorId, majorId);

        // LinkedList 头插暂存班级行，forEach 倒序回填还原查询顺序
        Json::Value list(Json::arrayValue);
        LinkedList<ClassRow> tmp;
        for (const auto &row : rows)
        {
            ClassRow c;
            c.id = row["id"].as<int64_t>();
            c.className = row["class_name"].as<string>();
            c.majorId = row["major_id"].as<int64_t>();
            c.majorName = row["major_name"].as<string>();
            c.deptName = row["dept_name"].as<string>();
            c.grade = row["grade"].as<int>();
            c.headTeacherId = row["head_teacher_id"].isNull()
                                  ? 0
                                  : row["head_teacher_id"].as<int64_t>();
            tmp.pushFront(c);
        }
        tmp.forEach([&list](const ClassRow &c) { list.append(classToJson(c)); });

        return util::okData(list);
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询班级列表失败: ") + e.what());
    }
}

api::Result DeptService::classCreate(const Json::Value &body)
{
    string className, err;
    int64_t majorId = 0, grade = 0;
    if (!util::requireStr(body, "className", className, err) ||
        !util::requireInt(body, "majorId", majorId, err) ||
        !util::requireInt(body, "grade", grade, err))
        return util::fail(util::kParam, err);

    const int64_t headTeacherId = optHeadTeacherId(body);

    try
    {
        auto client = api::db();

        auto dup = client->execSqlSync(
            "SELECT id FROM classes WHERE class_name = ? AND grade = ? LIMIT 1",
            className, grade);
        if (!dup.empty())
            return util::fail(util::kDuplicate, "同年级班级名称已存在");

        auto major = client->execSqlSync(
            "SELECT id FROM majors WHERE id = ? LIMIT 1", majorId);
        if (major.empty())
            return util::fail(util::kNotFound, "专业不存在");

        if (headTeacherId > 0)
        {
            auto head = client->execSqlSync(
                "SELECT id FROM teachers WHERE id = ? AND is_deleted = 0 LIMIT 1",
                headTeacherId);
            if (head.empty())
                return util::fail(util::kNotFound, "班主任不存在");
        }

        auto res = client->execSqlSync(
            "INSERT INTO classes (class_name, major_id, grade, head_teacher_id) "
            "VALUES (?, ?, ?, NULLIF(?, 0))",
            className, majorId, grade, headTeacherId);

        Json::Value data(Json::objectValue);
        data["id"] = static_cast<Json::Int64>(res.insertId());
        return util::okData(data, "新增成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("新增班级失败: ") + e.what());
    }
}

api::Result DeptService::classUpdate(int64_t id, const Json::Value &body)
{
    string className, err;
    int64_t majorId = 0, grade = 0;
    if (!util::requireStr(body, "className", className, err) ||
        !util::requireInt(body, "majorId", majorId, err) ||
        !util::requireInt(body, "grade", grade, err))
        return util::fail(util::kParam, err);

    const int64_t headTeacherId = optHeadTeacherId(body);

    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM classes WHERE id = ? LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "班级不存在");

        auto dup = client->execSqlSync(
            "SELECT id FROM classes WHERE class_name = ? AND grade = ? AND id != ? LIMIT 1",
            className, grade, id);
        if (!dup.empty())
            return util::fail(util::kDuplicate, "同年级班级名称已存在");

        auto major = client->execSqlSync(
            "SELECT id FROM majors WHERE id = ? LIMIT 1", majorId);
        if (major.empty())
            return util::fail(util::kNotFound, "专业不存在");

        if (headTeacherId > 0)
        {
            auto head = client->execSqlSync(
                "SELECT id FROM teachers WHERE id = ? AND is_deleted = 0 LIMIT 1",
                headTeacherId);
            if (head.empty())
                return util::fail(util::kNotFound, "班主任不存在");
        }

        auto res = client->execSqlSync(
            "UPDATE classes SET class_name = ?, major_id = ?, grade = ?, "
            "head_teacher_id = NULLIF(?, 0) WHERE id = ?",
            className, majorId, grade, headTeacherId, id);
        if (res.affectedRows() == 0)
            return util::fail(util::kNotFound, "班级不存在");
        return util::okMsg("保存成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("编辑班级失败: ") + e.what());
    }
}

api::Result DeptService::classRemove(int64_t id)
{
    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM classes WHERE id = ? LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "班级不存在");

        // 班级下仍有在读学生则禁止删除
        auto stu = client->execSqlSync(
            "SELECT id FROM students WHERE class_id = ? AND is_deleted = 0 LIMIT 1", id);
        if (!stu.empty())
            return util::fail(util::kRuleConflict, "该班级仍有关联学生，无法删除");

        auto res = client->execSqlSync(
            "DELETE FROM classes WHERE id = ?", id);
        if (res.affectedRows() == 0)
            return util::fail(util::kNotFound, "班级不存在");
        return util::okMsg("删除成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("删除班级失败: ") + e.what());
    }
}

} // namespace svc
