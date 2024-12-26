#include <stdio.h>

void main(int argc, char **argv)
{
    // Check command-line arguments
    if (argc != 2)
    {
        printf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
}