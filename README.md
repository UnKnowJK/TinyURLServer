# TinyURLServer
高性能短地址服务系统
该系统利用Redis的高性能和数据结构优势，能够将长URL转换为短URL，以适应短信营销、社交媒体分享和网络广告等场景中对URL长度的限制。
该系统除了基本的URL缩短、通过短地址快速 访问原始长地址功能，系统还提供短地址管理、数据统计、访问分析等附加功能，以提升用户体验。
## 短地址管理
使用方法：
1. 编译
编译时确保安装必要的库
服务器：
```shell
gcc TinyURL_server.c url_manage.c -o TinyURL_server -L /usr/local/lib/ -lhiredis
```
2. 运行
```shell
./TinyURL_server
```
客户端：
```shell
 gcc URL_client.c -o URL_client
```
2. 运行
```shell
./URL_client
```

界面及提供操作如下：
```txt
请输入选项：
-------------------------------
        1. 生成短地址，同时记录有效期、访问次数、创建时间、访问渠道（ip）
        2. 解析短地址
        3. 数据显示
            - 删除短地址
            - 开通会员~增长有效期
        4. 统计信息
        5. 退出程序
-------------------------------
```
## 重定向
本服务，实现了简单http头部解析，以提供短地址到长地址的重定向功能
使用方法：
> 浏览器输入http://youaddress:port/short_url
