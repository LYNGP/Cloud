#include "threadPool.h"

int tcpInit(int *psockfd, const char *ip, const char *port)
{
    // socket setsockopt bind listen
    *psockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);

    int reuse = 1;
    int ret = setsockopt(*psockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ERROR_CHECK(ret, -1, "setsockopt SO_REUSEADDR failed");
    ret = bind(*psockfd, (struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "bind failed");
    listen(*psockfd, 10);
    return 0;
}