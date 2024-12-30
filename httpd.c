/* httpd.c*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTENADDR "127.0.0.1" // localhost

#define BACKLOG 5 // maximum number of pending connections

// structure for HTTP request
struct sHttpRequest
{
    char method[8];
    char body[128];
};
typedef struct sHttpRequest httpreq;

// on success, returns a socket fd. on error, returns 0
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

// on success, returns the new client's socket fd. on error, returns 0
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

// on success, returns a pointer to the parsed HTTP request. on error, returns 0
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

// on success, returns the data. on error, returns 0
char *cli_read(int s)
{
    static char buf[512];
    memset(buf, 0, sizeof(buf));
    if (read(s, buf, sizeof(buf) - 1) < 0) {
        perror("read");
        return 0;
    }
}

void http_response(int c, char *type, char *data)
{
    char buf[512];
    int n;

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "Content-Type: %s\r\nContent-Length: %d\r\n\n%s\r\n", type, n, data);

    n = strlen(buf);
    write(c, buf, n);

    return;
}

void http_header(int c, int code)
{
    char buf[512];
    int n;

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "HTTP/1.1 %d OK\r\n", code);

    n = strlen(buf);
    write(c, buf, n);

    return;
}

void cli_connect(int s, int c)
{
    httpreq *req;
    char *p;
    char *res;

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
    
    if (!strcmp(req->method, "GET") && !strcmp(req->body, "/app/webpage")) {
        res = "<html><body><h1>Hello World</h1></body></html>";
        http_header(c, 200); // HTTP/1.1 200 OK
        http_response(c, "text/html", res);
    }
    else {
        res = "404 Not Found";
        http_header(c, 404); // HTTP/1.1 404 Not Found
        http_response(c, "text/plain", res);

    }

    free(req);
    close(c);

    return;
}

int main(int argc, char **argv)
{
    int s, c;
    char *port;

    // check command-line arguments
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
        if (!fork()) { // create child process
            cli_connect(s, c);
        }
    }

    return -1;
}
