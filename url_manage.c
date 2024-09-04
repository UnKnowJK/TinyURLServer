/*
 * @Author: ak
 * @Date: 2024-08-13 15:46:22
 * @LastEditors: ak
 * @LastEditTime: 2024-08-13 16:01:07
 * @FilePath: /Linux_sys_pro/tiny_url/url_manage.c
 * @Description: 短地址后台管理
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <hiredis/hiredis.h>
#include <time.h>
#include "url_manage.h"
// 面板展示
void show(int client_fd)
{
    char str[] = "=================短地址服务===================\n1. 生成短地址\n2. 解析短地址\n3. 数据显示\n4. 统计信息\n5. 退出程序\n=============================================\n请选择操作: ";
    write(client_fd, str, sizeof(str));
}

// 生成短地址：唯一ID、有效期、次数，并与长地址存入数据库
void createTinyURL(int client_fd)
{
    char str[] = ("-----------------生成短地址--------------------\n请输入需要缩短的长地址: \n");
    write(client_fd, str, sizeof(str));
    char url[64] = {0};
    read(client_fd, url, sizeof(url));

    char tiny_url[32] = {'0', '0', '0', '0', '0'};
    int i = 4;
    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "incr url_id");
    freeReplyObject(reply);

    reply = redisCommand(conn, "get url_id");
    int id = atoi(reply->str);
    freeReplyObject(reply);
    char c;
    while (id)
    {
        int tem = id % 62;
        if (tem > 35)
        {
            tem -= 36;
            c = tem + 'A';
        }
        else if (tem > 9)
        {
            tem -= 10;
            c = tem + 'a';
        }
        else
        {
            c = tem + '0';
        }
        tiny_url[i] = c;
        i--;
        id /= 62;
    }
    // printf("生成的短地址为：ak.cn/%s\n", tiny_url);
    char respond[64];
    sprintf(respond, "生成的短地址为：ak.cn/%s\n", tiny_url);
    write(client_fd, respond, sizeof(respond));

    // 获取当前时间，并转为字符串
    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", localtime(&now));

    // 存入短地址、长地址、创建时间、有效期、访问次数
    reply = redisCommand(conn, "hset ak.cn/%s url %s create_time %s days 7 num 0", tiny_url, url, time_str);
    freeReplyObject(reply);
    redisFree(conn);
}

// 创建数据库连接
redisContext *connectRedis()
{
    redisContext *conn = redisConnect("127.0.0.1", 6379);
    if (conn->err)
    {
        printf("connection error\n");
        redisFree(conn);
        return 0;
    }

    return conn;
}

// 解析短地址
void analyzeTinyURL(int client_fd)
{
    char str[] = "-----------------解析短地址--------------------\n请输入需要解析的短地址: ";
    write(client_fd, str, sizeof(str));
    char tiny_url[64];
    read(client_fd, tiny_url, sizeof(tiny_url));

    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "hget %s url", tiny_url);
    // printf("解析短地址成功,原地址为：%s\n", reply->str);
    char respond[1024];
    sprintf(respond, "解析短地址成功,原地址为：%s\n", reply->str);
    write(client_fd, respond, sizeof(respond));
    freeReplyObject(reply);

    // 每次查询访问，访问次数加1
    redisCommand(conn, "hincrby %s num 1", tiny_url);
    redisFree(conn);
}

// 短地址、长地址、有效期，数据显示
void dataDisplay(int client_fd)
{
    char str[] = "-----------------数据显示--------------------\n短地址\t\t原地址\t\t\t创建时间\t\t有效期\n";
    write(client_fd, str, sizeof(str));

    char respond[1024];
    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "keys *");
    redisReply *reply_row;

    for (int i = 0; i < reply->elements; i++)
    {
        if (reply->element[i]->len > 6)
        {
            reply_row = redisCommand(conn, "hvals %s", reply->element[i]->str);
            // reply->element[i]->str      短地址名
            // reply_row->element[0]->str  原地址名
            // reply_row->element[1]->str  创建时间
            // reply_row->element[2]->str  有效期
            sprintf(respond, "%s\t%s\t%s\t%5s\t\n", reply->element[i]->str, reply_row->element[0]->str,
                    reply_row->element[1]->str, reply_row->element[2]->str);
            write(client_fd, respond, sizeof(respond));
            // usleep(10000);
            memset(respond, 0, sizeof(respond));
        }
    }
    freeReplyObject(reply);
    redisFree(conn);
}

// 对显示数据进行操作，删除短地址、增长有效期
void operationTinyURL(int client_fd)
{
    int flag = 0;
    dataDisplay(client_fd);
    while (1)
    {
        if (flag == 1)
        {
            dataDisplay(client_fd);
        }
        char str[] = "可选择操作: \n\t1. 删除短地址\n\t2. 开通会员~增长有效期\n\t3. 返回\n请选择操作: ";
        write(client_fd, str, sizeof(str));

        char action[64] = {0};
        read(client_fd, action, sizeof(action));
        if (strlen(action) == 0)
        {
            read(client_fd, action, sizeof(action));
        }
        switch (action[0])
        {
        case '1':
            deleteTinyURL(client_fd);
            break;
        case '2':
            riseURLTime(client_fd);
            break;
        case '3':
            return;
        default:
            printf("请选择正确操作数(1-3)\n");
            break;
        }
        flag = 1;
    }
}

// 统计数据信息，短地址、被访问次数
void statisticalInformation(int client_fd)
{
    char str[] = "-----------------统计信息--------------------\n短地址\t\t访问次数\n";
    write(client_fd, str, sizeof(str));

    char respond[128];
    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "keys *");
    redisReply *reply_row;

    for (int i = 0; i < reply->elements; i++)
    {
        if (reply->element[i]->len > 6)
        {
            reply_row = redisCommand(conn, "hvals %s", reply->element[i]->str);
            // reply->element[i]->str      短地址名
            // reply_row->element[3]->str  访问次数
            sprintf(respond, "%s\t%5s\t\n", reply->element[i]->str, reply_row->element[3]->str);
            write(client_fd, respond, sizeof(respond));
            memset(respond, 0, sizeof(respond));
        }
    }
    freeReplyObject(reply);
    redisFree(conn);
}

// 删除短地址
void deleteTinyURL(int client_fd)
{
    char str[] = "请输入需要删除的短地址: \n";
    write(client_fd, str, sizeof(str));

    char tiny_url[64];
    read(client_fd, tiny_url, sizeof(tiny_url));

    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "del %s", tiny_url);
    // printf("删除成功~\n");
    char respond[] = "删除成功~\n";
    write(client_fd, respond, sizeof(respond));

    freeReplyObject(reply);
    redisFree(conn);
}

// 增长短地址有效期
void riseURLTime(int client_fd)
{
    char str[] = "请输入需要增长有效期的短地址: ";
    write(client_fd, str, sizeof(str));

    char tiny_url[64];
    read(client_fd, tiny_url, sizeof(tiny_url));

    int days;
    char str2[] = "请输入需要增长天数: ";
    write(client_fd, str2, sizeof(str2));

    char s_day[32];
    read(client_fd, s_day, sizeof(s_day));
    days = atoi(s_day);

    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "hincrby %s days %d", tiny_url, days);

    freeReplyObject(reply);
    redisFree(conn);
}

// 根据短地址获得长地址
char * requireLongURL(char *tiny_url){
    redisContext *conn = connectRedis();
    redisReply *reply = redisCommand(conn, "hget %s url", tiny_url);
    
    // 每次查询访问，访问次数加1
    redisCommand(conn, "hincrby %s num 1", tiny_url);
    redisFree(conn);
    return reply->str;
}