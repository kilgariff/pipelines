#include "pipelines.h"
#include "parser.h"

int main() {

    set_default_ctx();

    Pipeline * pipelines = pipelines_parse_pipefile("Pipefile");
    if (pipelines == NULL) {
        printf("Please define a valid Pipefile in the local directory\n");
        return 1;
    }

    Pipeline * pipeline = pipelines;
    while (pipeline->valid) {
        pipeline_start(pipeline);
        ++pipeline;
    }

    int result = pipeline_wait_all_finished();
    FREE(pipelines);
    return result;
}