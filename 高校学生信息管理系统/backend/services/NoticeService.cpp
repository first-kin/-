#include "services/NoticeService.h"

#include <exception>
#include <string>
#include <vector>

#include "structures/MyQueue.h"
#include "utils/Db.h"
using namespace std;

namespace svc
{

namespace
{

const char *kSelectPrefix =
    "SELECT n.id, n.title, n.content, n.publisher_id, u.username AS publisher_name, "
    "       n.is_top, "
    "       DATE_FORMAT(n.create_time, '%Y-%m-%d %H:%i:%s') AS create_time, "
    "       DATE_FORMAT(n.update_time, '%Y-%m-%d %H:%i:%s') AS update_time "
    "FROM notices n LEFT JOIN users u ON n.publisher_id = u.id ";

model::Notice rowToNotice(const drogon::orm::Row &row)
{
    model::Notice n;
    n.id = row["id"].as<int64_t>();
    n.title = row["title"].as<string>();
    n.content = row["content"].as<string>();
    n.publisherId = row["publisher_id"].as<int64_t>();
    n.publisherName = row["publisher_name"].isNull() ? "" : row["publisher_name"].as<string>();
    n.isTop = (row["is_top"].as<int>() != 0);
    n.createTime = row["create_time"].isNull() ? "" : row["create_time"].as<string>();
    n.updateTime = row["update_time"].isNull() ? "" : row["update_time"].as<string>();
    return n;
}

} // namespace

api::Result NoticeService::list(const string &keyword, int page, int size)
{
    if (page < 1)
        page = 1;
    if (size < 1 || size > 100)
        size = 10;
    const int offset = (page - 1) * size;
    const string pattern = "%" + keyword + "%";

    const char *kWhere =
        "WHERE n.is_deleted = 0 "
        "AND (? = '' OR n.title LIKE ? OR n.content LIKE ?) ";

    try
    {
        auto client = api::db();

        auto countRows = client->execSqlSync(
            string("SELECT COUNT(*) AS total FROM notices n ") + kWhere,
            keyword, pattern, pattern);
        const int64_t total = countRows[0]["total"].as<int64_t>();

        auto rows = client->execSqlSync(
            string(kSelectPrefix) + kWhere +
                "ORDER BY n.is_top DESC, n.create_time DESC LIMIT ? OFFSET ?",
            keyword, pattern, pattern, size, offset);

        Json::Value arr(Json::arrayValue);
        for (const auto &row : rows)
            arr.append(model::noticeToJson(rowToNotice(row)));

        Json::Value data(Json::objectValue);
        data["total"] = static_cast<Json::Int64>(total);
        data["page"] = page;
        data["size"] = size;
        data["list"] = arr;
        return api::Result::success(data);
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询公告列表失败: ") + e.what());
    }
}

api::Result NoticeService::latest(int limit)
{
    if (limit < 1 || limit > 50)
        limit = 5;

    try
    {
        // 按「重要度升序」扫描全量公告，滚动窗口保留最后(最重要)的 limit 条
        auto rows = api::db()->execSqlSync(
            string(kSelectPrefix) +
            "WHERE n.is_deleted = 0 ORDER BY n.is_top ASC, n.create_time ASC");

        // MyQueue：定长滑动窗口，超出容量时淘汰队首(最不重要)的一条
        MyQueue<model::Notice> window;
        for (const auto &row : rows)
        {
            window.push(rowToNotice(row));
            if (static_cast<int>(window.size()) > limit)
                window.pop();
        }

        // 出队得到升序序列，反转后即为「置顶优先 + 时间倒序」
        vector<model::Notice> ordered;
        while (!window.empty())
        {
            ordered.push_back(window.front());
            window.pop();
        }

        Json::Value arr(Json::arrayValue);
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it)
            arr.append(model::noticeToJson(*it));

        Json::Value data(Json::objectValue);
        data["total"] = static_cast<Json::Int64>(rows.size());
        data["limit"] = limit;
        data["list"] = arr;
        return api::Result::success(data);
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询最新公告失败: ") + e.what());
    }
}

api::Result NoticeService::detail(int64_t id)
{
    try
    {
        auto rows = api::db()->execSqlSync(
            string(kSelectPrefix) + "WHERE n.id = ? AND n.is_deleted = 0", id);
        if (rows.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "公告不存在");
        return api::Result::success(model::noticeToJson(rowToNotice(rows[0])));
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("查询公告详情失败: ") + e.what());
    }
}

api::Result NoticeService::create(const model::Notice &n, int64_t publisherId)
{
    try
    {
        auto client = api::db();
        client->execSqlSync(
            "INSERT INTO notices (title, content, publisher_id, is_top) VALUES (?, ?, ?, ?)",
            n.title, n.content, publisherId, n.isTop ? 1 : 0);

        auto idRows = client->execSqlSync("SELECT LAST_INSERT_ID() AS id");
        Json::Value data(Json::objectValue);
        data["id"] = static_cast<Json::Int64>(idRows[0]["id"].as<int64_t>());
        return api::Result::success(data, "公告发布成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("公告发布失败: ") + e.what());
    }
}

api::Result NoticeService::update(int64_t id, const model::Notice &n)
{
    try
    {
        auto client = api::db();
        auto cur = client->execSqlSync(
            "SELECT id FROM notices WHERE id = ? AND is_deleted = 0", id);
        if (cur.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "公告不存在");

        client->execSqlSync(
            "UPDATE notices SET title = ?, content = ?, is_top = ? WHERE id = ?",
            n.title, n.content, n.isTop ? 1 : 0, id);
        return api::Result::success(Json::Value(Json::objectValue), "公告修改成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("公告修改失败: ") + e.what());
    }
}

api::Result NoticeService::remove(int64_t id)
{
    try
    {
        auto client = api::db();
        auto cur = client->execSqlSync(
            "SELECT id FROM notices WHERE id = ? AND is_deleted = 0", id);
        if (cur.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "公告不存在");

        client->execSqlSync("UPDATE notices SET is_deleted = 1 WHERE id = ?", id);
        return api::Result::success(Json::Value(Json::objectValue), "公告删除成功");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("公告删除失败: ") + e.what());
    }
}

api::Result NoticeService::setTop(int64_t id, bool isTop)
{
    try
    {
        auto client = api::db();
        auto cur = client->execSqlSync(
            "SELECT id FROM notices WHERE id = ? AND is_deleted = 0", id);
        if (cur.size() == 0)
            return api::Result::error(api::E_NOT_FOUND, "公告不存在");

        client->execSqlSync("UPDATE notices SET is_top = ? WHERE id = ?", isTop ? 1 : 0, id);
        return api::Result::success(Json::Value(Json::objectValue),
                                    isTop ? "已置顶" : "已取消置顶");
    }
    catch (const exception &e)
    {
        return api::Result::error(api::E_DB, string("置顶操作失败: ") + e.what());
    }
}

} // namespace svc