#ifndef MINIWEB_LOGGING_H
#define MINIWEB_LOGGING_H

#include <dispatch/dispatch.h>
#include <stdio.h>

extern char *log_name;
extern FILE *log_file;
extern dispatch_queue_t log_queue;

void qfprintf(FILE *f, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

void qfflush(FILE *f);

void reopen_log_file_when_needed(void);

#define qprintf(fmt, ...) qfprintf(stdout, fmt, ##__VA_ARGS__)

#endif /* ifndef MINIWEB_LOGGING_H */

/* vim: set ts=8 sw=4 tw=0 ft=cpp et :*/
