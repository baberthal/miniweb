#ifndef MINIWEB_H
#define MINIWEB_H

#include <stdio.h>

/**
 * @brief The main server structure
 */
typedef struct _mw_server {
    char *doc_base;    ///< The base path to serve files from
    char *log_name;    ///< The name of our log file
    FILE *log_file;    ///< The log file handle
    char *server_port; ///< The port we will serve on
} mw_server;

#endif /* ifndef MINIWEB_H */

/* vim: set ts=8 sw=4 tw=0 ft=cpp et :*/
