#ifndef MW_MPOOL_H
#define MW_MPOOL_H

#include "config.h"

___BEGIN_DECLS

#include <stddef.h>

typedef struct {
    char *begin; ///< start position
    size_t len;  ///< capacity
    int idx;     ///< current index
    int cflag;   ///< clear flag
} mw_mempool;

void mw_mpool_init(mw_mempool *pmp, char *begin, size_t len);
void *mw_mpool_malloc(mw_mempool *pmp, size_t len);
void mw_mpool_free(mw_mempool *pmp, void *p);
void mw_mpool_clear(mw_mempool *pmp);

___END_DECLS
#endif /* ifndef MW_MPOOL_H */

/* vim: set ts=8 sw=4 tw=80 ft=c et :*/
