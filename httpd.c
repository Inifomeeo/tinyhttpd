/* httpd.c*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTENADDR "127.0.0.1"

// On error, returns 0. On success, returns the new client's socket fd
int cli_accept(int s)
{
    int c;
    struct sockaddr_in addr;
    socklen_t len;

    len = 0;
    memset(&addr, 0, sizeof(addr));

    c = accept(s, (struct sockaddr *)&addr, &len);
    if (c < 0)
    {
        perror("accept");
        return 0;
    }

    return c;
}

void cli_connect(int s, int c)
{
    return;
}

// On error, returns 0. On success, returns a socket fd
int srv_init(int portno)
{
    struct sockaddr_in addr;
    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        perror("socket");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(portno);
    addr.sin_addr.s_addr = inet_addr(LISTENADDR);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)))
    {
        close(s);
        perror("bind");
        return 0;
    }

    if (listen(s, 5))
    {
        close(s);
        perror("listen");
        return 0;
    }

    return s;
}

int main(int argc, char **argv)
{
    int s;
    char *port;

    // Check command-line arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = argv[1];
    s = srv_init(atoi(port));

    if (!s)
    {
        fprintf(stderr, "Failed to initialize server\n");
        exit(1);
    }

    printf("Listening on port %s\n", port);

    while (1)
    {
        int c = cli_accept(s);
        if (!c)
        {
            fprintf(stderr, "Failed to accept client\n");
            continue;
        }

        printf("Incoming connection\n");
        if (!fork()) // child process
        {
            cli_connect(s, c);
        }
    }

    return -1;
}
