#include "miniweb_request.h"
#include "miniweb_logging.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <time.h>

int n_reqs;
mw_request **debug_reqs;

static uint64_t getnanotime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * NSEC_PER_SEC + tv.tv_usec * NSEC_PER_SEC;
}

static void mw_req_free_impl(mw_request *req)
{
    req->reuse_guard = true;
    *(req->cb) = '\0';
    qprintf("$$$ mw_req_free %s; fd#%d; buf: %s\n",
            dispatch_queue_get_label(req->q),
            req->fd,
            req->cmd_buf);
    assert(req->sd_rd.ds == NULL && req->sd_wr.ds == NULL);
    close(req->sd);
    assert(req->fd_rd.ds == NULL);
    if (req->fd >= 0) close(req->fd);
    free(req->file_b.buf);
    free(req->deflate_b.buf);
    free(req->q_name);
    free(req->deflate);
    free(req);

    int i;
    bool found = false;
    for (i = 0; i < n_reqs; i++) {
        if (found) {
            debug_reqs[i - 1] = debug_reqs[i];
        }
        else {
            found = (debug_reqs[i] == req);
        }
    }
    debug_reqs = reallocf(debug_reqs, sizeof(mw_request *) * --n_reqs);
    assert(n_reqs >= 0);
}

void mw_req_free(mw_request *req)
{
    assert(!req->reuse_guard);
    dispatch_async(dispatch_get_main_queue(), ^{ mw_req_free_impl(req); });
}

void mw_req_disable_source(mw_request *req, mw_request_source *src)
{
    dispatch_async(req->q, ^{
        if (!src->suspended) {
            src->suspended = true;
            dispatch_suspend(src->ds);
        }
    });
}

void mw_req_enable_source(mw_request *req, mw_request_source *src)
{
    dispatch_async(req->q, ^{
        if (src->suspended) {
            src->suspended = false;
            dispatch_resume(src->ds);
        }
    });
}

void mw_req_delete_source(mw_request *req, mw_request_source *src)
{
    dispatch_async(req->q, ^{
        if (src->ds) {
            /* sources need to be resumed before they can be deleted
             * (otherwise an I/O and/or cancel block might be stranded
             * waiting for a resume that will never come, causing memory
             * leaks
             */
            mw_req_enable_source(req, src);
            dispatch_source_cancel(src->ds);
            dispatch_release(src->ds);
        }
        src->ds = NULL;
        src->suspended = false;
    });
}

void mw_dump_reqs(void)
{
    int i = 0;
    static int last_reported = -1;

    // We want to see that transition into n_reqs == 0, but we don't need to
    // keep seeing it
    if (n_reqs == 0 && n_reqs == last_reported) {
        return;
    }
    else {
        last_reported = n_reqs;
    }

    qprintf("%d active requests to dump\n", n_reqs);
    uint64_t now = getnanotime();
    /* because we iterate over the debug_reqs array in this queue (the "main"
     * queue), it has to "own" that array.  All manipulation of the array as a
     * whole must occur on this queue!
     */
    for (i = 0; i < n_reqs; i++) {
        mw_request *req = debug_reqs[i];
        qprintf("%s sources: fd_rd %p%s, sd_rd %p%s, sd_rw %p%s, timeo %p%s\n",
                req->q_name,
                (void *)req->fd_rd.ds,
                req->fd_rd.suspended ? " (SUSPENDED)" : "",
                (void *)req->sd_rd.ds,
                req->sd_rd.suspended ? " (SUSPENDED)" : "",
                (void *)req->sd_wr.ds,
                req->sd_wr.suspended ? " (SUSPENDED)" : "",
                (void *)req->timeo.ds,
                req->timeo.suspended ? " (SUSPENDED)" : "");
        if (req->timeout_at) {
            double when = req->timeout_at - now;
            when /= NSEC_PER_SEC;
            if (when < 0) {
                qprintf("  timeout %f seconds ago\n", -when);
            }
            else {
                qprintf("  timeout in %f seconds\n", when);
            }
        }
        else {
            qprintf("  timeout not yet set\n");
        }
        char *file_bd = buf_debug_str(&req->file_b),
             *deflate_bd = buf_debug_str(&req->deflate_b);
        qprintf("  file_b %s; deflate_b %s\n  cmd_buf used %ld, fd#%d; "
                "files_served %d\n",
                file_bd,
                deflate_bd,
                (long)(req->cb - req->cmd_buf),
                req->fd,
                req->files_served);
        if (req->deflate) {
            qprintf("  deflate total in: %ld", req->deflate->total_in);
        }
        qprintf("%s total written %lu, file size %lld\n",
                req->deflate ? "" : " ",
                req->total_written,
                req->sb.st_size);
        free(file_bd);
        free(deflate_bd);
    }
}

void mw_close_connection(mw_request *req)
{
    qprintf("$$$ close_connection %s, served %d files -- cancelling all "
            "sources\n",
            dispatch_queue_get_label(req->q),
            req->files_served);
    mw_req_delete_source(req, &req->fd_rd);
    mw_req_delete_source(req, &req->sd_rd);
    mw_req_delete_source(req, &req->sd_wr);
    mw_req_delete_source(req, &req->timeo);
}

void mw_write_filedata(mw_request *req, __unused size_t avail)
{
    /* we always attempt to write as much data as we have.  This is save
     * because we use non-blocking I/O.  It is a good idea because the amount
     * of buffer space that dispatch tells us may be stale (more space could
     * have opened up, or memory pressure may have caused it to go down).
     */
    mw_buffer *w_buf = req->deflate ? &req->deflate_b : &req->file_b;
    ssize_t sz = buf_outof_sz(w_buf);
    if (req->deflate) {
        struct iovec iov[2];
        if (!req->chunk_bytes_remaining) {
            req->chunk_bytes_remaining = sz;
            req->needs_zero_chunk = sz != 0;
            req->cnp = req->chunk_num;
            int n = snprintf(req->chunk_num,
                             sizeof(req->chunk_num),
                             "\r\n%lx\r\n%s",
                             sz,
                             sz ? "" : "\r\n");
            assert((size_t)n <= sizeof(req->chunk_num));
        }
        iov[0].iov_base = req->cnp;
        iov[0].iov_len = req->cnp ? strlen(req->cnp) : 0;
        iov[1].iov_base = w_buf->outof;
        iov[1].iov_len = (req->chunk_bytes_remaining < (size_t)sz)
                             ? req->chunk_bytes_remaining
                             : sz;
        sz = writev(req->sd, iov, 2);
        if (sz > 0) {
            if (req->cnp) {
                if (sz >= (ssize_t)strlen(req->cnp)) {
                    req->cnp = NULL;
                }
                else {
                    req->cnp += sz;
                }
            }
            sz -= iov[0].iov_len;
            sz = (sz < 0) ? 0 : sz;
            req->chunk_bytes_remaining -= sz;
        }
    }
    else {
        sz = write(req->sd, w_buf->outof, sz);
    }
    if (sz > 0) {
        buf_used_outof(w_buf, sz);
    }
    else if (sz < 0) {
        int e = errno;
        qprintf("write filedata %s write error %d %s\n",
                dispatch_queue_get_label(req->q),
                e,
                strerror(e));
        mw_close_connection(req);
        return;
    }

    req->total_written += sz;
    off_t bytes = req->total_written;
    if (req->deflate) {
        if (req->deflate->total_in - buf_outof_sz(w_buf)) {
            bytes = 0;
        }
    }
    if (bytes == req->sb.st_size) {
        if (req->needs_zero_chunk && req->deflate && (sz || req->cnp)) {
            return;
        }

        /* We have transferred the file, time to write the log entry
         *
         * TODO: Escape '"' in the request string
         */
        size_t rlen = strcspn(req->cmd_buf, "\r\n");
        char tstr[45], astr[45];
        struct tm tm;
        time_t clock;
        time(&clock);
        strftime(tstr,
                 sizeof(tstr),
                 "%d/%b/%Y:%H:%M:%S +0",
                 gmtime_r(&clock, &tm));
        addr2ascii(AF_INET,
                   &req->r_addr.sin_addr,
                   sizeof(struct in_addr),
                   astr);
        qfprintf(log_file,
                 "%s -- [%s] \"%.*s\" %hd %zd\n",
                 astr,
                 tstr,
                 (int)rlen,
                 req->cmd_buf,
                 req->status_number,
                 req->total_written);

        int64_t t_offset =
            5 * NSEC_PER_SEC + req->files_served * NSEC_PER_SEC / 10;
        int64_t timeout_at = req->timeout_at = getnanotime() + t_offset;

        req->timeo.ds =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, req->q);
        dispatch_source_set_timer(req->timeo.ds,
                                  dispatch_time(DISPATCH_TIME_NOW, t_offset),
                                  NSEC_PER_SEC,
                                  NSEC_PER_SEC);
        dispatch_source_set_event_handler(req->timeo.ds, ^{
            if (req->timeout_at == (uint64_t)timeout_at) {
                qfprintf(stderr,
                         "$$$ -- timeo fire (delta = %f) -- close connection: "
                         "q = "
                         "%s\n",
                         (getnanotime() - (double)timeout_at) / NSEC_PER_SEC,
                         dispatch_queue_get_label(req->q));
                mw_close_connection(req);
            }
        });
        dispatch_resume(req->timeo.ds);

        req->files_served++;
        qprintf(
            "$$$ wrote whole file (%s); timeo %p, about to enable %p and close "
            "%d, total written %zd, this is the %d%s file served\n",
            dispatch_queue_get_label(req->q),
            (void *)req->timeo.ds,
            (void *)req->sd_rd.ds,
            req->fd,
            req->total_written,
            req->files_served,
            (1 == req->files_served) ? "st" : (2 == req->files_served) ? "nd"
                                                                       : "th");
        mw_req_enable_source(req, &req->sd_rd);
        if (req->fd_rd.ds) {
            mw_req_delete_source(req, &req->fd_rd);
        }
        req->cb = req->cmd_buf;
    }
    else {
        assert(bytes <= req->sb.st_size);
    }

    if (0 == buf_outof_sz(w_buf)) {
        mw_req_disable_source(req, &req->sd_wr);
    }
}

void mw_read_filedata(mw_request *req, size_t avail)
{
    if (avail == 0) {
        mw_req_delete_source(req, &req->fd_rd);
        return;
    }

    /* We make sure we can read at least as many bytes as dispatch says are
     * available, but if our buffer is bigger we will read as much as we have
     * space for.  We have the file opened in non-blocking mode so this is safe
     */
    buf_need_into(&req->file_b, avail);
    size_t rsz = buf_into_sz(&req->file_b);
    ssize_t sz = read(req->fd, req->file_b.into, rsz);
    if (sz >= 0) {
        assert(req->sd_wr.ds);
        size_t sz0 = buf_outof_sz(&req->file_b);
        buf_used_into(&req->file_b, sz);
        assert((size_t)sz == buf_outof_sz(&req->file_b) - sz0);
    }
    else {
        int e = errno;
        qprintf("read_filedata %s read error: %d %s\n",
                dispatch_queue_get_label(req->q),
                e,
                strerror(e));
        mw_close_connection(req);
        return;
    }
    if (req->deflate) {
        // NOTE: deflateBound is "worst case", we could try with any non-zero
        // buffer, and allof more if we get Z_BUF_ERROR
        buf_need_into(&req->deflate_b,
                      deflateBound(req->deflate, buf_outof_sz(&req->file_b)));
        req->deflate->next_in = (req->file_b.outof);
        size_t o_sz = buf_outof_sz(&req->file_b);
        req->deflate->avail_in = o_sz;
        req->deflate->next_out = req->deflate_b.into;

        size_t i_sz = buf_into_sz(&req->deflate_b);
        req->deflate->avail_out = i_sz;
        assert((off_t)req->deflate->avail_in + (off_t)req->deflate->total_in <=
               req->sb.st_size);
        // at EOF we want to use Z_FINISH, otherwise we pass Z_NO_FLUSH so we
        // get max compression
        int rc = deflate(req->deflate,
                         ((off_t)req->deflate->avail_in +
                              (off_t)req->deflate->total_in >=
                          req->sb.st_size)
                             ? Z_FINISH
                             : Z_NO_FLUSH);
        assert(rc == Z_OK || rc == Z_STREAM_END);
        buf_used_outof(&req->file_b, o_sz - req->deflate->avail_in);
        buf_used_into(&req->deflate_b, i_sz - req->deflate->avail_out);
        if (i_sz != req->deflate->avail_out) {
            mw_req_enable_source(req, &req->sd_wr);
        }
    }
    else {
        mw_req_enable_source(req, &req->sd_wr);
    }
}

void mw_read_req(mw_request *req, size_t avail)
{
    if (req->timeo.ds) {
        mw_req_delete_source(req, &req->timeo);
    }

    // -1 to account for the trailing NULL byte
    int s = (sizeof(req->cmd_buf) - (req->cb - req->cmd_buf)) - 1;
    if (s == 0) {
        qprintf("reqd req fd#%d command overflow\n", req->sd);
        mw_close_connection(req);
        return;
    }

    int rd = read(req->sd, req->cb, s);
    if (rd > 0) {
        req->cb += rd;

        if (req->cb > req->cmd_buf + 4) {
            int i;
            for (i = -4; i != 0; i++) {
                char ch = *(req->cb + i);
                if (ch != '\n' && ch != '\r') {
                    break;
                }
            }
            if (i == 0) {
                *(req->cb) = '\0';
                assert(buf_outof_sz(&req->file_b) == 0);
                assert(buf_outof_sx(&req->deflate_b) == 0);
            }
        }
    }
}

void mw_accept_cb(int fd)
{
    static int req_num = 0;
    mw_request *new_req = calloc(1, sizeof(mw_request));
    assert(new_req);
    new_req->cb = new_req->cmd_buf;
    socklen_t r_len = sizeof(new_req->r_addr);
    int s = accept(fd, (struct sockaddr *)&(new_req->r_addr), &r_len);
    if (s < 0) {
        qfprintf(stderr,
                 "accept failure (rc = %d, errno=%d %s)\n",
                 s,
                 errno,
                 strerror(errno));
        return;
    }

    assert(s >= 0);
    new_req->sd = s;
    new_req->req_num = req_num;
    asprintf(&(new_req->q_name), "req#%d s#%d", req_num++, s);
    qprintf("accept_cb fd#%d; made: %s\n", fd, new_req->q_name);

    // All further work for this request will happen on new_req->q, except the
    // final teardown
    new_req->q = dispatch_queue_create(new_req->q_name, NULL);
    dispatch_set_context(new_req->q, new_req);
    dispatch_set_finalizer_f(new_req->q, (dispatch_function_t)mw_req_free);

    debug_reqs = reallocf(debug_reqs, sizeof(mw_request) * ++n_reqs);
    debug_reqs[n_reqs - 1] = new_req;

    new_req->sd_rd.ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ,
                                               new_req->sd,
                                               0,
                                               new_req->q);
    dispatch_source_set_event_handler(new_req->sd_rd.ds, ^{
        mw_read_req(new_req, dispatch_source_get_data(new_req->sd_rd.ds));
    });

    dispatch_release(new_req->q);
    dispatch_resume(new_req->sd_rd.ds);
}
