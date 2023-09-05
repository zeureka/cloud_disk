#!/bin/bash
echo
echo ========== fastdfs ==========
./fastdfs.sh stop
./fastdfs.sh all

echo
echo ========== fastCGI ==========
./fcgi.sh

echo
echo ========== nginx ==========
./nginx.sh stop
./nginx.sh start

echo
echo ========== redis ==========
./redis.sh stop
./redis.sh start
