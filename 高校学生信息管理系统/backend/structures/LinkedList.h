#pragma once
#include <cstddef>  // size_t

using std::size_t;

template <typename T>
class LinkedList
{
public:
    LinkedList() : head_(nullptr), len_(0) {}
    ~LinkedList()
    {
        Node *cur = head_;
        while (cur != nullptr)
        {
            Node *next = cur->next;
            delete cur;
            cur = next;
        }
    }

    // 头插法插入节点
    void pushFront(const T &val)
    {
        Node *node = new Node{val, head_};
        head_ = node;
        ++len_;
    }

    // 按条件删除第一个匹配的节点,删除成功返回 true
    bool removeIf(bool (*pred)(const T &))
    {
        Node *prev = nullptr;
        Node *cur = head_;
        while (cur != nullptr)
        {
            if (pred(cur->val))
            {
                if (prev == nullptr)
                    head_ = cur->next;
                else
                    prev->next = cur->next;
                delete cur;
                --len_;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    // 遍历链表,对每个节点执行 f(节点值)
    template <typename F>
    void forEach(F f) const
    {
        Node *cur = head_;
        while (cur != nullptr)
        {
            f(cur->val);
            cur = cur->next;
        }
    }

    size_t size() const { return len_; }
    bool empty() const { return len_ == 0; }

private:
    // 链表节点
    struct Node
    {
        T val;
        Node *next;
    };
    Node *head_;  // 头指针
    size_t len_;  // 节点个数
};