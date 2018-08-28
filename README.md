# malloc-free
少于300行代码实现的基于链表的极简堆管理器，具有自动合并碎片功能。是我曾经出过的编程题中自认为质量比较高的题目，但鲜有面试者成功完成作答（代码填空）。如果你的项目对性能不敏感，简单够用就行，那么你可以尝试将其用于生产。

## 接口说明
```
//MyHeap.h
class MyHeap{
public:
    MyHeap(size_t heap_size);
    ~MyHeap();

    //模拟C标准库的malloc函数
    void* malloc(size_t size);

    //模拟C标准库的free函数
    void  free(void* p);

    //判断堆是否处于初始状态，用于debug
    bool  IsInitialStatus();

    //检查空闲块双向链表的合法性，用于debug
    bool  CheckBlkList();
  }
```

## 测试代码

```
//main.cpp

#include "MyHeap.h"
#include <vector>
#include <assert.h>

int main()
{
    static const int HEAP_SIZE = 100 * 1024 * 1024;
    static const int RUN_TIMES = 10000;

    MyHeap myHeap(HEAP_SIZE);

    //断言堆处于初始状态
    assert(myHeap.IsInitialStatus());

    std::vector<void*> pointers;
    for (int i = 0; i < RUN_TIMES; i++) {
        //平均每三次循环中，2次分配，一次释放
        int k = rand() % 3;       
        if (k != 0) {
            //随机长度
            int size = rand();
            void* p = myHeap.malloc(size);
            if (p) {
                assert(myHeap.CheckBlkList());

                //新分配的内存块填充随机值
                memset(p, rand(), size);
                pointers.push_back(p);
                assert(myHeap.CheckBlkList());
            }
        }
        else
        {
            //随机释放此前分配的内存块
            int pos = rand() % pointers.size();
            void* p = pointers.at(pos);
            pointers.erase(pointers.begin() + pos);
            myHeap.free(p);
            assert(myHeap.CheckBlkList());
        }
    }

    //最后释放全部内存块
    for (size_t i = 0; i < pointers.size(); i++) {
        myHeap.free(pointers.at(i));
    }

    //断言退回到了初始状态
    assert(myHeap.IsInitialStatus());
    return 0;
}

```
