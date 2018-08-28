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

