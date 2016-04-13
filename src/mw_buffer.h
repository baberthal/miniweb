#ifndef MW_BUFFER_H
#define MW_BUFFER_H

#include "config.h"
#include <stddef.h>
#include <stdint.h>

/**
 * \brief Manage a buffer, currently at sz bytes, but realloc if needed.
 *
 * The buffer has a part that we read data INTO, and a part that we read data
 * OUTOF.
 *
 * Best use of space would be a circular buffer (and we would use readv/writev
 * and pass around iovec structs), but for now, we use a simpler layout:
 *     buf    ---> outof    - wasted
 *     outof  ---> into     - "ready to write data OUT OF"
 *     into   ---> buf+sz   - "ready to write data IN TO"
 */
typedef struct _mw_buffer {
    /* TODO: Use a Circular Buffer */
    size_t sz; ///< The current size of the buffer, realloc if needed.

    unsigned char *buf;
    unsigned char *into;
    unsigned char *outof;
} mw_buffer;

size_t buf_into_sz(mw_buffer *b);

void buf_need_into(mw_buffer *b, size_t count);

void buf_used_into(mw_buffer *b, size_t used);

size_t buf_outof_sz(mw_buffer *b);

int buf_sprintf(mw_buffer *b, char *fmt, ...) PRINTF_STYLE(2, 3);

void buf_used_outof(mw_buffer *b, size_t used);

char *buf_debug_str(mw_buffer *b);

#endif /* ifndef MW_BUFFER_H */
