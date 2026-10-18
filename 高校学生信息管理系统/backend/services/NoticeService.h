#pragma once

#include <cstdint>
#include <string>

#include "models/Notice.h"
#include "utils/Result.h"
using std::string;

namespace svc
{

// 通知公告业务层
//   职责：公告的发布/编辑/删除/置顶 + 列表与详情查询
//   数据结构：MyQueue(手写循环队列, FIFO) 作为「最新公告滚动窗口」，
//             从全量公告流中只保留最新的 N 条（Top-N 滑动窗口）。
class NoticeService
{
public:
    // 分页列表：置顶优先，其次按发布时间倒序；支持标题/内容关键字搜索
    static api::Result list(const string &keyword, int page, int size);

    // 首页最新公告（最多 limit 条，置顶优先）
    static api::Result latest(int limit);

    static api::Result detail(int64_t id);
    static api::Result create(const model::Notice &n, int64_t publisherId);
    static api::Result update(int64_t id, const model::Notice &n);
    static api::Result remove(int64_t id);
    static api::Result setTop(int64_t id, bool isTop);
};

} // namespace svc