/*
 * @Author: ak
 * @Date: 2024-08-13 09:33:01
 * @LastEditors: ak
 * @LastEditTime: 2024-08-13 15:55:05
 * @FilePath: /Linux_sys_pro/tiny_url/TinyURL_server.c
 * @Description: 短地址服务，监听多个端口，处理不同请求（地址管理、寻址重定向）
 */
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "url_manage.h"

#define MAX_EVENTS 10
#define PORTS_COUNT 2 // 假设监听两个端口
#define url_manage_port 9020
#define addressing_skip_port 8020

// 创建套接字
int createSocket(int port)
{
    int server_fd;
    struct sockaddr_in address;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listen on port %d...\n", port);
    return server_fd;
}
// url后台管理
void url_manage(int server_fd, int client_fd, struct sockaddr_in client_addr);
// url地址寻址重定向
void addressing_skip(int server_fd, int client_fd);
// 发送301重定向的函数
void send_301_redirect(int socket_fd, const char *location);

int main()
{
    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event, events[MAX_EVENTS];
    int listenfds[PORTS_COUNT] = {0}; // 存储监听套接字的数组
    int port[2] = {url_manage_port, addressing_skip_port};
    listenfds[0] = createSocket(url_manage_port);
    listenfds[1] = createSocket(addressing_skip_port);

    // 创建并设置每个端口的监听套接字
    for (int i = 0; i < PORTS_COUNT; ++i)
    {
        // 将监听套接字添加到epoll的监控列表中
        event.data.fd = listenfds[i];
        event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfds[i], &event) == -1)
        {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
        event.data.ptr = (void *)&port[i];
    }
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    // epoll等待事件的发生
    while (1)
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].events & EPOLLIN)
            {
                // 接受连接
                int client_fd = accept(events[i].data.fd, (struct sockaddr *)&client_addr, &client_addr_len);
                // ... 处理连接 ...
                // 根据端口号，处理不同需求
                if (listenfds[0] == events[i].data.fd)
                {
                    char client_ip[128];
                    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, sizeof(client_ip));
                    printf("客户端已连接，IP地址为: %s\n", client_ip);
                    url_manage(listenfds[0], client_fd, client_addr);
                }
                else
                {
                    char client_ip[128];
                    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, sizeof(client_ip));
                    printf("客户端已连接，IP地址为: %s\n", client_ip);
                    addressing_skip(listenfds[1], client_fd);
                }
            }
        }
    }

    close(epfd);
    return 0;
}

void url_manage(int server_fd, int client_fd, struct sockaddr_in client_addr)
{
    while (1)
    {
        // 接收请求，建立了 TCP 连接，获得了一个新的客户端套接字
        printf("等待客户端请求...\n");
        // 获取客户端IP地址
        char client_ip[128];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, sizeof(client_ip));
        printf("客户端已连接，IP地址为: %s\n", client_ip);

        while (1)
        {
            show(client_fd);

            char action[64];
            read(client_fd, action, sizeof(action));

            switch (action[0])
            {
            case '1':
                createTinyURL(client_fd);
                break;
            case '2':
                analyzeTinyURL(client_fd);
                break;
            case '3':
                operationTinyURL(client_fd);
                break;
            case '4':
                statisticalInformation(client_fd);
                break;
            case '5':
                close(client_fd);
                close(server_fd);
                return;
            default:
                printf("请选择正确操作数(1-5)\n");
                break;
            }
        }
    }
}

void addressing_skip(int server_fd, int client_fd)
{
    char buffer[1024] = {0};
    read(client_fd, buffer, 1024);
    
    // 从http请求中解析短地址
    int count = 0;
    int sign = 0;
    char tem[8] = {0};
    char tiny_url[16] = {0};
    for (int i = 0; i < strlen(buffer); i++)
    {
        if (buffer[i] == ' ' || buffer[i] == '/')
        {
            sign++;
            continue;
        }
        if (sign == 3)
        {
            break;
        }
        if (sign == 2)
        {
            tem[count] = buffer[i];
            count++;
        }
    }
    sprintf(tiny_url, "ak.cn/%s", tem);
    
    // 获得长地址
    char *location = requireLongURL(tiny_url);
    printf("%s\n", location);
    
    const char *response_headers = "HTTP/1.1 301 Moved Permanently\r\n"
                                   "Location: ";
    char headers_with_location[1024];
    int headers_len = snprintf(headers_with_location, sizeof(headers_with_location),
                               "%s%s\r\n"
                               "Content-Length: 0\r\n"
                               "Connection: close\r\n\r\n",
                               response_headers, location);

    // 发送重定向响应
    send(client_fd, headers_with_location, headers_len, 0);

    // 关闭连接
    close(client_fd);
    close(server_fd);
    return;
}
