#!/bin/sh
MOD_PATH=`pwd`
cd /root/SRC/nginx-1.5.7
gmake clean
./configure --prefix=/usr/local/nginx/ --with-debug --add-module=$MOD_PATH
gmake ; gmake install
killall nginx
/usr/local/nginx/sbin/nginx
tail -f /usr/local/nginx/logs/error.log
