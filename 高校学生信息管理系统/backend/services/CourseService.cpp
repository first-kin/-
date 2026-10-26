#include "services/CourseService.h"

#include <exception>

#include "structures/LinkedList.h"
#include "utils/Db.h"
using namespace std;

namespace svc
{

namespace
{

// 统一的课程查询列(含教师名联表、时间格式化)
const char *kSelectPrefix =
    "SELECT c.id, c.course_name, c.credit, c.teacher_id, t.name AS teacher_name, "
    "       c.class_id, cl.class_name, "
    "       c.semester, c.course_time, c.room, c.status, "
    "       DATE_FORMAT(c.create_time, '%Y-%m-%d %H:%i:%s') AS create_time "
    "FROM courses c LEFT JOIN teachers t ON c.teacher_id = t.id "
    "LEFT JOIN classes cl ON c.class_id = cl.id ";

// 筛选条件(占位符不重复使用)：keyword、semester、teacherId、classId、studentId。
const char *kWhereClause =
    "WHERE c.is_deleted = 0 "
    "AND (? = '' OR c.course_name LIKE ? OR IFNULL(t.name, '') LIKE ?) "
    "AND (? = '' OR IFNULL(c.semester, '') = ?) "
    "AND (? = 0 OR c.teacher_id = ?) "
    "AND (? = 0 OR c.class_id = ?) "
    "AND (? = 0 OR c.id IN (SELECT sc.course_id FROM student_courses sc WHERE sc.student_id = ?)) ";

model::Course rowToCourse(const drogon::orm::Row &row)
{
    model::Course c;
    c.id = row["id"].as<int64_t>();
    c.courseName = row["course_name"].as<string>();
    c.credit = row["credit"].isNull() ? 0.0 : row["credit"].as<double>();
    c.teacherId = row["teacher_id"].isNull() ? 0 : row["teacher_id"].as<int64_t>();
    c.teacherName = row["teacher_name"].isNull() ? "" : row["teacher_name"].as<string>();
    c.classId = row["class_id"].isNull() ? 0 : row["class_id"].as<int64_t>();
    c.className = row["class_name"].isNull() ? "" : row["class_name"].as<string>();
    c.semester = row["semester"].isNull() ? "" : row["semester"].as<string>();
    c.courseTime = row["course_time"].isNull() ? "" : row["course_time"].as<string>();
    c.room = row["room"].isNull() ? "" : row["room"].as<string>();
    c.status = row["status"].isNull() ? 1 : row["status"].as<int>();
    c.createTime = row["create_time"].isNull() ? "" : row["create_time"].as<string>();
    return c;
}

} // namespace

api::Result CourseService::list(const string &keyword, int page, int size,
                                int64_t teacherId, const string &semester, int64_t classId,
                                int64_t studentId)
{
    if (page < 1)
        page = 1;
    if (size < 1 || size > 100)
        size = 10;
    const int offset = (page - 1) * size;
    const string pattern = "%" + keyword + "%";

    try
    {
        auto client = api::db();

        auto countRows = client->execSqlSync(
            string("SELECT COUNT(*) AS total FROM courses c "
                        "LEFT JOIN teachers t ON c.teacher_id = t.id ") +
                kWhereClause,
            keyword, pattern, pattern, semester, semester, teacherId, teacherId,
            classId, classId, studentId, studentId);
        const int64_t total = countRows[0]["total"].as<int64_t>();

        auto rows = client->execSqlSync(
            string(kSelectPrefix) + kWhereClause +
                "ORDER BY c.id DESC LIMIT ? OFFSET ?",
            keyword, pattern, pattern, semester, semester, teacherId, teacherId,
            classId, classId, studentId, studentId, size, offset);

        // 用手写单链表承载结果集(头插)，再遍历序列化
        LinkedList<model::Course> courses;
        for (const auto &row : rows)
            courses.pushFront(rowToCourse(row));

        Json::Value arr(Json::arrayValue);
        courses.forEach([&arr](const model::Course &c) { arr.append(model::courseToJson(c)); });

        Json::Value data(Json::objectValue);
        data["total"] = static_cast<Json::Int64>(total);
        data["page"] = page;
        data["size"] = size;
        data["list"] = arr;
        return api::Result::success(data);
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询课程列表失败: ") + e.what());
    }
}

api::Result CourseService::detail(int64_t id)
{
    try
    {
        auto rows = api::db()->execSqlSync(
            string(kSelectPrefix) + "WHERE c.id = ? AND c.is_deleted = 0", id);
        if (rows.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "课程不存在");
        return api::Result::success(model::courseToJson(rowToCourse(rows[0])));
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询课程失败: ") + e.what());
    }
}

api::Result CourseService::create(const model::Course &c)
{
    try
    {
        auto client = api::db();

        auto dup = client->execSqlSync(
            "SELECT id FROM courses WHERE course_name = ? AND is_deleted = 0", c.courseName);
        if (dup.size() > 0)
            return api::Result::error(api::E_CONFLICT, "课程名已存在");

        if (c.teacherId > 0)
        {
            auto t = client->execSqlSync(
                "SELECT id FROM teachers WHERE id = ? AND is_deleted = 0", c.teacherId);
            if (t.size() == 0)
                return api::Result::error(api::E_PARAM, "指定的授课教师不存在");
        }

        if (c.classId > 0)
        {
            auto cl = client->execSqlSync(
                "SELECT id FROM classes WHERE id = ?", c.classId);
            if (cl.size() == 0)
                return api::Result::error(api::E_PARAM, "指定的开设班级不存在");
        }

        // teacher_id / class_id 为外键，未指定时必须写 NULL
        auto res = client->execSqlSync(
            "INSERT INTO courses (course_name, credit, teacher_id, class_id, semester, course_time, room, status) "
            "VALUES (?, ?, NULLIF(?, 0), NULLIF(?, 0), ?, ?, ?, ?)",
            c.courseName, c.credit, c.teacherId, c.classId, c.semester, c.courseTime, c.room, c.status);

        Json::Value data(Json::objectValue);
        data["id"] = static_cast<Json::Int64>(res.insertId());
        return api::Result::success(data, "新增课程成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("新增课程失败: ") + e.what());
    }
}

api::Result CourseService::update(int64_t id, const model::Course &c)
{
    try
    {
        auto client = api::db();

        auto cur = client->execSqlSync(
            "SELECT id FROM courses WHERE id = ? AND is_deleted = 0", id);
        if (cur.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "课程不存在");

        auto dup = client->execSqlSync(
            "SELECT id FROM courses WHERE course_name = ? AND is_deleted = 0 AND id <> ?",
            c.courseName, id);
        if (dup.size() > 0)
            return api::Result::error(api::E_CONFLICT, "课程名已存在");

        if (c.teacherId > 0)
        {
            auto t = client->execSqlSync(
                "SELECT id FROM teachers WHERE id = ? AND is_deleted = 0", c.teacherId);
            if (t.size() == 0)
                return api::Result::error(api::E_PARAM, "指定的授课教师不存在");
        }

        if (c.classId > 0)
        {
            auto cl = client->execSqlSync(
                "SELECT id FROM classes WHERE id = ?", c.classId);
            if (cl.size() == 0)
                return api::Result::error(api::E_PARAM, "指定的开设班级不存在");
        }

        client->execSqlSync(
            "UPDATE courses SET course_name = ?, credit = ?, teacher_id = NULLIF(?, 0), "
            "class_id = NULLIF(?, 0), semester = ?, course_time = ?, room = ?, status = ? WHERE id = ?",
            c.courseName, c.credit, c.teacherId, c.classId, c.semester, c.courseTime, c.room, c.status, id);

        return api::Result::success(Json::Value(Json::objectValue), "修改课程成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("修改课程失败: ") + e.what());
    }
}

api::Result CourseService::remove(int64_t id)
{
    try
    {
        auto client = api::db();

        auto cur = client->execSqlSync(
            "SELECT id FROM courses WHERE id = ? AND is_deleted = 0", id);
        if (cur.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "课程不存在");

        // CRS-03：已有成绩记录的课程禁止删除
        auto cnt = client->execSqlSync(
            "SELECT COUNT(*) AS n FROM scores WHERE course_id = ?", id);
        if (cnt[0]["n"].as<int64_t>() > 0)
            return api::Result::error(api::E_CONFLICT, "该课程已有成绩记录，禁止删除");

        client->execSqlSync("UPDATE courses SET is_deleted = 1 WHERE id = ?", id);
        return api::Result::success(Json::Value(Json::objectValue), "删除课程成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("删除课程失败: ") + e.what());
    }
}

} // namespace svc
