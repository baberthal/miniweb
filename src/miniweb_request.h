#ifndef MINIWEB_REQUEST_H
#define MINIWEB_REQUEST_H

#include "mw_buffer.h"
#include <dispatch/dispatch.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <zlib.h>

/**
 * \brief A struct to track request sources.
 *
 * libdispatch gives suspension a counting behavior, but we want a simple
 * on/off behavior, so we use this struct to track suspensions.
 */
typedef struct _mw_request_source {
    dispatch_source_t ds; ///< The request source's dispatch source
    bool suspended;       ///< Use this to track suspensions.
} mw_request_source;

/**
 * @brief A structure to hold a single HTTP request
 */
typedef struct _mw_request {
    struct sockaddr_in r_addr; ///< The request address
    z_stream *deflate;         ///< Z-Stream for compressed requests

    char cmd_buf[8196]; ///< Holds the HTTP Request
    char *cb;           ///< pointer to the current position of cmd_buf

    char chunk_num[13]; ///< chunk, big enough for 8 digits plus \r\n\r\n\0
    char *cnp;          ///< pointer to current position of chunk_num

    bool needs_zero_chunk; ///< do we need zero chunk?
    bool reuse_guard;      ///< should we resuse the guard?

    short status_number;          ///< http status code
    size_t chunk_bytes_remaining; ///< the number of bytes left in chunk
    char *q_name;                 ///< Name of our queue
    int req_num;                  ///< for debugging
    int files_served;             ///< files served for this socket
    dispatch_queue_t q;           ///< this request's queue

    int sd; ///< the socket descriptor, where network I/O takes place
    int fd; ///< the source file, or -1 if none=

    mw_request_source fd_rd; ///< for read events from the source file
    mw_request_source sd_rd; ///< for read events from the network socket
    mw_request_source sd_wr; ///< for write events to the network socket
    mw_request_source timeo; ///< timeout event waiting for a new header

    uint64_t timeout_at; ///< when we will timeout
    struct stat sb;

    /**
     * For compressed GET requests:
     *   - data is compressed from file_b into deflate_b
     *   - data is written to the network socket from deflate_b
     *
     * For uncompressed GET requests:
     *   - data is written to the network socket from file_b
     *   - deflate_b is unused
     */
    mw_buffer file_b; ///< Where we read data from fd into
    mw_buffer deflate_b;
    ssize_t total_written; ///< The total number of bytes written
} mw_request;

extern int n_reqs;              ///< The number of requests we have received
extern mw_request **debug_reqs; ///< For debugging purposes

/**
 * @brief Free a request
 *
 * @param req The request to free
 */
void mw_req_free(mw_request *req);

/**
 * @brief Disable a request's source
 *
 * @param req The request whose source to disable
 * @param src The source to disable
 */
void mw_req_disable_source(mw_request *req, mw_request_source *src);

/**
 * @brief Enable a request's source
 *
 * @param req The request whose source to enable
 * @param src The source to enable
 */
void mw_req_enable_source(mw_request *req, mw_request_source *src);

/**
 * @brief Delete a request's source
 *
 * @param req The request whose source to delete
 * @param src The source to delete
 */
void mw_req_delete_source(mw_request *req, mw_request_source *src);

/**
 * @brief Dump all requests. For debugging purposes.
 */
void mw_dump_reqs(void);

/**
 * @brief Close a connection
 *
 * @param req The request whose connection to close
 */
void mw_close_connection(mw_request *req);

/**
 * @brief Write data to a file
 *
 * We have some 'content data' (either from the file, or from compressing the
 * file), and the network socket is ready for use to write to it.
 *
 * @param req The associated request
 * @param avail How much data is available for writing
 */
void mw_write_filedata(mw_request *req, size_t avail);

/**
 * @brief Read data from a file
 *
 * Our 'content data' file has some data for us to read
 *
 * @param req The associated request
 * @param avail How much data is available for reading
 */
void mw_read_filedata(mw_request *req, size_t avail);

/**
 * @brief Read a request
 *
 * We are waiting for an HTTP request. We have either not yet received a
 * request (so this is the first request), or pipelineing is on, and we haven't
 * finished a request, and there is data to read on the network socket.
 *
 * @param req The request to read
 * @param avail How much data is available for reading
 */
void mw_read_req(mw_request *req, size_t avail);

/**
 * @brief We have a new connection, allocate a req and set up the read event
 *        handler
 *
 * @param fd The file descriptor for this request's network socket
 */
void mw_accept_cb(int fd);

#endif /* ifndef MINIWEB_REQUEST_H */

/* vim: set ts=8 sw=4 tw=0 ft=c et :*/
