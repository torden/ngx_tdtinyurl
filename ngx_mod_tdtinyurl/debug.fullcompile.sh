#!/bin/sh
cd /root/SRC/nginx-1.5.2
gmake clean
./configure --prefix=/usr/local/nginx/ --with-debug --add-module=./
#read -p "Continue (y/n)?" choice
#case "$choice" in 
#  y|Y ) gmake;;
#  n|N ) exit;;
#  * ) exit;;
#esac
#read -p "Continue (y/n)?" choice
#case "$choice" in 
#  y|Y ) gmake install;;
#  n|N ) exit;;
#  * ) exit;;
#esac

gmake ; gmake install
killall nginx
/usr/local/nginx/sbin/nginx
tail -f /usr/local/nginx/logs/error.log
