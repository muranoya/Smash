#include <stdio.h>
#include <stdlib.h>
#include "smash.h"

static void
print_uses(char *argv[])
{
    printf("%s: [file]\n", argv[0]);
    exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) print_uses(argv);

    return EXIT_SUCCESS;
}

