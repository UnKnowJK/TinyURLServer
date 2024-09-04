/*
 * @Author: ak
 * @Date: 2024-08-07 20:13:52
 * @LastEditors: ak
 * @LastEditTime: 2024-08-13 16:01:15
 * @FilePath: /Linux_sys_pro/tiny_url/URL_client.c
 * @Description: 短地址服务客户端
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>

#define PORT 9020

int main(int argc, char const *argv[])
{
    int sockfd;

    // 创建套接字
    // 1. 域, 本地  2. 类型，字节流（TCP) 3. 协议
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 创建连接，连接服务器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // 地址类型
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 1. 套接字 2.地址 3.地址长度
    int r = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (r == -1)
    {
        printf("连接失败\n");
        return EXIT_FAILURE;
    }

    printf("客户端已开启，已连接服务器~~\n");
    while (1)
    {
        char buf[1024] = {0};
        char input[64] = {0};

        read(sockfd, buf, sizeof(buf));
        printf("%s", buf);

        if (buf[strlen(buf) - 2] == ':' || buf[strlen(buf) - 3] == ':')
        {
            scanf("%s", input);
            write(sockfd, input, sizeof(input));
            // usleep(100000);
            if (input[0] == '5')
            {
                close(sockfd);
                return 0;
            }
            memset(input, 0, sizeof(input));
        }
    }

    // close(sockfd);
    return 0;
}
