#ifndef CTX_H
#define CTX_H

#include <stdlib.h>

typedef struct {
    void * (*alloc)(size_t sz);
    void (*free)(void *);
} Ctx;

extern _Thread_local Ctx ctx;

#define ALLOC(x) (*ctx.alloc)(x)
#define FREE(x) (*ctx.free)((void *) x)

inline void set_default_ctx() {
    ctx.alloc = malloc;
    ctx.free = free;
}

#endif // CTX_H