#include "MyHeap.h"
#include <string.h>
#include <assert.h>


MyHeap::MyHeap(size_t heap_size)
 : mpHeapBuffer(0)
 , mHeapSize(heap_size)
 , mpFreeBlockHead(0)
{
    if (heap_size > sizeof(MemBlock)) {
        mpHeapBuffer = new char[heap_size];
        if (mpHeapBuffer) {
            memset(mpHeapBuffer, 0, heap_size);

            MemBlock* t = (MemBlock*)mpHeapBuffer;
            t->prev_blk = 0;
            t->next_blk = 0;
            t->size = heap_size - sizeof(MemBlock);
            UpdateBlockChecksum(t);

            mpFreeBlockHead = t;
        }
    }
}

MyHeap::~MyHeap()
{
    delete[] mpHeapBuffer;
    mpHeapBuffer = 0;
    mpFreeBlockHead = 0;
}

bool MyHeap::IsInitialStatus()
{
    return ((mpHeapBuffer == (char*)mpFreeBlockHead) &&
            (mHeapSize == mpFreeBlockHead->size + sizeof(MemBlock)));
}

void MyHeap::UpdateBlockChecksum(MemBlock* blk)
{
    unsigned long* t = (unsigned long*)blk;
    unsigned long sum = 0;

    blk->checksum = 0;

    for (int i = 0; i < sizeof(MemBlock) / sizeof(unsigned long); i++) {
        sum ^= t[i];
    }

    blk->checksum = sum;   
}

bool MyHeap::IsValidBlock(MemBlock* blk)
{
    if (0 == blk) {
        return false;
    }

    unsigned long* t = (unsigned long*)blk;
    unsigned long sum = 0;

    if ((char*)blk >= mpHeapBuffer && (char*)blk <= mpHeapBuffer + mHeapSize) {
        for (int i = 0; i < sizeof(MemBlock) / sizeof(unsigned long); i++) {
            sum ^= t[i];
        }

        return (0 == sum);
    }
    else {
        return false;
    }
}

bool MyHeap::CheckBlkList()
{
    MemBlock* blk = mpFreeBlockHead;
    MemBlock* tail = 0;
    
    while (IsValidBlock(blk)) {
        tail = blk;
        blk = blk->next_blk;
    }

    if (0 == blk) {
        blk = tail;
        while (IsValidBlock(blk)) {
            blk = blk->prev_blk;
        }
    }

    return (0 == blk);
}

void* MyHeap::malloc(size_t size)
{
    if (0 == mpFreeBlockHead) {
        //没有空闲块可用
        return 0;
    }
    
    //整数字长对齐
    if (0 == size) {
        size = 1;
    }
    size = (size + (sizeof(int) - 1)) / sizeof(int) * sizeof(int);
    
    //找到第一个够尺寸的空闲块
    MemBlock* blk = mpFreeBlockHead;
    assert(IsValidBlock(blk));

    while (blk && blk->size < size) {
        blk = blk->next_blk;
        if (blk) {
            assert(IsValidBlock(blk));
        }
    }
    if (0 == blk) {
        //没有找到够尺寸的空闲块
        return 0;
    }

    assert(CheckBlkList());
    if (blk->size >= size + sizeof(MemBlock) + sizeof(int)) {
        //此块可以拆分
        
        MemBlock* rest = (MemBlock*)((char*)blk + sizeof(MemBlock) + size);
        rest->prev_blk = blk->prev_blk;
        rest->next_blk = blk->next_blk;
        rest->size     = blk->size - size - sizeof(MemBlock);
        UpdateBlockChecksum(rest);

        if (blk->prev_blk) {
            blk->prev_blk->next_blk = rest;
            UpdateBlockChecksum(blk->prev_blk);
        }
        else {
            //blk 是头空闲块，拆分后rest变为头空闲块
            mpFreeBlockHead = rest;
        }
        if (blk->next_blk) {
            blk->next_blk->prev_blk = rest;
            UpdateBlockChecksum(blk->next_blk);
        }
        else {
            //blk 是尾空闲块
        }

        blk->size = size;
        blk->next_blk = blk->prev_blk = 0;
        UpdateBlockChecksum(blk);
        assert(CheckBlkList());
    }
    else {
        //此块不可以拆分
        
        MemBlock* prev = blk->prev_blk;
        MemBlock* next = blk->next_blk;

        if (prev) {
            prev->next_blk = next;
            UpdateBlockChecksum(prev);
        }
        else {
            //blk是头空闲块，被分配了，blk的下一块变成头空闲块
            mpFreeBlockHead = next;
        }
        if (next) {
            next->prev_blk = prev;
            UpdateBlockChecksum(next);
        }
        else {
            //blk是尾空闲块
        }

        //没有拆块，不要修改blk的size
        blk->next_blk = blk->prev_blk = 0;
        UpdateBlockChecksum(blk);
        assert(CheckBlkList());
    }

    return (void*)((char*)blk + sizeof(MemBlock));
}

void MyHeap::free(void* p)
{
    if (0 == p) {
        return;
    }

    MemBlock* blk = (MemBlock*)((char*)p - sizeof(MemBlock));
    assert(IsValidBlock(blk));
    assert(0 == blk->prev_blk && 0 == blk->next_blk);

    if (0 == mpFreeBlockHead) {
        //第一个空闲块
        mpFreeBlockHead = blk;
        blk->next_blk = 0;
        blk->prev_blk = 0;
        UpdateBlockChecksum(blk);
        memset(blk + sizeof(MemBlock), 0xcc, blk->size);
        return;
    }

    //找到插入点，插在left和right之间
    MemBlock *left = 0;
    MemBlock *right = 0;
    MemBlock *freed = 0;

    if (blk < mpFreeBlockHead) {
        //插入点在头
        left = 0;
        right = mpFreeBlockHead;
    }
    else {
        MemBlock* t = mpFreeBlockHead;
        while (t) {
            assert(IsValidBlock(t));
            if ((t < blk) && (blk < t->next_blk)) {
                //插入点在中间
                left = t;
                right = t->next_blk;
                assert(IsValidBlock(right));
                break;
            }
            else if ((t < blk) && (0 == t->next_blk)) {
                //插入点在尾
                left = t;
                right = 0;
                break;
            }
            else {
                t = t->next_blk;
            }
        }
    }
    assert(!(0 == left && 0 == right));
    if ((0 == left && 0 == right)) {
        //找不到插入点，堆数据结构被破坏了！
        return;
    }

    if (left == 0 && right != 0) {
        //插在头，考虑块合并
        
        if ((char*)blk + blk->size + sizeof(MemBlock) == (char*)right) {
            //合并
            blk->prev_blk = 0;
            blk->next_blk = right->next_blk;
            blk->size += right->size + sizeof(MemBlock);
            UpdateBlockChecksum(blk);

            if (right->next_blk) {
                right->next_blk->prev_blk = blk;
                UpdateBlockChecksum(right->next_blk);
            }
        }
        else {
            //不合并
            blk->prev_blk = 0;
            blk->next_blk = right;
            UpdateBlockChecksum(blk);

            right->prev_blk = blk;
            UpdateBlockChecksum(right);
        }
        freed = mpFreeBlockHead = blk;
    }
    else if (left != 0 && right == 0) {
        //插在尾，考虑块合并
        
        if ((char*)left + left->size + sizeof(MemBlock) == (char*)blk) {
            //合并
            left->size += blk->size + sizeof(MemBlock);
            UpdateBlockChecksum(left);
            freed = left;
        }
        else {
            //不合并
            blk->prev_blk = left;
            blk->next_blk = 0;
            UpdateBlockChecksum(blk);

            left->next_blk = blk;
            UpdateBlockChecksum(left);
            freed = blk;
        }
    }
    else {
        //插在中间，考虑跟左右块合并的各种情况

        bool merge_left = (char*)left + left->size + sizeof(MemBlock) == (char*)blk;
        bool merge_right = (char*)blk + blk->size + sizeof(MemBlock) == (char*)right;

        if (merge_left && merge_right) {
            //跟左右合并
            
            left->size += blk->size + right->size + 2 * sizeof(MemBlock);
            left->next_blk = right->next_blk;
            UpdateBlockChecksum(left);
            if (right->next_blk) {
                right->next_blk->prev_blk = left;
                UpdateBlockChecksum(right->next_blk);
            }
            freed = left;
        }
        else if (merge_left) {
            //仅跟左块合并
            
            left->size += blk->size + sizeof(MemBlock);
            UpdateBlockChecksum(left);
            freed = left;
        }
        else if (merge_right) {
            //仅跟右块合并
           
            blk->size += right->size + sizeof(MemBlock);
            blk->next_blk = right->next_blk;
            blk->prev_blk = left;
            UpdateBlockChecksum(blk);
            
            if (right->next_blk) {
                right->next_blk->prev_blk = blk;
                UpdateBlockChecksum(right->next_blk);
            }

            left->next_blk = blk;
            UpdateBlockChecksum(left);
            freed = blk;
        }
        else {
            //不合并

            blk->prev_blk = left;
            blk->next_blk = right;
            UpdateBlockChecksum(blk);

            left->next_blk = blk;
            UpdateBlockChecksum(left);

            right->prev_blk = blk;
            UpdateBlockChecksum(right);
            freed = blk;
        }
    }

    memset((char*)freed + sizeof(MemBlock), 0xcc, freed->size);
    assert(CheckBlkList());
}

