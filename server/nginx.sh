#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Options"
    echo "start the nginx: start"
    echo "stop the nginx: stop"
    exit 1
fi

case $1 in
    start)
        docker start fastdfs_nginx
        if [ $? -eq 0 ];then
            echo "nginx start success ..."
        else
            echo "nginx start fail ..."
        fi
        ;;
    stop)
        docker stop fastdfs_nginx
        if [ $? -eq 0 ];then
            echo "nginx stop success ..."
        else
            echo "nginx stop fail ..."
        fi
        ;;
    *)
        echo "do nothing..."
        ;;
esac

