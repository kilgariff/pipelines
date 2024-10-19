#include "pipelines.h"
#include "parser.h"

#include <yaml.h>

typedef enum {
    STATE_DOCUMENT,
    STATE_MAPPING,
    STATE_PIPELINES,
    STATE_PIPELINES_SEQUENCE,
    STATE_PIPELINE,
    STATE_PIPELINE_NAME,
    STATE_PIPELINE_FIELDS,
    STATE_PIPELINE_FIELD_NAME,
    STATE_PIPELINE_FIELD_VALUE_WORKDIR,
    STATE_PIPELINE_FIELD_VALUE_WATCH_PATH,
    STATE_PIPELINE_FIELD_VALUE_WATCH_PATH_SEQUENCE,
    STATE_PIPELINE_FIELD_VALUE_CMD,
    STATE_FINISHED,
} PipelineParserState;

static char * state_str(PipelineParserState state) {

    switch (state) {
        case STATE_DOCUMENT: {
            return "STATE_DOCUMENT";
        }
        case STATE_MAPPING: {
            return "STATE_MAPPING";
        }
        case STATE_PIPELINES: {
            return "STATE_PIPELINES";
        }
        case STATE_PIPELINES_SEQUENCE: {
            return "STATE_PIPELINES_SEQUENCE";
        }
        case STATE_PIPELINE: {
            return "STATE_PIPELINE";
        }
        case STATE_PIPELINE_NAME: {
            return "STATE_PIPELINE_NAME";
        }
        case STATE_PIPELINE_FIELDS: {
            return "STATE_PIPELINE_FIELDS";
        }
        case STATE_PIPELINE_FIELD_NAME: {
            return "STATE_PIPELINE_FIELD_NAME";
        }
        case STATE_PIPELINE_FIELD_VALUE_WORKDIR: {
            return "STATE_PIPELINE_FIELD_VALUE_WORKDIR";
        }
        case STATE_PIPELINE_FIELD_VALUE_WATCH_PATH: {
            return "STATE_PIPELINE_FIELD_VALUE_WATCH_PATH";
        }
        case STATE_PIPELINE_FIELD_VALUE_WATCH_PATH_SEQUENCE: {
            return "STATE_PIPELINE_FIELD_VALUE_WATCH_PATH_SEQUENCE";
        }
        case STATE_PIPELINE_FIELD_VALUE_CMD: {
            return "STATE_PIPELINE_FIELD_VALUE_CMD";
        }
        case STATE_FINISHED: {
            return "STATE_FINISHED";
        }
    }

    return "NO STRING FOR STATE";
}

Pipeline * pipelines_parse_pipefile(char const * path) {

    Pipeline * pipeline = NULL;
    char const ** watch_path = NULL;
    int watch_path_count = 0;
    int watch_paths_allocated = 0;

    char * contents = read_entire_file(path);
    if (contents == NULL) return NULL;
    
    int contents_len = strlen(contents);

    int pipelines_allocated = 32;
    Pipeline * pipelines = ALLOC(sizeof(Pipeline) * pipelines_allocated);
    if (pipelines == NULL) return NULL;

    int pipeline_count = 0;
    pipeline = pipelines;
    memset(pipeline, 0, sizeof(Pipeline));

    yaml_parser_t parser;
    yaml_event_t event;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, contents, contents_len);

    int indent = 0;
    char debug_line[4096];
    debug_line[0] = '\0';

    PipelineParserState state = STATE_DOCUMENT;
    while (state != STATE_FINISHED) {

        #ifdef VERBOSE_LOGS
            printf("%s:", state_str(state));
            print_repeated(" ", 40 - strlen(state_str(state)));
            printf("=>  ");
        #endif

        if (!yaml_parser_parse(&parser, &event)) {

            #ifdef VERBOSE_LOGS
            printf("Parse error when loading \"%s\": ", path);
            #endif

            if (parser.error == YAML_READER_ERROR) {
                printf("(%s: #%X at %ld)\n",
                       parser.problem, parser.problem_value, (long)parser.problem_offset);
            } else {
                printf("(%s at %ld)\n",
                       parser.problem, (long)parser.problem_offset);
            }

            goto error;
        }

        switch (event.type) {
            case YAML_NO_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "No event\n");
                #endif
                break;
            }
            case YAML_STREAM_START_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "Stream start\n");
                #endif
                break;
            }
            case YAML_STREAM_END_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "Stream end\n");
                #endif
                state = STATE_FINISHED;
                break;
            }
            case YAML_DOCUMENT_START_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "Document start\n");
                #endif

                if (state == STATE_DOCUMENT) {
                    state = STATE_MAPPING;
                }
                break;
            }
            case YAML_DOCUMENT_END_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "Document end\n");
                #endif
                break;
            }
            case YAML_ALIAS_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "Alias\n");
                #endif
                break;
            }
            case YAML_SCALAR_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent);
                snprintf(debug_line, sizeof(debug_line), "Scalar: %s\n", event.data.scalar.value);
                #endif

                switch (state) {

                    case STATE_PIPELINES: {

                        if (strcmp(event.data.scalar.value, "pipelines") == 0) {
                            state = STATE_PIPELINES_SEQUENCE;
                        }
                        break;
                    }

                    case STATE_PIPELINE_NAME: {
                        FREE(pipeline->name);
                        pipeline->name = copy_str(event.data.scalar.value);
                        state = STATE_PIPELINE_FIELDS;
                        break;
                    }

                    case STATE_PIPELINE_FIELD_NAME: {

                        if (strcmp(event.data.scalar.value, "workdir") == 0) {
                            state = STATE_PIPELINE_FIELD_VALUE_WORKDIR;

                        } else if (strcmp(event.data.scalar.value, "watch_paths") == 0) {
                            state = STATE_PIPELINE_FIELD_VALUE_WATCH_PATH;
                            
                        } else if (strcmp(event.data.scalar.value, "cmd") == 0) {
                            state = STATE_PIPELINE_FIELD_VALUE_CMD;
                        }
                        break;
                    }

                    case STATE_PIPELINE_FIELD_VALUE_WORKDIR: {
                        FREE(pipeline->workdir);
                        pipeline->workdir = copy_str(event.data.scalar.value);
                        state = STATE_PIPELINE_FIELD_NAME;
                        break;
                    }

                    case STATE_PIPELINE_FIELD_VALUE_WATCH_PATH:
                    case STATE_PIPELINE_FIELD_VALUE_WATCH_PATH_SEQUENCE: {
                        
                        ++watch_path_count;
                        if (watch_paths_allocated < watch_path_count + 1) {

                            if (watch_paths_allocated == 0) ++watch_paths_allocated;
                            watch_paths_allocated *= 2;

                            char ** watch_paths = ALLOC(sizeof(char *) * watch_paths_allocated);
                            if (watch_paths == NULL) {
                                goto error;
                            }

                            if (pipeline->watch_paths != NULL) {
                                memcpy(watch_paths, pipeline->watch_paths, sizeof(char *) * watch_path_count);
                                FREE(pipeline->watch_paths);
                            }

                            pipeline->watch_paths = watch_paths;
                        }

                        pipeline->watch_paths[watch_path_count - 1] = copy_str(event.data.scalar.value);
                        pipeline->watch_paths[watch_path_count] = NULL;

                        if (state == STATE_PIPELINE_FIELD_VALUE_WATCH_PATH) {
                            state = STATE_PIPELINE_FIELD_NAME;
                        }
                        break;
                    }

                    case STATE_PIPELINE_FIELD_VALUE_CMD: {
                        FREE(pipeline->cmd);
                        pipeline->cmd = copy_str(event.data.scalar.value);
                        state = STATE_PIPELINE_FIELD_NAME;
                        break;
                    }
                }

                break;
            }
            case YAML_SEQUENCE_START_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent++);
                snprintf(debug_line, sizeof(debug_line), "Sequence start\n");
                #endif

                if (state == STATE_PIPELINES_SEQUENCE) {
                    state = STATE_PIPELINE;

                } else if (state == STATE_PIPELINE_FIELD_VALUE_WATCH_PATH) {
                    state = STATE_PIPELINE_FIELD_VALUE_WATCH_PATH_SEQUENCE;
                }
                break;
            }
            case YAML_SEQUENCE_END_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", --indent);
                snprintf(debug_line, sizeof(debug_line), "Sequence end\n");
                #endif

                if (state == STATE_PIPELINE_FIELD_VALUE_WATCH_PATH_SEQUENCE) {
                    state = STATE_PIPELINE_FIELD_NAME;
                }

                break;
            }
            case YAML_MAPPING_START_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", indent++);
                snprintf(debug_line, sizeof(debug_line), "Mapping start\n");
                #endif

                switch (state) {

                    case STATE_MAPPING: {
                        state = STATE_PIPELINES;
                        break;
                    }

                    case STATE_PIPELINE: {

                        ++pipeline_count;

                        if (pipelines_allocated < pipeline_count + 1) {
                            
                            if (pipelines_allocated == 0) ++pipelines_allocated;
                            pipelines_allocated *= 2;

                            Pipeline * new_pipelines = ALLOC(pipelines_allocated);
                            if (new_pipelines == NULL) {
                                goto error;
                            }

                            memcpy(new_pipelines, pipelines, sizeof(Pipeline) * (pipeline_count + 1));
                            FREE(pipelines);
                            pipelines = new_pipelines;
                        }

                        pipeline = &pipelines[pipeline_count - 1];
                        pipeline->valid = 1;
                        watch_paths_allocated = 0;
                        watch_path_count = 0;

                        memset(&pipelines[pipeline_count], 0, sizeof(Pipeline));

                        state = STATE_PIPELINE_NAME;
                        break;
                    }

                    case STATE_PIPELINE_FIELDS: {
                        state = STATE_PIPELINE_FIELD_NAME;
                        break;
                    }
                }

                break;
            }
            case YAML_MAPPING_END_EVENT: {
                #ifdef VERBOSE_LOGS
                print_repeated("  ", --indent);
                snprintf(debug_line, sizeof(debug_line), "Mapping end\n");
                #endif

                if (state == STATE_PIPELINE_FIELD_NAME) {
                    state = STATE_PIPELINE;
                }
                break;
            }
            default: {
                #ifdef VERBOSE_LOGS
                snprintf(debug_line, sizeof(debug_line), "Unexpected event!\n");
                #endif
                break;
            }
        }

        #ifdef VERBOSE_LOGS
        printf("%s", debug_line);
        #endif

        yaml_event_delete(&event);
    }

    return pipelines;

error:

    yaml_parser_delete(&parser);

    pipeline = pipelines;
    while (pipeline->valid) {
        pipeline_free(pipeline);
    }

    FREE(pipelines);
    return NULL;
}