#include "util.h"
#include "ctx.h"

#include <stdio.h>
#include <string.h>

int count_in_str(char const * str, char c);
void split_str(char ** dest, char * src, char c);

char * copy_str(char const * str) {

    size_t sz = strlen(str);
    char * result = ALLOC(sz + 1);
    memcpy(result, str, sz);
    result[sz] = '\0';

    return result;
}

char * read_entire_file(char const * path) {

    FILE * f = fopen(path, "r");
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    int sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char * content = ALLOC(sz + 1);
    if (content == NULL) return NULL;
    content[sz] = '\0';

    if (fread(content, sz, 1, f) < 0) {
        FREE(content);
        content = NULL;
    }

    fclose(f);
    return content;
}

void print_repeated(char const * str, int count) {
    for (int i = 0; i < count; ++i) {
        printf("%s", str);
    }
}