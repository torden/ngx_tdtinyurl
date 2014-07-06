#!/bin/sh
MOD_PATH=`pwd`
cd /root/SRC/nginx-1.*
gmake clean
killall nginx
rm -rf /usr/local/nginx/sbin/nginx

./configure --prefix=/usr/local/nginx/ --with-debug --add-module=$MOD_PATH
gmake ; gmake install

echo "" > /usr/local/nginx/logs/error.log
/usr/local/nginx/sbin/nginx
tail -f /usr/local/nginx/logs/error.log
