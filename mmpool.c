#include "mmpool.h"


/***
 * brief: 创建一个内存池 
 * @param1 [in] 参数size，默认分配创建内存的size为16*4096 (MP_DEFAULT_POOL_SIZE)，这个size为分配的小块内存的大小
 * return mp_pool_s * 内存池指针。
***/
struct mp_pool_s *mp_create_pool(size_t size)
{
    struct mp_pool_s *p;
    // 这个大小 = size + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s)，为什么会加上sizeof(struct mp_pool_s) + sizeof(struct mp_node_s)
    // 若我们这分配posix_memalign((void **)&p, MP_POOL_ALIGNMENT, size)
    // 若这里size = 4096, 而我们要分配4095，发现这时不够
    // 因为last = (unsigned char *)p + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s);
    // end = p->head->last + size; 
    // 能分配的大小为 end-last 发现小于4095，所以就分配不出来。
    // 申请的内存以32个字节(MP_POOL_ALIGNMENT) 对齐。
    int ret = posix_memalign((void **)&p, MP_POOL_ALIGNMENT, size + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s));
    if (ret) {
        return NULL;
    }
    // max 决定是去小块内存中去分配，还是去大块内存中去分配，
    // max最大不能超过4095，也就是在小内存块中，一次分配到内存最大不能超过4095
    // 为什么要以4095为分界线呢？应该是x86架构下，一页内存是4096，
    // 为了提高分配后的访问效率，所以一次分配的内存不能超过一页大小。
    // 但是为什么max的最大不是4096，而是4095呢？这个问题我目前还不清楚。
    p->max = (size < MP_MAX_ALLOC_FROM_POOL) ? size : MP_MAX_ALLOC_FROM_POOL;
    p->current = p->head;
    p->large = NULL;

    p->head->last = (unsigned char *)p + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s);
    p->head->end = p->head->last + size;
    p->head->failed = 0;

    return p;
}


/***
 * brief: 销毁内存池
 * @param1 [in] pool 要销毁的内存池。
 * 
***/
void mp_destory_pool(struct mp_pool_s *pool)
{
    struct mp_node_s *h, *n;
    struct mp_large_s *l;
    // 释放大块内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }
    // 释放小块内存
    h = pool->head->next;
    while (h) {
        n = h->next;
        free(h);
        h = n;
    }
    free(pool);
}


/***
 * brief: 重置内存池，释放大块内存。小块内存不释放，但是小块内存的last初始化。
 * @param1 [in] pool 内存池指针。
 * 
***/
void mp_reset_pool(struct mp_pool_s *pool)
{
    struct mp_node_s *h;
    struct mp_large_s *l;
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }
    pool->large = NULL;
    for (h = pool->head; h; h = h->next) {
        h->last = (unsigned char *)h + sizeof(struct mp_node_s);
    }
}


/***
 * brief: 申请小块内存。
 * @param1 [in] pool 内存池
 * @param2 [in] size，申请小块内存后，需要从当前小块内存中分配出去的内存。
 * return 用户需要的内存的首地址。
***/
//int kk = 0;
static void *mp_alloc_block(struct mp_pool_s *pool, size_t size)
{
    unsigned char *m;
    struct mp_node_s *h = pool->head;
    // psize = sizeof(mp_node_s) + 4096;
    size_t psize = (size_t)(h->end - (unsigned char *)h);
    int ret = posix_memalign((void **)&m, MP_POOL_ALIGNMENT, psize);
    if (ret) return NULL;
    //printf("--------kk==%d \n",++kk);
    struct mp_node_s *p, *new_node, *current;
    new_node = (struct mp_node_s*)m;

    // 可以看到 end指针直接等于 m + sizeof(mp_node_s) + 4096，而posix_memalign分配的空间，比可能大于等于这个值大(因为有32字节对齐)
    // end指针，不会影响，新建小块内存后的第一次内存的分配，也就是这里的size大小，这里的last指针直接指向了m+size.
    // 但有会影响第2次及以后在这个小块内存中进行分配。
    // 因为第一次分配不会收到end的影响，我们我们可以看到下面m指针的移动，
    // 先进行加m += sizeof(struct mp_node_s); 然后又进行了一次字节对齐，在这次字节对齐后，然后直接分配出去了size大小的内存
    // 这时候last指针，指向了 m+size，这时候的last是否会大于end指针呢？
    // 这里不会的因为我们下面的字节对齐，m就会移动的，下面会解释
    new_node->end = m + psize;  
    new_node->next = NULL;
    new_node->failed = 0;

    // sizeof(struct mp_node_s) 在32位系统中为16，在64位系统为32.
    // m指针的首地址是可以整除32的。所以。m+=sizeof(struct mp_node_s);后的空间一定可以整除4或8的。
    m += sizeof(struct mp_node_s);   
    // 从当前小块内存中，分配出去第一个内存，先字节对齐以 sizeof(unsigned long) 大小对齐。
    // 感觉其实这里不用对齐了。m指针的空间的首地址应该都是可以整除8。
    // 所以这里的last指针是不会超过end指针的。
    //printf("---%s---- m==%p\n",__func__, m);
    m = mp_align_ptr(m, MP_ALIGNMENT);
    //printf("---%s---- mp_align_ptr==%p\n",__func__, m);
    new_node->last = m + size;
    //printf("new_node->last==%p new_node->end==%p\n",new_node->last, new_node->end);

    /***
     * 当fail失败次数超过5次，那么current指针就会往后移动。
     * 由下面代码可见，若第3个小块中fail = 5了，那么第2个小块中的fail一定大于等于5，为什么呢？
     * 因为链表是从前往后遍历的，且遍历到当前位置，当前位置的fail就会+1。
     * 遍历到第3个小块内存时，第2个小块内存中的fail就已经加过1。
     * 所以不会出现current指针之前的小块内存申请内存失败的次数小于等于5的情况。
    ***/
    current = pool->current;
    for (p = current; p->next; p = p->next) {
        if (p->failed++ > 4) { //
            current = p->next;
        }
    }
    // 尾插法插入到链表中去。
    /* 尾插法的好处: 可以充分的利用所有的小块内存。*/
    p->next = new_node;
    pool->current = current ? current : new_node;

    return m;
}


/***
 * brief: 申请>max(最大为4095)的大内存
 * @param1 [in] pool 内存池指针
 * @param2 [in] size 需要找系统申请的大内存的大小。
 * return 用户需要的内存的首地址。
***/
static void *mp_alloc_large(struct mp_pool_s *pool, size_t size)
{
    /* 先申请大内存块 */
    void *p = malloc(size);
    if (p == NULL) return NULL;

    size_t n = 0;
    struct mp_large_s *large;
    /* 先判断是否存在已经使用，且已经释放的大内存，只剩下了在小内存块中申请的 mp_large_s结构 */
    /* 为了提升效率，若前4个没有只剩下，mp_larg_s结构的指针，那么就跳出循环*/
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }
        if (n ++ > 3) break;
    }
    /* 在小内存中，申请 mp_large_s 大内存的结构*/
    large = mp_alloc(pool, sizeof(struct mp_large_s));
    if (large == NULL) {
        free(p);
        return NULL;
    }
    /* 把大内存块 与 mp_large_s结构中 alloc相连*/
    large->alloc = p;
    /* 头插法 */
    large->next = pool->large;
    pool->large = large;

    return p;
}


/***
 * brief: 
 *  (1) 使用系统函数posix_memalign申请内存，自己规定内存对齐方式，对齐大小需要时2^n, 不能大于256.
 *  (2) 给内存池添加申请大块内存。
 * @param1 [in] pool 内存池指针。
 * @param2 [in] size 申请的大块内存的大小。
 * @param3 [in] alignment 对齐方式，为2^n
***/
void *mp_memalign(struct mp_pool_s *pool, size_t size, size_t alignment)
{
    void *p;
    int ret = posix_memalign(&p, alignment, size);
    if (ret) {
        return NULL;
    }
    struct mp_large_s *large = mp_alloc(pool, sizeof(struct mp_large_s));
    if (large == NULL) {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


/***
 * brief: 申请内存，类似于内核的malloc
 * @param1 [in] struct mp_pool_s *pool  内存池
 * @param2 [in] size_t size; 申请的内存大小 类似malloc(int size);
 * return void * 申请内存的起始地址
***/
void *mp_alloc(struct mp_pool_s *pool, size_t size)
{
    unsigned char *m;
    struct mp_node_s *p;
    /* 用户申请内存小于等于max，将去小块内存中去申请*/
    if (size <= pool->max) {
        p = pool->current;

        do {
            m = mp_align_ptr(p->last, MP_ALIGNMENT);
            if ((size_t)(p->end - m) >= size) {
                p->last = m + size;
                return m;
            }
            p = p->next;
        } while (p);

        return mp_alloc_block(pool, size);
    }
    /* 若大于max去大块内存中申请 */
    return mp_alloc_large(pool, size);
}


void *mp_nalloc(struct mp_pool_s *pool, size_t size)
{
    unsigned char *m;
    struct mp_node_s *p;

    if (size <= pool->max) {
        p = pool->current;
        do {
            m = p->last;
            if ((size_t)(p->end - m) >= size) {
                p->last = m+size;
                return m;
            }
            p = p->next;
        } while (p);

        return mp_alloc_block(pool, size);
    }

    return mp_alloc_large(pool, size);
}


/***
 * brief: 类型与系统调用的calloc 把申请的内存清空。
 * @param1 [in] struct mp_pool_s *pool  内存池
 * @param2 [in] size_t size; 申请的内存大小
 * return void * 申请内存的起始地址
***/
void *mp_calloc(struct mp_pool_s *pool, size_t size)
{
    void *p = mp_alloc(pool, size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}


/***
 * brief：在内存池中，释放对应的大块内存
 * @param1 [in] pool 内存池指针
 * @param2 [in] p  指向需要释放的大块内存的指针。
***/
void mp_free(struct mp_pool_s *pool, void *p)
{
    struct mp_large_s *l;
    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            free(l->alloc);
            l->alloc = NULL;
            return ;
        }
    }
}


