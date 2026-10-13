#pragma once
#include <cstddef>      // size_t
#include <stdexcept>    // underflow_error

using namespace std;

// 栈:后进先出(LIFO),底层为动态数组
template <typename T>
class MyStack
{
public:
    MyStack() : data_(new T[8]), cap_(8), top_(0) {}

    ~MyStack()
    {
        delete[] data_;
    }

    MyStack(const MyStack &) = delete;
    MyStack &operator=(const MyStack &) = delete;

    // 入栈
    void push(const T &v)
    {
        if (top_ == cap_)
            grow();
        data_[top_++] = v;
    }

    // 出栈(不返回值)
    void pop()
    {
        if (top_ == 0)
            throw underflow_error("MyStack::pop: stack is empty");
        --top_;
    }

    // 查看栈顶元素
    T &top() { return data_[top_ - 1]; }
    const T &top() const { return data_[top_ - 1]; }

    bool empty() const { return top_ == 0; }
    size_t size() const { return top_; }

private:
    void grow()
    {
        T *nd = new T[cap_ * 2];
        for (size_t i = 0; i < top_; ++i)
            nd[i] = data_[i];
        delete[] data_;
        data_ = nd;
        cap_ *= 2;
    }

    T *data_;
    size_t cap_;
    size_t top_;  // 栈顶下标(也等于元素个数)
};
