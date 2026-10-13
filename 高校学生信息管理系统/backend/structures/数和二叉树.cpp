// 数和二叉树.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<vector>
#include<stack>
using namespace std;
//双亲链
struct PNode {
    char data;
    int parent;
    PNode(char d, int p) {
        data = d;
        parent = p;
    }
};
//孩子链
struct SonNode {
    char data;
    vector<SonNode*>sons;
    SonNode() {}
    SonNode(char d) :data(d) {}
};
//长子兄弟链 
struct EBNode {
    char data;
    EBNode* brother;
    EBNode* eson;
    EBNode() :eson(NULL), brother(NULL) {}
    EBNode(char d) {
        data = d;
        eson = brother = NULL;
    }
};
//二叉树结构体
struct BTNode {
    char data;
    BTNode* lchild;
    BTNode* rchild;
    BTNode() :lchild(NULL), rchild(NULL) {}
    BTNode(char d) {
        data = d;
        lchild = rchild = NULL;
    }
};
//以b为根的二叉树的值的和
int sum(BTNode* r) {
    if (r == NULL)return 0;
    else return r->data + sum(r->lchild) + sum(r->rchild);
}
//二叉树类 
class BTree {
    BTNode* r;
public:
    BTree() {
        r = NULL;
    }
    void CreateBTree(string str) {
        stack<BTNode*>st;
        BTNode* p;
        bool flag;
        int i = 0;
        while (i < str.length()) {
            switch (str[i]) {
            case'(':
                st.push(p);
                flag = true;
                break; 
            case')':
                st.pop();
                break;
            case',':
                flag = false;
                break;
            default:
                p = new BTNode(str[i]);
                if (r == NULL)
                    r = p;
                else {
                    if (flag && !st.empty())
                        st.top()->lchild = p;
                    else if (!st.empty())
                        st.top()->rchild = p;   
            }
                break;
            }
            i++;
        }
    }
};

int main()
{
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
