#!/bin/bash

# 关闭 tracker 和storage
tracker_start() {
    docker ps -f name=tracker
    if [ $? -eq 0 ]; then
        echo "tracker is already running and does not need to be started..."
    else
        docker start tracker
        if [ $? -ne 0 ];then
            echo "tracker start failed..."
        else
            echo "tracker start success..."
        fi
    fi
}

storage_start() {
    docker ps -f name=storage
    if [ $? -eq 0 ]; then
        echo "storage is already running and does not need to be started..."
    else
        docker start storage
        if [ $? -ne 0 ];then
            echo "storage start failed..."
        else
            echo "storage start success..."
        fi
    fi
}

if [ $# -eq 0 ];then
    echo "Operation:"
    echo "  start storage please input argument: storage"
    echo "  start tracker please input argument: tracker"
    echo "  start storage && tracker please input argument: all"
    echo "  stop storage && tracker input argument: stop"
    exit 0
fi

case $1 in
    storage)
        storage_start
        ;;
    tracker)
        tracker_start
        ;;
    all)
        tracker_start
        storage_start
        ;;
    stop)
        docker stop storage
        docker stop tracker
        ;;
    *)
        echo "nothing......."
esac
