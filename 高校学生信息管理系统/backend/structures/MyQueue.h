#pragma once
#include <cstddef>      // size_t
#include <stdexcept>    // underflow_error

using std::size_t;
using std::underflow_error;

// 循环队列:先进先出(FIFO),用环形数组实现,出队 O(1)
template <typename T>
class MyQueue
{
public:
    MyQueue() : data_(new T[8]), cap_(8), head_(0), len_(0) {}

    ~MyQueue()
    {
        delete[] data_;
    }

    MyQueue(const MyQueue &) = delete;
    MyQueue &operator=(const MyQueue &) = delete;

    // 入队
    void push(const T &v)
    {
        if (len_ == cap_)
            grow();
        data_[(head_ + len_) % cap_] = v;
        ++len_;
    }

    // 出队(不返回值)
    void pop()
    {
        if (len_ == 0)
            throw underflow_error("MyQueue::pop: queue is empty");
        head_ = (head_ + 1) % cap_;
        --len_;
    }

    // 查看队首元素
    T &front() { return data_[head_]; }
    const T &front() const { return data_[head_]; }

    // 查看队尾元素
    T &back() { return data_[(head_ + len_ - 1) % cap_]; }

    bool empty() const { return len_ == 0; }
    size_t size() const { return len_; }

private:
    // 扩容时顺便把环形数据拉直
    void grow()
    {
        T *nd = new T[cap_ * 2];
        for (size_t i = 0; i < len_; ++i)
            nd[i] = data_[(head_ + i) % cap_];
        delete[] data_;
        data_ = nd;
        head_ = 0;
        cap_ *= 2;
    }

    T *data_;
    size_t cap_;
    size_t head_;  // 队首下标
    size_t len_;   // 元素个数
};
