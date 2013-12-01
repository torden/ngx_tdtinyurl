#!/bin/sh
PHP_HOME="/usr/local/php-fpm/"
APACHE_LOG_HOME="/usr/local/httpd/logs/"
gmake clean
$PHP_HOME/bin/phpize --clean
$PHP_HOME/bin/phpize
find $PHP_HOME -name "tdtinyurl.so" -exec rm -rf {} \;
./configure --with-php-config=$PHP_HOME/bin/php-config --with-uqid=/usr/local/ --with-hiredis=/usr/local/hiredis/
gmake 
ldd modules/tdtinyurl.so
gmake install
cp -rf tdtinyurl.ini $PHP_HOME/etc/
killall php-fpm ; $PHP_HOME/sbin/php-fpm
#tail -f $APACHE_LOG_HOME/error_log
$PHP_HOME/bin/php ./test.php
#export USE_ZEND_ALLOC=0
#export ZEND_DONT_UNLOAD_MODULES=1
export USE_ZEND_ALLOC=1
export ZEND_DONT_UNLOAD_MODULES=0
export MALLOC_TRACE="/tmp/leak.out"
/usr/local/valgrind/bin/valgrind  --num-callers=30 --tool=memcheck --log-file=log.valgrind.use_zend_alloc.log -v /usr/local/php-fpm/bin/php ./test.php

export USE_ZEND_ALLOC=0
export ZEND_DONT_UNLOAD_MODULES=1
/usr/local/valgrind/bin/valgrind  --num-callers=30 --tool=memcheck --log-file=log.valgrind.no_use_zendalloc.log -v /usr/local/php-fpm/bin/php ./test.php
