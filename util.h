#ifndef UTIL_H
#define UTIL_H

inline int count_in_str(char const * str, char c) {
    int result = 0;
    char const * c_ = str;
    while (*c_++ != '\0') result += *c_ == c;
    return result;
}

inline void split_str(char ** dest, char * src, char c) {
    char ** d = dest;
    char * s = src;
    while (*s != '\0') {
        *d++ = s;
        while (*s != '\0' && *s != c) ++s;
        *s++ = '\0';
    }
    *d = 0;
}

char * copy_str(char const * str);

char * read_entire_file(char const * path);

void print_repeated(char const * str, int count);

#endif // UTIL_H