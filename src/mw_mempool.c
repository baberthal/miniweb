#include "mw_mempool.h"
#include <stdio.h>
#include <stdlib.h>

void mw_mpool_init(mw_mempool *pmp, char *begin, size_t len)
{
    pmp->begin = begin;
    pmp->len = len;
    pmp->idx = 0;
    pmp->cflag = 0;
}

void *mw_mpool_malloc(mw_mempool *pmp, size_t len)
{
    void *ret = NULL;
    size_t r_idx = pmp->idx + len;
    if (r_idx > pmp->len) {
        ret = malloc(len);
        pmp->cflag = 1;
    }
    else {
        ret = pmp->begin + pmp->idx;
        pmp->idx = r_idx;
    }

    return ret;
}

void mw_mpool_free(mw_mempool *pmp, void *p)
{
    /* only perform free when allocated on the heap */
    if (p < (void *)pmp->begin || p >= (void *)(pmp->begin + pmp->len)) {
        free(p);
    }
}

void mw_mpool_clear(mw_mempool *pmp)
{
    pmp->idx = 0;
    pmp->cflag = 0;
}
