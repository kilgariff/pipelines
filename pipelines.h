#ifndef PIPELINES_H
#define PIPELINES_H

#include "ctx.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char * name;
    char * workdir;
    char ** watch_paths;
    char * cmd;
    int valid;
} Pipeline;

typedef enum {
    PIPELINES_ERR_NONE = 0,
    PIPELINES_ERR_ARG_INVALID = -1,
    PIPELINES_ERR_ALLOC = -2,
    PIPELINES_ERR_FORK = -3,
    PIPELINES_ERR_EXEC = -4,
    PIPELINES_ERR_NONZERO_STATUS = -5
} PIPELINES_ERROR;

inline char const * pipelines_strerror(PIPELINES_ERROR error) {
    switch (error) {
        case PIPELINES_ERR_ARG_INVALID: return "Command argument is invalid";
        case PIPELINES_ERR_ALLOC: return "Memory allocation failure";
        case PIPELINES_ERR_FORK: return "Unable to fork child process";
        case PIPELINES_ERR_EXEC: return "Unable to execute specified command";
        case PIPELINES_ERR_NONZERO_STATUS: return "Child exited with non-zero status code";
    }

    return "NO_STRING_FOR_ERROR";
}

typedef enum {
    PIPELINES_RUN_IN_SHELL = 1
} PIPELINES_RUN_FLAGS;

void pipeline_free(Pipeline * pipeline);
int pipeline_start(Pipeline * pipeline);
int pipeline_wait_paths_modified(char const * const * paths);
int pipeline_run_cmd(char const * command, PIPELINES_RUN_FLAGS flags);
int pipeline_wait_all_finished();

#endif