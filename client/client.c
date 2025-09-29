#include "func.h"

int main()
{
    // 创建客户端套接字
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(clientfd, -1, "socket");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8080);
    serveraddr.sin_addr.s_addr = inet_addr("10.14.118.48");

    int ret = connect(clientfd, 
                      (const struct sockaddr *)&serveraddr, 
                      sizeof(serveraddr));
    ERROR_CHECK(ret, -1, "connect");

    // 先接收文件名
    char filename[100] = {0};
    int len = 0;
    ret = recv(clientfd, &len, sizeof(len), MSG_WAITALL);
    printf("ret: %d, filename's len: %d\n", ret, len);
    ret = recv(clientfd, filename, len, MSG_WAITALL);
    printf("ret: %d, recv msg: %s\n", ret, filename);
    
    // 修改文件保存路径到bin目录
    char filepath[200] = {0};
    snprintf(filepath, sizeof(filepath), "bin/%s", filename);
    int wfd = open(filepath, O_CREAT | O_RDWR, 0644);
    ERROR_CHECK(wfd, -1, "open");

    // 接收文件长度（按照协议）
    off_t file_length = 0;
    recv(clientfd, &len, sizeof(len), MSG_WAITALL);
    recv(clientfd, &file_length, len, MSG_WAITALL);
    printf("file length: %ld\n", file_length);
    
    // 接收文件内容（服务端用sendfile直接发送）
    char buff[4096] = {0};
    off_t received = 0;
    while(received < file_length) {
        int to_recv = (file_length - received > sizeof(buff)) ? sizeof(buff) : (file_length - received);
        ret = recv(clientfd, buff, to_recv, 0);
        if(ret <= 0) {
            break;
        }
        write(wfd, buff, ret);
        received += ret;
    }
    
    printf("File received successfully: %ld bytes\n", received);
    close(wfd);
    close(clientfd);

    return 0;
}