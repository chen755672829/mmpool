#ifndef __MMPOOL_H
#define __MMPOOL_H
// size_t 的声明是实现相关的。它出现在一个或多个标准头文件中，比如stdio.h 和stblib.h
// stdint.h 也是所有类型声明的头文件
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//  alignment：对齐
//  sizeof(unsigned long) 用户申请小内存时，在小内存块中的对齐方式。可以为16或32，感觉最好为16比较好。
//  这样既能减少小块内存中的没有使用的空间碎片。又能和系统调用函数分配的内存进行对齐，提升分配后，访问内存的效率。
//  因为POSIX 标明了通过malloc( ), calloc( ), 和 realloc( ) 返回的地址对于任何的C类型来说都是对齐的。
//  在Linux中，这些函数返回的地址在32位系统是以8字节为边界对齐，在64位系统是以16字节为边界对齐的。
#define MP_ALIGNMENT            sizeof(unsigned long)
#define MP_POOL_ALIGNMENT       32      // posix_memalign开辟内存时对齐的方式
#define MP_PAGE_SIZE            4096
#define MP_MAX_ALLOC_FROM_POOL  (MP_PAGE_SIZE-1)
#define MP_DEFAULT_POOL_SIZE    16*4096   //默认create内存池的参数size，也就分配的小块内存大小。


#define mp_align(n, alignment) (((n)+(alignment-1)) & ~(alignment-1))
#define mp_align_ptr(p, alignment) (void *)((((size_t)p)+(alignment-1)) & ~(alignment-1))

// 大块内存结构
struct mp_large_s {
    struct mp_large_s *next;  // 链表，指向下一个大块内存结构
    void *alloc;   // 指向具体分配的大块内存首地址
};

// 小块内存结构。其中unsigned char*表示只占一个字节
struct mp_node_s {
    unsigned char *last; //小块内存开始能分配内存地址,以后随着分配内存而移动
    unsigned char *end;  // 这块小块内存最后的地址。
    struct mp_node_s *next; // 链表
    //failed 当前小块内存中申请失败(说明当前小块中可使用的内存不足了)的次数
    //当前小块内存申请失败次数达到上限，将移动内存池的current指针将指向到
    //一下没有达到上限的小块内存。
    size_t failed;
};

// 内存池
struct mp_pool_s {
    size_t max;                 //大小块内存的分界线。
    struct mp_node_s *current;  //当前所在的 小块内存。
    struct mp_large_s *large;   //大块内存
    struct mp_node_s head[0];  //柔性数组 所有小块内存
};


/***
 * brief: 创建一个内存池 
 * @param1 [in] size是大小块内存的分界线。若申请的内存小于size，则去小块内存中，进行分配，若大于size，则分配大块内存
 * return mp_pool_s * 内存池指针。
***/
struct mp_pool_s *mp_create_pool(size_t size);


/***
 * brief: 销毁内存池
 * @param1 [in] pool 要销毁的内存池。
 * 
***/
void mp_destory_pool(struct mp_pool_s *pool);


/***
 * brief: 申请内存，类似于内核的malloc
 * @param1 [in] struct mp_pool_s *pool  内存池
 * @param2 [in] size_t size; 申请的内存大小 类似malloc(int size);
 * return void * 申请内存的起始地址
***/
void *mp_alloc(struct mp_pool_s *pool, size_t size);
void *mp_nalloc(struct mp_pool_s *pool, size_t size);


/***
 * brief: 类型与系统调用的calloc 把申请的内存清空。
 * @param1 [in] struct mp_pool_s *pool  内存池
 * @param2 [in] size_t size; 申请的内存大小
 * return void * 申请内存的起始地址
***/
void *mp_calloc(struct mp_pool_s *pool, size_t size);


/***
 * brief：在内存池中，释放对应的大块内存
 * @param1 [in] pool 内存池指针
 * @param2 [in] p  指向需要释放的大块内存的指针。
***/
void mp_free(struct mp_pool_s *pool, void *p);


#endif

