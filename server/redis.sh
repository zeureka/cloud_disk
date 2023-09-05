#!/bin/bash

# 根据参数设置redis状态
if [[ $1 = "" ]]; then
    echo "place input argument:"
    echo "start: start redis server"
    echo "stop": stop redis server
    echo "status: show the redis server status"
    exit 1
fi

case $1 in
    start)
        # 判断redis server 容器是否已经启动
        docker ps -f name=redis
        if [ $? -eq 0 ]; then
            echo "Redis container is runing..."
        else
            echo "Redis container starting..."
            docker start redis
            if [ $? -eq 0 ]; then
                echo "Redis container start success!!!"
            else
                echo "Redis container start failed!!!"
            fi
        fi
        ;;
    stop)
        # 判断redis server 容器是否已经启动
        docker ps -f name=redis
        if [ $? -ne 0 ]; then
            echo "Redis container is not runing..."
            exit 1
        fi
        echo "Redis stopping..."
        docker stop redis
        if [$? -ne 0]; then
            echo "Redis container stop failed..."
        else
            echo "Redis container stop success!!!"
        fi
        ;;
    status)
        # 判断redis server 容器是否已经启动
        docker ps -f name=redis
        if [ $? -eq 0 ]; then
            echo "Redis container is running..."
        else
            echo "Redis container is not running..."
        fi
        ;;
    *)
        echo "do nothing..."
        ;;
esac

