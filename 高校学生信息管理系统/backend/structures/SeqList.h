#pragma once
#include <cstddef>      // size_t
#include <stdexcept>    // out_of_range

using namespace std;

// 顺序表:底层为动态数组,容量不足时自动 2 倍扩容
template <typename T>
class SeqList
{
public:
    SeqList() : data_(new T[8]), cap_(8), len_(0) {}

    ~SeqList()
    {
        delete[] data_;
    }

    // 禁用拷贝,防止浅拷贝导致重复释放(Rule of Three)
    SeqList(const SeqList &) = delete;
    SeqList &operator=(const SeqList &) = delete;

    // 尾部插入元素
    void pushBack(const T &v)
    {
        if (len_ == cap_)
            grow();
        data_[len_++] = v;
    }

    // 删除下标 i 处的元素,后面元素前移
    void removeAt(size_t i)
    {
        if (i >= len_)
            throw out_of_range("SeqList::removeAt: index out of range");
        for (size_t j = i; j + 1 < len_; ++j)
            data_[j] = data_[j + 1];
        --len_;
    }

    T &operator[](size_t i) { return data_[i]; }
    const T &operator[](size_t i) const { return data_[i]; }

    size_t size() const { return len_; }
    bool empty() const { return len_ == 0; }

private:
    // 容量翻倍扩容
    void grow()
    {
        T *nd = new T[cap_ * 2];
        for (size_t i = 0; i < len_; ++i)
            nd[i] = data_[i];
        delete[] data_;
        data_ = nd;
        cap_ *= 2;
    }

    T *data_;     // 底层数组
    size_t cap_;  // 当前容量
    size_t len_;  // 元素个数
};
