/* httpd.c*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTENADDR "127.0.0.1" // Localhost

#define BACKLOG 5 // Maximum number of pending connections

struct sHttpRequest
{
    char method[8];
    char body[128];
};
typedef struct sHttpRequest httpreq;

// On success, returns a socket fd. On error, returns 0
int srv_init(int portno)
{
    struct sockaddr_in addr;
    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(portno);
    addr.sin_addr.s_addr = inet_addr(LISTENADDR);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) {
        close(s);
        perror("bind");
        return 0;
    }

    if (listen(s, BACKLOG)) {
        close(s);
        perror("listen");
        return 0;
    }

    return s;
}

// On success, returns the new client's socket fd. On error, returns 0
int cli_accept(int s)
{
    int c;
    struct sockaddr_in addr;
    socklen_t len;

    len = 0;
    memset(&addr, 0, sizeof(addr));

    c = accept(s, (struct sockaddr *)&addr, &len);
    if (c < 0) {
        perror("accept");
        return 0;
    }

    return c;
}

// On success, returns a pointer to the parsed HTTP request. On error, returns 0
httpreq *parse_http(char *str)
{
    httpreq *req;
    char *p;

    req = malloc(sizeof(httpreq));

    for(p = str; *p && *p != ' '; p++);
    if (*p == ' ') {
        *p = 0;
    }
    else {
        //perror("Parsing HTTP request failed");
        free(req);
        return 0;
    }
    strncpy(req->method, str, 7);

    for(str=++p; *p && *p != ' '; p++);
    if (*p == ' ') {
        *p = 0;
    }
    else {
        //perror("Parsing HTTP request failed" 2);
        free(req);
        return 0;
    }

    strncpy(req->body, str, 127);
    return req;
}

// On success, returns the data. On error, returns 0
char *cli_read(int s)
{
    static char buf[512];
    memset(buf, 0, sizeof(buf));
    if (read(s, buf, sizeof(buf) - 1) < 0) {
        perror("read");
        return 0;
    }
}

void cli_connect(int s, int c)
{
    httpreq *req;
    char *p;

    p = cli_read(c);
    if (!p) {
        fprintf(stderr, "Failed to read from client\n");
        close(c);
        return;
    }

    req = parse_http(p);
    if (!req) {
        fprintf(stderr, "Failed to parse HTTP request\n");
        close(c);
        return;
    }
    
    printf("Method: %s\nBody: %s\n", req->method, req->body);
    free(req);
    close(c);
    return;
}

int main(int argc, char **argv)
{
    int s, c;
    char *port;

    // Check command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = argv[1];
    s = srv_init(atoi(port));

    if (!s) {
        fprintf(stderr, "Failed to initialize server\n");
        exit(1);
    }

    printf("Listening on port %s\n", port);

    while (1) {
        c = cli_accept(s);
        if (!c) {
            fprintf(stderr, "Failed to accept client\n");
            continue;
        }

        printf("Incoming connection\n");
        if (!fork()) { // Create child process
            cli_connect(s, c);
        }
    }

    return -1;
}
