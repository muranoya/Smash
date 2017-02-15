#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "smash.h"

Vector *
make_vector()
{
    Vector *vec;
    vec = (Vector*)malloc(sizeof(Vector));
    vec->size = 32;
    vec->body = (void*)malloc(sizeof(void*)*vec->size);
    vec->len = 0;
    return vec;
}

void
free_vector(Vector *vec)
{
    free(vec->body);
    free(vec);
}

void *
vec_pop(Vector *vec)
{
    assert(vec->len > 0);
    return vec->body[--vec->len];
}

void
vec_push(Vector *vec, void *v)
{
    if (vec->len > vec->size)
    {
        vec->size *= 1.5;
        vec->body = (void**)realloc(vec->body, sizeof(void*)*vec->size);
    }
    vec->body[vec->len++] = v;
}

void
vec_concat(Vector *dst, Vector *src)
{
    int i;
    int len = vec_cnt(src);
    for (i = 0; i < len; ++i)
    {
        vec_push(dst, src->body[i]);
    }
}

void *
vec_peek(const Vector *vec)
{
    assert(vec->len > 0);
    return vec->body[vec->len-1];
}

int
vec_cnt(const Vector *vec)
{
    return vec->len;
}

