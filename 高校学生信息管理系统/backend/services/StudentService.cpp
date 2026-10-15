#include "services/StudentService.h"
#include <exception>
#include <string>
#include <vector>

#include "structures/LinkedList.h"
#include "structures/SeqList.h"
#include "utils/Db.h"
#include "utils/HttpUtil.h"
using namespace std;

namespace svc
{
namespace
{

// 一行学生记录（数据库下划线列 -> 内存结构 -> JSON 小驼峰）
struct StudentRow
{
    int64_t id = 0;
    string studentNo;
    string name;
    string gender;
    string birthDate;
    int64_t classId = 0;
    string className;
    string phone;
    string email;
    string address;
};

StudentRow rowToStudent(const drogon::orm::Row &row)
{
    StudentRow s;
    s.id = row["id"].as<int64_t>();
    s.studentNo = row["student_no"].as<string>();
    s.name = row["name"].as<string>();
    s.gender = row["gender"].as<string>();
    s.birthDate = row["birth_date"].isNull() ? "" : row["birth_date"].as<string>();
    s.classId = row["class_id"].as<int64_t>();
    s.className = row["class_name"].as<string>();
    s.phone = row["phone"].isNull() ? "" : row["phone"].as<string>();
    s.email = row["email"].isNull() ? "" : row["email"].as<string>();
    s.address = row["address"].isNull() ? "" : row["address"].as<string>();
    return s;
}

// full=false 输出列表字段，full=true 输出详情字段（接口文档 §5.1 / §5.2）
Json::Value studentToJson(const StudentRow &s, bool full)
{
    Json::Value j(Json::objectValue);
    j["id"] = static_cast<Json::Int64>(s.id);
    j["studentNo"] = s.studentNo;
    j["name"] = s.name;
    j["gender"] = s.gender;
    j["className"] = s.className;
    if (!s.phone.empty())
        j["phone"] = s.phone;
    if (!s.email.empty())
        j["email"] = s.email;
    if (full)
    {
        if (!s.birthDate.empty())
            j["birthDate"] = s.birthDate;
        j["classId"] = static_cast<Json::Int64>(s.classId);
        if (!s.address.empty())
            j["address"] = s.address;
    }
    return j;
}

string trim(const string &s)
{
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos)
        return "";
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

vector<string> splitLines(const string &text)
{
    vector<string> lines;
    string cur;
    for (char c : text)
    {
        if (c == '\n')
        {
            lines.push_back(cur);
            cur.clear();
        }
        else
            cur += c;
    }
    if (!cur.empty())
        lines.push_back(cur);
    return lines;
}

vector<string> splitCsvLine(const string &line)
{
    vector<string> out;
    string cur;
    bool inQuote = false;
    for (char c : line)
    {
        if (c == '"')
            inQuote = !inQuote;
        else if (c == ',' && !inQuote)
        {
            out.push_back(cur);
            cur.clear();
        }
        else
            cur += c;
    }
    out.push_back(cur);
    return out;
}

// 含逗号/引号/换行的字段加引号包裹
string csvField(const string &v)
{
    if (v.find_first_of(",\"\n\r") == string::npos)
        return v;
    string out = "\"";
    for (char c : v)
    {
        if (c == '"')
            out += "\"\"";
        else
            out += c;
    }
    out += "\"";
    return out;
}

const char *kStudentSelect =
    "SELECT s.id, s.student_no, s.name, s.gender, s.class_id, c.class_name, "
    "       s.phone, s.email, s.address, "
    "       DATE_FORMAT(s.birth_date, '%Y-%m-%d') AS birth_date "
    "FROM students s JOIN classes c ON s.class_id = c.id ";

// $1/$2 教师范围(0=不限)  $3/$4/$5/$6 keyword(转义后)  $7/$8 classId
const char *kStudentWhere =
    "WHERE s.is_deleted = 0 "
    "AND ($1 = 0 OR s.class_id IN "
    "     (SELECT cl.id FROM classes cl WHERE cl.head_teacher_id = $2)) "
    "AND ($3 = '' OR s.student_no LIKE $4 ESCAPE '!' "
    "          OR s.name LIKE $5 ESCAPE '!' "
    "          OR c.class_name LIKE $6 ESCAPE '!') "
    "AND ($7 = 0 OR s.class_id = $8) ";

// teacher 角色按登录 user_id 解析教师 id（无档案返回 0，查不到即空结果）
int64_t resolveTeacherId(drogon::orm::DbClient *client, int64_t selfTeacherUserId)
{
    if (selfTeacherUserId <= 0)
        return 0;
    auto rows = client->execSqlSync(
        "SELECT id FROM teachers WHERE user_id = $1 AND is_deleted = 0 LIMIT 1",
        selfTeacherUserId);
    if (rows.empty())
        return -1;
    return rows[0]["id"].as<int64_t>();
}

} // namespace

api::Result StudentService::list(int64_t selfTeacherUserId, const string &keyword,
                                 int64_t classId, int page, int size)
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
            string("SELECT COUNT(*) AS total FROM students s "
                   "JOIN classes c ON s.class_id = c.id ") +
                kStudentWhere,
            teacherId, teacherId, keyword, pattern, pattern, pattern, classId, classId);
        const int64_t total = countRows[0]["total"].as<int64_t>();

        auto rows = client->execSqlSync(
            string(kStudentSelect) + kStudentWhere +
                "ORDER BY s.id DESC LIMIT $9 OFFSET $10",
            teacherId, teacherId, keyword, pattern, pattern, pattern, classId, classId,
            size, offset);

        // LinkedList 头插暂存查询行，forEach 倒序回填进 SeqList 分页缓冲
        SeqList<StudentRow> buf;
        LinkedList<StudentRow> tmp;
        for (const auto &row : rows)
            tmp.pushFront(rowToStudent(row));
        tmp.forEach([&buf](const StudentRow &s) { buf.pushBack(s); });

        Json::Value list(Json::arrayValue);
        for (size_t i = 0; i < buf.size(); ++i)
            list.append(studentToJson(buf[i], false));

        return util::okData(util::pageData(total, page, size, list));
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询学生列表失败: ") + e.what());
    }
}

api::Result StudentService::detail(int64_t id, int64_t selfTeacherUserId)
{
    try
    {
        auto client = api::db();
        const int64_t teacherId = resolveTeacherId(client.get(), selfTeacherUserId);
        if (teacherId < 0)
            return util::fail(util::kNotFound, "学生不存在");

        auto rows = client->execSqlSync(
            string(kStudentSelect) +
                "WHERE s.id = $1 AND s.is_deleted = 0 "
                "AND ($2 = 0 OR s.class_id IN "
                "     (SELECT cl.id FROM classes cl WHERE cl.head_teacher_id = $3)) "
                "LIMIT 1",
            id, teacherId, teacherId);

        if (rows.empty())
            return util::fail(util::kNotFound, "学生不存在");

        return util::okData(studentToJson(rowToStudent(rows[0]), true));
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("查询学生详情失败: ") + e.what());
    }
}

api::Result StudentService::create(const Json::Value &body)
{
    string studentNo, name, gender, err;
    int64_t classId = 0;
    if (!util::requireStr(body, "studentNo", studentNo, err) ||
        !util::requireStr(body, "name", name, err) ||
        !util::requireStr(body, "gender", gender, err) ||
        !util::requireInt(body, "classId", classId, err))
        return util::fail(util::kParam, err);
    if (!util::validGender(gender))
        return util::fail(util::kParam, "gender 只能为 男 或 女");

    string birthDate, phone, email, address;
    const bool hasBirth = util::optStr(body, "birthDate", birthDate);
    const bool hasPhone = util::optStr(body, "phone", phone);
    const bool hasEmail = util::optStr(body, "email", email);
    const bool hasAddr = util::optStr(body, "address", address);

    try
    {
        auto client = api::db();

        auto dup = client->execSqlSync(
            "SELECT id FROM students WHERE student_no = $1 LIMIT 1", studentNo);
        if (!dup.empty())
            return util::fail(util::kDuplicate, "学号已存在");

        auto cls = client->execSqlSync(
            "SELECT id FROM classes WHERE id = $1 LIMIT 1", classId);
        if (cls.empty())
            return util::fail(util::kNotFound, "班级不存在");

        // 事务保证 users + students 两表写入原子性
        int64_t stuId = 0;
        {
            auto trans = client->newTransaction();
            const string initPwd = studentNo.size() >= 6
                                       ? studentNo.substr(studentNo.size() - 6)
                                       : studentNo;
            auto userRes = trans->execSqlSync(
                "INSERT INTO users (username, password, role) VALUES ($1, $2, 'student')",
                studentNo, util::hashPassword(initPwd));
            const int64_t userId = static_cast<int64_t>(userRes.insertId());

            auto stuRes = trans->execSqlSync(
                "INSERT INTO students (user_id, student_no, name, gender, birth_date, "
                "class_id, phone, email, address) "
                "VALUES ($1, $2, $3, $4, NULLIF($5, ''), $6, "
                "NULLIF($7, ''), NULLIF($8, ''), NULLIF($9, ''))",
                userId, studentNo, name, gender, hasBirth ? birthDate : string(),
                classId, hasPhone ? phone : string(), hasEmail ? email : string(),
                hasAddr ? address : string());
            stuId = static_cast<int64_t>(stuRes.insertId());
        } // 作用域结束自动提交，异常时自动回滚

        Json::Value data(Json::objectValue);
        data["id"] = static_cast<Json::Int64>(stuId);
        return util::okData(data, "新增成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("新增学生失败: ") + e.what());
    }
}

api::Result StudentService::update(int64_t id, const Json::Value &body)
{
    string name, gender, err;
    int64_t classId = 0;
    if (!util::requireStr(body, "name", name, err) ||
        !util::requireStr(body, "gender", gender, err) ||
        !util::requireInt(body, "classId", classId, err))
        return util::fail(util::kParam, err);
    if (!util::validGender(gender))
        return util::fail(util::kParam, "gender 只能为 男 或 女");

    string birthDate, phone, email, address;
    const bool hasBirth = util::optStr(body, "birthDate", birthDate);
    const bool hasPhone = util::optStr(body, "phone", phone);
    const bool hasEmail = util::optStr(body, "email", email);
    const bool hasAddr = util::optStr(body, "address", address);

    try
    {
        auto client = api::db();

        auto exist = client->execSqlSync(
            "SELECT id FROM students WHERE id = $1 AND is_deleted = 0 LIMIT 1", id);
        if (exist.empty())
            return util::fail(util::kNotFound, "学生不存在");

        auto cls = client->execSqlSync(
            "SELECT id FROM classes WHERE id = $1 LIMIT 1", classId);
        if (cls.empty())
            return util::fail(util::kNotFound, "班级不存在");

        // 学号不可修改；空串经 NULLIF 写为 NULL；affectedRows==0 表示数据无变化
        auto res = client->execSqlSync(
            "UPDATE students SET name = $2, gender = $3, birth_date = NULLIF($4, ''), "
            "class_id = $5, phone = NULLIF($6, ''), email = NULLIF($7, ''), "
            "address = NULLIF($8, '') WHERE id = $1 AND is_deleted = 0",
            id, name, gender, hasBirth ? birthDate : string(), classId,
            hasPhone ? phone : string(), hasEmail ? email : string(),
            hasAddr ? address : string());
        (void)res;
        return util::okMsg("保存成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("编辑学生失败: ") + e.what());
    }
}

api::Result StudentService::remove(int64_t id)
{
    try
    {
        auto res = api::db()->execSqlSync(
            "UPDATE students SET is_deleted = 1 WHERE id = $1 AND is_deleted = 0", id);
        if (res.affectedRows() == 0)
            return util::fail(util::kNotFound, "学生不存在");
        return util::okMsg("删除成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("删除学生失败: ") + e.what());
    }
}

api::Result StudentService::importCsv(const string &csvText)
{
    // 约定列序: studentNo,name,gender,birthDate,classId,phone,email,address
    try
    {
        auto client = api::db();

        SeqList<int64_t> classIds;
        auto clsRows = client->execSqlSync("SELECT id FROM classes");
        for (const auto &r : clsRows)
            classIds.pushBack(r["id"].as<int64_t>());

        string text = csvText;
        const string bom = "\xEF\xBB\xBF";
        if (text.size() >= bom.size() && text.compare(0, bom.size(), bom) == 0)
            text.erase(0, bom.size());

        auto lines = splitLines(text);

        Json::Value errors(Json::arrayValue);
        int64_t total = 0, success = 0, failed = 0;

        bool firstNonEmpty = true;
        for (size_t li = 0; li < lines.size(); ++li)
        {
            const string line = trim(lines[li]);
            if (line.empty())
                continue;
            if (firstNonEmpty)
            {
                firstNonEmpty = false;
                if (line.find("studentNo") != string::npos ||
                    line.find("学号") != string::npos)
                    continue; // 表头行
            }

            const size_t lineNo = li + 1;
            const vector<string> f = splitCsvLine(line);
            const string no = trim(f[0]);
            const string nm = f.size() > 1 ? trim(f[1]) : "";
            const string gd = f.size() > 2 ? trim(f[2]) : "";
            const string bd = f.size() > 3 ? trim(f[3]) : "";
            const string cid = f.size() > 4 ? trim(f[4]) : "";
            const string ph = f.size() > 5 ? trim(f[5]) : "";
            const string em = f.size() > 6 ? trim(f[6]) : "";
            const string ad = f.size() > 7 ? trim(f[7]) : "";

            ++total;

            auto appendErr = [&errors](size_t ln, const string &m)
            {
                errors.append("第 " + to_string(ln) + " 行" + m);
            };

            if (no.empty() || nm.empty())
            {
                ++failed;
                appendErr(lineNo, "学号或姓名为空");
                continue;
            }
            if (!util::validGender(gd))
            {
                ++failed;
                appendErr(lineNo, "性别必须为 男 或 女");
                continue;
            }
            const int64_t clsId = util::toInt(cid);
            bool classOk = clsId > 0;
            if (classOk)
            {
                classOk = false;
                for (size_t i = 0; i < classIds.size(); ++i)
                {
                    if (classIds[i] == clsId)
                    {
                        classOk = true;
                        break;
                    }
                }
            }
            if (!classOk)
            {
                ++failed;
                appendErr(lineNo, "班级ID不存在");
                continue;
            }

            try
            {
                auto dup = client->execSqlSync(
                    "SELECT id FROM students WHERE student_no = $1 LIMIT 1", no);
                if (!dup.empty())
                {
                    ++failed;
                    appendErr(lineNo, "学号重复");
                    continue;
                }

                auto trans = client->newTransaction();
                const string initPwd =
                    no.size() >= 6 ? no.substr(no.size() - 6) : no;
                auto userRes = trans->execSqlSync(
                    "INSERT INTO users (username, password, role) VALUES ($1, $2, 'student')",
                    no, util::hashPassword(initPwd));
                trans->execSqlSync(
                    "INSERT INTO students (user_id, student_no, name, gender, "
                    "birth_date, class_id, phone, email, address) "
                    "VALUES ($1, $2, $3, $4, NULLIF($5, ''), $6, "
                    "NULLIF($7, ''), NULLIF($8, ''), NULLIF($9, ''))",
                    static_cast<int64_t>(userRes.insertId()), no, nm, gd, bd, clsId,
                    ph, em, ad);
                ++success;
            }
            catch (const exception &e)
            {
                ++failed;
                errors.append("第 " + to_string(lineNo) + " 行导入失败: " + e.what());
            }
        }

        Json::Value data(Json::objectValue);
        data["total"] = static_cast<Json::Int64>(total);
        data["success"] = static_cast<Json::Int64>(success);
        data["failed"] = static_cast<Json::Int64>(failed);
        data["errors"] = errors;
        return util::okData(data, "导入完成");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("导入学生失败: ") + e.what());
    }
}

api::Result StudentService::exportCsv(int64_t selfTeacherUserId, const string &keyword,
                                      int64_t classId)
{
    const string pattern = util::likeAll(keyword);
    try
    {
        auto client = api::db();
        const int64_t teacherId = resolveTeacherId(client.get(), selfTeacherUserId);
        if (teacherId < 0)
            teacherId = 0;

        auto rows = client->execSqlSync(
            string(kStudentSelect) + kStudentWhere + "ORDER BY s.id DESC",
            teacherId, teacherId, keyword, pattern, pattern, pattern, classId, classId);

        // LinkedList 头插倒序还原 -> SeqList 缓冲 -> 逐行输出 CSV
        SeqList<StudentRow> buf;
        LinkedList<StudentRow> tmp;
        for (const auto &row : rows)
            tmp.pushFront(rowToStudent(row));
        tmp.forEach([&buf](const StudentRow &s) { buf.pushBack(s); });

        string csv = "\xEF\xBB\xBF";
        csv += "studentNo,name,gender,birthDate,className,phone,email,address\r\n";
        for (size_t i = 0; i < buf.size(); ++i)
        {
            const StudentRow &s = buf[i];
            csv += csvField(s.studentNo) + ",";
            csv += csvField(s.name) + ",";
            csv += csvField(s.gender) + ",";
            csv += csvField(s.birthDate) + ",";
            csv += csvField(s.className) + ",";
            csv += csvField(s.phone) + ",";
            csv += csvField(s.email) + ",";
            csv += csvField(s.address) + "\r\n";
        }

        Json::Value data(Json::objectValue);
        data["filename"] = "students.csv";
        data["content"] = csv;
        return util::okData(data, "导出成功");
    }
    catch (const exception &e)
    {
        return util::fail(util::kDb, string("导出学生失败: ") + e.what());
    }
}

} // namespace svc