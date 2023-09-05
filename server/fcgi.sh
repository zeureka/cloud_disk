#!/bin/bash

# 定义两个变量，用来判断启动CGI进程或杀死正在运行的CGI进程
START=1
STOP=1

case $1 in
    start)
        START=1
        STOP=0
        ;;
    stop)
        START=0
        STOP=1
        ;;
    "")
        STOP=1
        START=1
        ;;
    *)
        STOP=0
        START=0
        ;;
esac

# 杀死正在运行的 CGI 进程
if [ "${STOP}" -eq 1 ]; then
    # 登录
    kill -9 $(ps aux | grep "./bin_cgi/login" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 注册
    kill -9 $(ps aux | grep "./bin_cgi/enroll" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 上传文件
    kill -9 $(ps aux | grep "./bin_cgi/upload" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # MD5 秒传
    kill -9 $(ps aux | grep "./bin_cgi/md5" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 我的文件
    kill -9 $(ps aux | grep "./bin_cgi/myfiles" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 分享删除文件
    kill -9 $(ps aux | grep "./bin_cgi/dealfile" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 共享文件列表
    kill -9 $(ps aux | grep "./bin_cgi/sharefiles" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
    # 共享文件download字段处理、取消分享、转存文件
    kill -9 $(ps aux | grep "./bin_cgi/dealsharefile" | grep -v grep | awk '{print $2}') > /dev/null 2>&1
	# 我的信息
    kill -9 $(ps aux | grep "./bin_cgi/myinfo" | grep -v grep | awk '{print $2}') > /dev/null 2>&1

    echo "CGI 程序已经成功关闭 bye~ ..."
fi

# 启动 CGI 进程
if [ "${START}" -eq 1 ];then
    # 登录
    echo -n "Login: "
    spawn-fcgi -a 172.18.46.97 -p 10000 -f ./bin_cgi/login
    # 注册
    echo -n "Enroll: "
    spawn-fcgi -a 172.18.46.97 -p 10001 -f ./bin_cgi/enroll
    # 上传文件
    echo -n "Upload File: "
    spawn-fcgi -a 172.18.46.97 -p 10002 -f ./bin_cgi/upload
    # MD5秒传
    echo -n "MD5: "
    spawn-fcgi -a 172.18.46.97 -p 10003 -f ./bin_cgi/md5
    # 我的文件
    echo -n "My Files: "
    spawn-fcgi -a 172.18.46.97 -p 10004 -f ./bin_cgi/myfiles
    # 删除共享文件
    echo -n "Deal File: "
    spawn-fcgi -a 172.18.46.97 -p 10005 -f ./bin_cgi/dealfile
    # 共享文件列表
    echo -n "Share File List: "
    spawn-fcgi -a 172.18.46.97 -p 10006 -f ./bin_cgi/sharefiles
    # 共享文件download字段处理、取消分享、转存文件
    echo -n "Deal Share File: "
    spawn-fcgi -a 172.18.46.97 -p 10007 -f ./bin_cgi/dealsharefile
	# 我的信息
    echo -n "My Information: "
    spawn-fcgi -a 172.18.46.97 -p 10008 -f ./bin_cgi/myinfo

    echo "CGI 程序已经成功启动 ^_^ ..."
fi
