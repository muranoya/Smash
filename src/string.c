#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smash.h"

String *
make_string(const char *str)
{
    String *s;

    s = (String*)malloc(sizeof(String));
    s->len = strlen(str) + 1;
    s->str = (char *)malloc(sizeof(char)*s->len);
    strcpy(s->str, str);

    return s;
}

String *
copy_string(const String *str)
{
    return make_string(str->str);
}

void
free_string(String *s)
{
    free(s->str);
    free(s);
}

String *
append_chars(String *s, const char *c)
{
    s->len = strlen(c) + s->len + 1;
    s->str = (char *)realloc(s->str, sizeof(char)*s->len);
    strcat(s->str, c);

    return s;
}

String *
append_char(String *s, const char c)
{
    return append_chars(s, (const char[]){c, '\0'});
}

const char *
string2char(String *s)
{
    return s->str;
}

