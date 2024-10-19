#include "pipelines.h"

#include <sys/wait.h>
#include <sys/inotify.h>

char const * pipelines_strerror(PIPELINES_ERROR error);

int pipeline_monitor(Pipeline * pipeline) {

    int res = 0;

    // Init inotify:
    int fd = inotify_init();
    if (fd < 0) {
        res = fd;
        goto exit;
    }

    // Calculate the number of paths:
    int path_count = 0;
    char ** path = pipeline->watch_paths;
    while (*path++) {
        ++path_count;
    }

    // Allocate an array with one watch descriptor for each path:
    int * watches = ALLOC(sizeof(int) * path_count);
    if (watches == NULL) {
        res = -1;
        goto exit;
    }
    
    // Add watch descriptors:
    int * wd = watches;
    for (int i = 0; i < path_count; ++i) {
        wd[i] = inotify_add_watch(fd, pipeline->watch_paths[i], IN_CLOSE_WRITE);
        if (wd[i] < 0) {
            printf("Error: %s\n", strerror(errno));
            res = wd[i];
            goto exit;
        }
    }

    // Output log message stating that we're watching:
    printf("(%d) >> Pipeline %s monitoring ", getpid(), pipeline->name);
    for (int i = 0; i < path_count; ++i) {
        printf("\"%s\"", pipeline->watch_paths[i]);
        if (i < path_count - 1) printf(", ");
    }
    printf(" for changes\n");

    // Read some data from the file descriptor:
    // TODO(ross): Actually parse the event and use its metadata.
    char buf[2048];
    int r = read(fd, buf, sizeof(buf));
    if (r <= 0) {
        res = r;
        goto exit;
    }

exit:
    FREE(watches);
    close(fd);
    return res;
}

int pipeline_run_cmd(char const * command, PIPELINES_RUN_FLAGS flags) {

    int res = 0;
    char ** arg_list;
    char * cmd_copy;

    int const cmd_len = strlen(command);
    if (cmd_len == 0) return PIPELINES_ERR_ARG_INVALID;

    // Copy command string:
    cmd_copy = ALLOC(cmd_len + 1);
    if (cmd_copy == NULL) {
        res = PIPELINES_ERR_ALLOC;
        goto exit;
    }
    memcpy(cmd_copy, command, cmd_len + 1);

    // Prepare command-line arguments:
    if (flags & PIPELINES_RUN_IN_SHELL) {

        // Command will execute in a shell, so pass it as a single argument.
        char const * shell_args[4] = {
            "/bin/sh", "-c", cmd_copy, NULL
        };

        arg_list = ALLOC(sizeof(shell_args));
        if (arg_list == NULL) { res = PIPELINES_ERR_ALLOC; goto exit; }

        memcpy(arg_list, shell_args, sizeof(char const *) * 4);

    } else {

        // Split the command by whitespace so that argv[] is correct:
        arg_list = ALLOC((count_in_str(command, ' ') + 2) * sizeof(char const *));
        if (arg_list == NULL) { res = PIPELINES_ERR_ALLOC; goto exit; }

        split_str(arg_list, cmd_copy, ' ');
    }
    
    // Fork a child process:
    switch (fork()) {
        case 0: break;
        case -1: res = PIPELINES_ERR_FORK; goto exit;
        default: {
            int status;
            wait(&status);
            res = status == 0 ? PIPELINES_ERR_NONE : PIPELINES_ERR_NONZERO_STATUS;
            goto exit;
        }
    }

    // Specify environment:
    char const * const env[] = {
        "PATH=/usr/bin",
        NULL
    };

    // Replace forked child process with program specified in command:
    if (execve(arg_list[0], (char **) arg_list, (char **) env) < 0) {
        printf("Error trying to execute command: %s\n", strerror(errno));
        res = PIPELINES_ERR_EXEC;
        goto exit;
    }

exit:
    FREE(arg_list);
    FREE(cmd_copy);
    return res;
}

void pipeline_free(Pipeline * pipeline) {

    FREE(pipeline->name);
    char ** path = pipeline->watch_paths;
    while (*path++) FREE(path);
    FREE(pipeline->watch_paths);
    FREE(pipeline->cmd);
}

int pipeline_start(Pipeline * pipeline) {

    switch (fork()) {
        case -1: return 1;
        case 0: {

            chdir(pipeline->workdir);
            while (pipeline_monitor(pipeline) == 0) {
                pipeline_run_cmd(pipeline->cmd, PIPELINES_RUN_IN_SHELL);
            }
            printf(">> Error monitoring %s\n", pipeline->name);
            exit(1);
        }
    }
    return 0;
}

int pipeline_wait_all_finished(Pipeline * pipelines) {

    int count = 0;
    while (pipelines->valid) {
        ++pipelines;
        ++count;
    }

    int result = 0;
    int status = 0;

    while (count--) {
        wait(&status);
        result += status;
    }
    return result;
}