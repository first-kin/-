#include "services/TeacherService.h"
#include <exception>
#include <string>

#include "structures/Mystack.h"
#include "structures/SeqList.h"
#include "utils/Db.h"
#include "utils/HttpUtil.h"
using namespace std;

namespace svc
{
namespace
{

// 一行教师记录（数据库下划线列 -> 内存结构 -> JSON 小驼峰）
struct TeacherRow
{
    int64_t id = 0;
    string teacherNo;
    string name;
    string gender;
    string title;
    int64_t deptId = 0;
    string deptName;
    string phone;
    string email;
};

TeacherRow rowToTeacher(const drogon::orm::Row &row)
{
    TeacherRow t;
    t.id = row["id"].as<int64_t>();
    t.teacherNo = row["teacher_no"].as<string>();
    t.name = row["name"].as<string>();
    t.gender = row["gender"].as<string>();
    t.title = row["title"].isNull() ? "" : row["title"].as<string>();
    t.deptId = row["dept_id"].as<int64_t>();
    t.deptName = row["dept_name"].as<string>();
    t.phone = row["phone"].isNull() ? "" : row["phone"].as<string>();
    t.email = row["email"].isNull() ? "" : row["email"].as<string>();
    return t;
}

Json::Value teacherToJson(const TeacherRow &t)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(t.id);
    j["teacherNo"] = t.teacherNo;
    j["name"] = t.name;
    j["gender"] = t.gender;
    if (!t.title.empty())
        j["title"] = t.title;
    j["deptId"] = static_cast<Json::Int64>(t.deptId);
    j["deptName"] = t.deptName;
    if (!t.phone.empty())
        j["phone"] = t.phone;
    if (!t.email.empty())
        j["email"] = t.email;
    return j;
}

const char *kTeacherSelect =
    "SELECT t.id, t.teacher_no, t.name, t.gender, t.title, t.dept_id, "
    "       d.dept_name, t.phone, t.email "
    "FROM teachers t JOIN departments d ON t.dept_id = d.id ";

// ?/? 本人范围(0=不限)  ?/?/? keyword(转义后)  ?/? deptId
const char *kTeacherWhere =
    "WHERE t.is_deleted = 0 "
    "AND (? = 0 OR t.id = ?) "
    "AND (? = '' OR t.teacher_no LIKE ? ESCAPE '!' OR t.name LIKE ? ESCAPE '!') "
    "AND (? = 0 OR t.dept_id = ?) ";

// teacher 角色按登录 user_id 解析教师 id（无档案返回 -1，查询结果为空）
int64_t resolveTeacherId(drogon::orm::DbClient *client, int64_t selfTeacherUserId)
{
    if (selfTeacherUserId <= 0)
        return 0;
    auto rows = client->execSqlSync(
        "SELECT id FROM teachers WHERE user_id = ? AND is_deleted = 0 LIMIT 1",
        selfTeacherUserId);
    if (rows.empty())
        return -1;
    return rows[0]["id"].as<int64_t>();
}

} // namespace

api::Result TeacherService::list(int64_t selfTeacherUserId, const string &keyword,
                                 int64_t deptId, int page, int size)
{
    if (page < 1)
        page = 1;
    if (size < 1 || size > 100)
        size = 10;
    const int offset = (page - 1) * size;
    const string pattern = util::likeAll(keyword);

    try
    {
        auto client = api::db();
        const int64_t teacherId = resolveTeacherId(client.get(), selfTeacherUserId);
        if (teacherId < 0)
            return util::okData(util::pageData(0, page, size, Json::Value(Json::arrayValue)));

        auto countRows = client->execSqlSync(
            string("SELECT COUNT(*) AS total FROM teachers t ") + kTeacherWhere,
            teacherId, teacherId, keyword, pattern, pattern, deptId, deptId);
        const int64_t total = countRows[0]["total"].as<int64_t>();

        auto rows = client->execSqlSync(
            string(kTeacherSelect) + kTeacherWhere +
                "ORDER BY t.id DESC LIMIT ? OFFSET ?",
            teacherId, teacherId, keyword, pattern, pattern, deptId, deptId,
            size, offset);

        // SeqList 作为分页缓冲，按下标顺序组装 JSON
        SeqList<TeacherRow> buf;
        for (const auto &row : rows)
            buf.pushBack(rowToTeacher(row));

        Json::Value list(Json::arrayValue);
        for (size_t i = 0; i < buf.size(); ++i)
            list.append(teacherToJson(buf[i]));

        return util::okData(util::pageData(total, page, size, list));
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询教师列表失败: ") + e.what());
    }
}

api::Result TeacherService::detail(int64_t id, int64_t selfTeacherUserId)
{
    try
    {
        auto client = api::db();
        const int64_t teacherId = resolveTeacherId(client.get(), selfTeacherUserId);
        if (teacherId < 0)
            return util::fail(util::kNotFound, "教师不存在");

        auto rows = client->execSqlSync(
            string(kTeacherSelect) +
                "WHERE t.id = ? AND t.is_deleted = 0 AND (? = 0 OR t.id = ?) LIMIT 1",
            id, teacherId, teacherId);

        if (rows.empty())
            return util::fail(util::kNotFound, "教师不存在");

        return util::okData(teacherToJson(rowToTeacher(rows[0])));
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询教师详情失败: ") + e.what());
    }
}

api::Result TeacherService::create(const Json::Value &body)
{
    string teacherNo, name, gender, err;
    int64_t deptId = 0;
    if (!util::requireStr(body, "teacherNo", teacherNo, err) ||
        !util::requireStr(body, "name", name, err) ||
        !util::requireStr(body, "gender", gender, err) ||
        !util::requireInt(body, "deptId", deptId, err))
        return util::fail(util::kParam, err);
    if (!util::validGender(gender))
        return util::fail(util::kParam, "gender 只能为 男 或 女");

    string title, phone, email;
    const bool hasTitle = util::optStr(body, "title", title);
    const bool hasPhone = util::optStr(body, "phone", phone);
    const bool hasEmail = util::optStr(body, "email", email);

    try
    {
        auto client = api::db();

        auto dup = client->execSqlSync(
            "SELECT id FROM teachers WHERE teacher_no = ? LIMIT 1", teacherNo);
        if (!dup.empty())
            return util::fail(util::kDuplicate, "工号已存在");

        // 登录账号用户名 = 工号，需同时防止与学生学号撞名（users.username 唯一）
        auto dupUser = client->execSqlSync(
            "SELECT id FROM users WHERE username = ? LIMIT 1", teacherNo);
        if (!dupUser.empty())
            return util::fail(util::kDuplicate, "工号已存在");

        auto dept = client->execSqlSync(
            "SELECT id FROM departments WHERE id = ? LIMIT 1", deptId);
        if (dept.empty())
            return util::fail(util::kNotFound, "院系不存在");

        // 事务保证 users + teachers 两表写入原子性
        int64_t teaId = 0;
        {
            auto trans = client->newTransaction();
            const string initPwd = teacherNo.size() >= 6
                                       ? teacherNo.substr(teacherNo.size() - 6)
                                       : teacherNo;
            auto userRes = trans->execSqlSync(
                "INSERT INTO users (username, password, role) VALUES (?, ?, 'teacher')",
                teacherNo, util::hashPassword(initPwd));
            const int64_t userId = static_cast<int64_t>(userRes.insertId());

            auto teaRes = trans->execSqlSync(
                "INSERT INTO teachers (user_id, teacher_no, name, gender, title, "
                "dept_id, phone, email) "
                "VALUES (?, ?, ?, ?, NULLIF(?, ''), ?, NULLIF(?, ''), NULLIF(?, ''))",
                userId, teacherNo, name, gender, hasTitle ? title : string(),
                deptId, hasPhone ? phone : string(), hasEmail ? email : string());
            teaId = static_cast<int64_t>(teaRes.insertId());
        } // 作用域结束自动提交，异常时自动回滚

        Json::Value data(Json::objectValue);
        data["id"] = static_cast<Json::Int64>(teaId);
        return util::okData(data, "新增成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("新增教师失败: ") + e.what());
    }
}

api::Result TeacherService::update(int64_t id, const Json::Value &body)
{
    string name, gender, err;
    int64_t deptId = 0;
    if (!util::requireStr(body, "name", name, err) ||
        !util::requireStr(body, "gender", gender, err) ||
        !util::requireInt(body, "deptId", deptId, err))
        return util::fail(util::kParam, err);
    if (!util::validGender(gender))
        return util::fail(util::kParam, "gender 只能为 男 或 女");

    string title, phone, email;
    const bool hasTitle = util::optStr(body, "title", title);
    const bool hasPhone = util::optStr(body, "phone", phone);
    const bool hasEmail = util::optStr(body, "email", email);

    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM teachers WHERE id = ? AND is_deleted = 0 LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "教师不存在");

        auto dept = client->execSqlSync(
            "SELECT id FROM departments WHERE id = ? LIMIT 1", deptId);
        if (dept.empty())
            return util::fail(util::kNotFound, "院系不存在");

        // 工号不可修改；空串经 NULLIF 写为 NULL
        auto res = client->execSqlSync(
            "UPDATE teachers SET name = ?, gender = ?, title = NULLIF(?, ''), "
            "dept_id = ?, phone = NULLIF(?, ''), email = NULLIF(?, '') "
            "WHERE id = ? AND is_deleted = 0",
            id, name, gender, hasTitle ? title : string(), deptId,
            hasPhone ? phone : string(), hasEmail ? email : string());
        (void)res;
        return util::okMsg("保存成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("编辑教师失败: ") + e.what());
    }
}

api::Result TeacherService::remove(int64_t id)
{
    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM teachers WHERE id = ? AND is_deleted = 0 LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "教师不存在");

        // 仍担任班主任则禁止删除（classes 表无逻辑删除列，直接查引用）
        auto classRows = client->execSqlSync(
            "SELECT id FROM classes WHERE head_teacher_id = ? LIMIT 1", id);
        if (!classRows.empty())
            return util::fail(util::kRuleConflict, "该教师仍关联班级或课程，无法删除");

        // 任教课程名压入手写栈，pop 出栈倒序拼接进提示信息
        MyStack<string> courseNames;
        auto courseRows = client->execSqlSync(
            "SELECT course_name FROM courses WHERE teacher_id = ? AND is_deleted = 0",
            id);
        for (const auto &r : courseRows)
            courseNames.push(r["course_name"].as<string>());
        if (!courseNames.empty())
        {
            string detail = "该教师仍关联班级或课程，无法删除（任教: ";
            while (!courseNames.empty())
            {
                detail += courseNames.top();
                courseNames.pop();
                if (!courseNames.empty())
                    detail += "、";
            }
            detail += "）";
            return util::fail(util::kRuleConflict, detail);
        }

        auto res = client->execSqlSync(
            "UPDATE teachers SET is_deleted = 1 WHERE id = ? AND is_deleted = 0", id);
        if (res.affectedRows() == 0)
            return util::fail(util::kNotFound, "教师不存在");
        return util::okMsg("删除成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("删除教师失败: ") + e.what());
    }
}

} // namespace svc