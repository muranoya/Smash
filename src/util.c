#include <stdio.h>
#include <stdlib.h>

void
eperror(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
