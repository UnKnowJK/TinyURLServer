/*
 * @Author: ak
 * @Date: 2024-08-13 11:23:59
 * @LastEditors: ak
 * @LastEditTime: 2024-08-13 15:11:08
 * @FilePath: /Linux_sys_pro/0809/temp.h
 * @Description: 
 */
#include <hiredis/hiredis.h>
#include <time.h>

void show(int client_fd);
void createTinyURL(int client_fd);
redisContext *connectRedis();
void analyzeTinyURL(int client_fd);
void dataDisplay(int client_fd);
void statisticalInformation(int client_fd);
void deleteTinyURL(int client_fd);
void riseURLTime(int client_fd);
void operationTinyURL(int client_fd);
char * requireLongURL(char *tiny_url);