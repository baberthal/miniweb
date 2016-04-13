#include "miniweb_logging.h"
#include <stdarg.h>
#include <stdlib.h>

char *log_name = NULL;
FILE *log_file = NULL;
dispatch_queue_t log_queue;

void qfprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *str;
    /* We generate the formatted string on the same queue (thread) that calls
     * this function, so the values can change while the fputs call is being
     * sent to the queue, or waiting for other work to be completed
     */
    vasprintf(&str, fmt, ap);
    dispatch_async(log_queue, ^{
        fputs(str, f);
        free(str);
    });
    if ('*' == *fmt) {
        dispatch_sync(log_queue, ^{ fflush(f); });
    }
    va_end(ap);
}

void qfflush(FILE *f)
{
    dispatch_sync(log_queue, ^{ fflush(f); });
}

void reopen_log_file_when_needed(void)
{
    int lf_dup = dup(fileno(log_file));
    FILE **lf = &log_file;

    dispatch_source_t vn =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE,
                               lf_dup,
                               DISPATCH_VNODE_REVOKE | DISPATCH_VNODE_RENAME |
                                   DISPATCH_VNODE_DELETE,
                               log_queue);

    dispatch_source_set_event_handler(vn, ^{
        printf("lf_dup is %d (logfile's fileno = %d)\n",
               lf_dup,
               fileno(log_file));
        fprintf(log_file, "# flush 'n roll!\n");
        dispatch_source_cancel(vn);
        dispatch_release(vn);
        fflush(log_file);
        *lf = freopen(log_name, "a", log_file);

        reopen_log_file_when_needed();
    });

    dispatch_source_set_cancel_handler(vn, ^{ close(lf_dup); });
    dispatch_resume(vn);
}

/* vim: set ts=8 sw=4 tw=0 ft=c et :*/
