#include "mw_buffer.h"
#include <assert.h>
#include <malloc/malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

size_t buf_into_sz(mw_buffer *b)
{
    return (b->buf + b->sz) - b->into;
}

void buf_need_into(mw_buffer *b, size_t count)
{
    // resize the buf so it has at least count bytes ready to use
    size_t sz = buf_into_sz(b);
    if (count <= sz) return;
    malloc_good_size(count - sz + b->sz);
    unsigned char *old = b->buf;
    /* We _could_ account for a special case where:
     *      b->buf == b->into && b->into == b->outof
     * and do a free & malloc rather that realloc, but after testing it happens
     * only for the 1st use of the buffer, where realloc is the same cost as a
     * malloc anyway.
     */
    b->buf = reallocf(b->buf, sz);
    assert(b->buf);
    b->sz = sz;
    b->into = b->buf + (b->into - old);
    b->outof = b->buf + (b->outof - old);
}

void buf_used_into(mw_buffer *b, size_t used)
{
    b->into += used;
    assert(b->into <= b->buf + b->sz);
}

size_t buf_outof_sz(mw_buffer *b)
{
    return b->into - b->outof;
}

int buf_sprintf(mw_buffer *b, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    size_t s = buf_into_sz(b);
    int l = vsnprintf((char *)(b->into), s, fmt, ap);
    if ((size_t)l < s) {
        buf_used_into(b, l);
    }
    else {
        // reset ap -- vsnprintf has already used it
        va_end(ap);
        va_start(ap, fmt);
        buf_need_into(b, l);
        s = buf_into_sz(b);
        l = vsnprintf((char *)(b->into), s, fmt, ap);
        assert((size_t)l <= s);
        buf_used_into(b, l);
    }
    va_end(ap);

    return l;
}

void buf_used_outof(mw_buffer *b, size_t used)
{
    b->outof += used;
    assert(b->outof <= b->into);
    if (b->into == b->outof) b->into = b->outof = b->buf;
}

char *buf_debug_str(mw_buffer *b)
{
    char *ret = NULL;
    asprintf(&ret, "S%zu i#%zu o#%zu", b->sz, buf_into_sz(b), buf_outof_sz(b));
    return ret;
}
