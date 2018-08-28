#ifndef __MYHEAP_H__
#define __MYHEAP_H__

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

private:

    //用双向链表把所有的空内存块【按地址从小到大】串起来
    struct MemBlock
    {
        MemBlock*     prev_blk;  //前一块
        MemBlock*     next_blk;  //后一块
        size_t        size;      //当前块的长度，不包括MemBlock结构体本身
        unsigned long checksum;  //前三个字段的校验和
    };

    //修改MemBlock结构体的成员后，更新校验和
    void UpdateBlockChecksum(MemBlock* blk);

    //验证校验和判断MemBlock是否有效
    bool IsValidBlock(MemBlock* blk);

    //堆的首地址和长度
    char*     mpHeapBuffer;
    size_t    mHeapSize;

    //指向第一个空闲内存块
    MemBlock* mpFreeBlockHead;
};
#endif

