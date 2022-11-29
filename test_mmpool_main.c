
#include "mmpool.h"

int main(int argc, char *argv[]) {

    int size = 1 << 12;
    struct mp_pool_s *p = mp_create_pool(MP_DEFAULT_POOL_SIZE);
    //struct mp_pool_s *p = mp_create_pool(size);

    int i = 0;
    for (i = 0;i < 10;i ++) {
        void *mp = mp_alloc(p, 4095);
        //mp_free(mp);
    }

    printf("mp_create_pool->max: %ld\n", p->max);
    printf("mp_align(123, 32): %d, mp_align(17, 32): %d\n", mp_align(123, 32), mp_align(17, 32));
    printf("mp_align_ptr(p->current, 32): %lx, p->current: %lx, mp_align(p->large, 32): %lx, p->large: %lx\n", mp_align_ptr(p->current, 32), p->current, mp_align_ptr(p->large, 32), p->large);

    int j = 0;
    for (i = 0;i < 5;i ++) {

        char *pp = mp_calloc(p, 32);
        for (j = 0;j < 32;j ++) {
            if (pp[j]) {
                printf("calloc wrong\n");
            }
            printf("calloc success\n");
        }
    }

    //printf("mp_reset_pool\n");

    for (i = 0;i < 5;i ++) {
        void *l = mp_alloc(p, 8192);
        mp_free(p, l);
    }

    mp_reset_pool(p);

    //printf("mp_destory_pool\n");
    for (i = 0;i < 58;i ++) {
        mp_alloc(p, 256);
    }

    mp_destory_pool(p);

    return 0;

}

