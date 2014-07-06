# Just! a TidyURL Example

---

A Simple TinyURL Full Stack? Open Source, Including Nginx Module, PHP Extendtion, Scripts.

# Requirement (Mandatory)

* Linux, gcc 3.x or Higher, gmake
* Nginx (1.0.x or Higher) : http://nginx.org/en/download.html
* Hiredis Latest version : https://github.com/redis/hiredis
* KyotoCabinet (1.2.30 or higher) : http://fallabs.com/kyotocabinet/pkg/
* Valgrind Latest : http://valgrind.org/
* php 5.x or higher : http://kr1.php.net/downloads.php
* Redis 2.0x or higher, *RECOMMENDED* 2.6.16 or Higher : http://download.redis.io/releases/redis-2.8.2.tar.gz
* SQLite 3.0x or higher : http://www.sqlite.org/download.html
* Phantomjs Latest verion (Embeded) : http://phantomjs.org/

# Requirement (Optional)

* Python 2.3 or Higher : https://www.python.org/
* Ruby 2.0x or Higher : https://www.ruby-lang.org/
* JDK 1.6 or Higher : http://www.oracle.com/technetwork/java/index.html

# Tested

* Python 2.3/2.4
* PHP 5.x
* JDK 1.6.X
* Nginx 1.0 ~ 1.6.x

# Support

* Support Convert Redis Data to KytoCabinet DBM file tools  (Java, Ruby, Python)
* Support PHP 5.x Extension
* Support nginx 1.x

# Data Structure 

## Redis

* First, the ngx module doing find the tinyurl key in Redis
* Primary Lookup Data Storage

|Redis|
| ------------- | ------------- | ------------- |
|key|Tinyurl Key|tduqid generated a sequence number|
|time|Expire Time|TIMESTAMP|
|url|Long URL|Original URL|
|api|API Verion||
|secure|Secure Mode|0 : Normal Redirection, 1 : Web Page Fetching and Rendering|
|rid|Related ID|Unique Sequence ID|

## Kyotocabinet DBM

* If the ngx module can't find the tinyurl key in Redis OR connecting fail
* Secondary Lookup Data Storage, For Failover

|Kyotocabinet DBM|
| ------------- | ------------- | ------------- |
|key|Tinyurl Key|tduqid generated a sequence number|
|url|Long URL|Original URL|
|secure|Secure Mode|0 : Normal Redirection, 1 : Web Page Fetching and Rendering|


## SQLITE

* Main Data Storage and Easy Build a TinyURL Web Interface

|TDTINYURL|Main Data|
| ------------- | ------------- | ------------- |
|rid|INTEGER|PRIMARY KEY AUTOINCREMENT|
|tinyurl|text|not null|
|longurl|text|not null|
|time|integer|not null|
|secure|integer|not null|
|hits|integer|not null|
|regdate|datetime|default current_timestamp|

|TDTINYURL_LOG|Logging|
| ------------- | ------------- | ------------- |
|tinyurl|text|not null|
|agent|text|not null|
|ipaddr|text|not null|
|referer|text||
|regdate|datetime|default current_timestamp|
|FOREIGN KEY(tinyurl) REFERENCES TINYURL(tinyurl)||||

# Installation

* Prepared RPM packages
* Required : zip, pcre , python , jdk , valgrind , openssl

```bash
yum groupinstall "Develoment Tools" -y
yum install openssl.i686 openssl.x86_64 openssl-static.x86_64 openssl-devel.x86_64 openssl-devel.i686 -y
yum install valgrind glibc* -y
yum install zlib.i686 zlib.x86_64 zlib-devel.i686 zlib-devel.x86_64 zlib-static.x86_64 -y
yum install libxml2.i686 libxml2.x86_64 libxml2-devel.i686 libxml2-devel.x86_64 -y
yum install libcurl-devel.i686 libcurl-devel.x86_64 curl.x86_64 libcurl.i686 libcurl.x86_64 -y
yum install pcre-devel.i686 pcre-devel.x86_64 pcre-static.x86_64 pcre.i686 pcre.x86_64 -y
yum install python-devel.x86_64 python-devel.i686 -y
yum install java-1.7.0-openjdk-devel.x86_64 java-1.7.0-openjdk-src.x86_64 -y
yum install gcc gcc-g++* vim* -y

```


* Install tduqid

```bash
cd tduqid
gmake
```

* Install Hiredis

```bash
git clone https://github.com/redis/hiredis
cd hiredis
PREFIX=/usr/local/hiredis gmake
PREFIX=/usr/local/hiredis gmake install
```

* Install KyotoCabinet

```bash
wget http://fallabs.com/kyotocabinet/pkg/kyotocabinet-1.2.76.tar.gz
tar xvzf kyotocabinet-1.2.76.tar.gz
cd kyotocabinet-1.2.76
./configure --prefix=/usr/local/kyotocabinet --enable-static --enable-devel
gmake
gmake install
```

* Install Nginx

```bash
mkdir /root/SRC/
cd /root/SRC/
wget http://nginx.org/download/nginx-1.5.7.tar.gz
tar xvzf nginx-1.5.7.tar.gz
```

* Redis

```bash
cd /root/SRC/
wget http://download.redis.io/releases/redis-2.8.2.tar.gz
tar xvzf redis-2.8.2.tar.gz
cd redis-2.8.2
PREFIX=/usr/local/redis gmake
PREFIX=/usr/local/redis gmake install
```

* SQLite3

```bash
cd /root/SRC/
wget http://www.sqlite.org/2014/sqlite-autoconf-3080403.tar.gz
tar xvzf sqlite-autoconf-3080403.tar.gz
cd sqlite-autoconf-3080403
./configure --prefix=/usr/local/sqlite3 --enable-threadsafe --enable-readline --enable-dynamic-extensions --with-pic --enable-static --enable-shared
gmake
gmake install
```

* PHP(fpm mode)

```bash
wget http://kr1.php.net/get/php-5.5.12.tar.gz/from/this/mirror
tar xvzf mirror
cd php-5.5.12

./configure \
--prefix=/usr/local/php-fpm \
--enable-fpm \
--with-zlib=/usr/ \
--enable-bcmath \
--with-gettext=/usr/ \
--enable-mbstring \
--enable-sysvmsg \
--enable-sysvsem \
--enable-sysvshm \
--with-config-file-scan-dir=/usr/local/php-fpm/etc \
--with-config-file-path=/usr/local/php-fpm/etc \
--with-curl \
--enable-pcntl \
--disable-debug \
--enable-zip \
--enable-opcache \

gmake
gmake install

cp -rf php.ini-production /usr/local/php-fpm/etc/php.ini
cp -rf sapi/fpm/php-fpm.conf /usr/local/php-fpm/etc/

/usr/local/php-fpm/sbin/php-fpm
```

* phpRedis

```bash
git clone https://github.com/nicolasff/phpredis.git
cd phpredis/
/usr/local/php-fpm/bin/phpize
./configure --enable-redis --with-php-config=/usr/local/php-fpm/bin/php-config
gmake
gmake install

cp -rf rpm/redis.ini /usr/local/php-fpm/etc/
killall php-fpm
/usr/local/php-fpm/sbin/php-fpm

/usr/local/php-fpm/bin/php -i | fgrep Redis

Redis Support => enabled
Redis Version => 2.2.5
```

# Install tinyurl module with Nginx

## generally

```bash
cd /nginx-1.5.*
./configure --prefix=/usr/local/nginx/ --with-debug --add-module=CLONE_PATH/ngx_mod_tdtinyurl/
gmake
gmake install

cp -rf ./ngx_mod_tdtinyurl/tdtinyurl_nginx.conf /usr/local/nginx/conf/nginx.conf
killall nginx ; /usr/local/nginx/sbin/nginx; tail -f /usr/local/nginx/logs/error.log
```


## Nginx Config directive

|directive|default flag|description|
| ------------- | ------------- | ------------- |
|tdtinyurl|off| on or off|
|tdtinyurl_work_mode|rl|rl is first lookup to redis and if redis failure, lookup to kyotocabinet dbm file|
|tdtinyurl_dbm_path|N/A|kyotocabinet dbm file full path|
|tdtinyurl_redis_connnect_mode|uds|uds or tcp|
|tdtinyurl_redis_uds_path|N/A|redis unix domain socket file path, default /tmp/redis.socket|
|tdtinyurl_redis_ip_addr|N/A|127.0.0.1 or localhost or xxxx.com|
|tdtinyurl_redis_port|6379|redis service port when set tcp mode to tdtinyurl_redis_connnect_mode|

* Sample Config

```config
http {
  include             mime.types;
  default_type        application/octet-stream;
  sendfile            on;
  tcp_nopush          on;
  keepalive_timeout   65;
  server {
      listen          80;
      server_name     localhost;
      access_log      logs/host.access.log;

      # tdtinyurl
      location / {
          root                                html;
          index                               index.html index.htm;
          tdtinyurl                           on;
          tdtinyurl_work_mode                 "lr"; # lr is 1) Local DBM 2) Redis , rl = 1) Redis 2) Local
          tdtinyurl_dbm_path                  "/usr/local/data/tdtinyurl/tdtinyurl_kcdb.kch";
          tdtinyurl_redis_connnect_mode       uds ; # tcp or uds, recommended uds
          tdtinyurl_redis_uds_path            /tmp/redis.sock;
          tdtinyurl_redis_ip_addr             "127.0.0.1";
          tdtinyurl_redis_port                "6379";
      }

      # tdtinyurl raise below codes
      error_page   502 503 403 406  /fail.html;
      error_page   404 /not.html;
      error_page   500 /fail.html;
      error_page   504 /timeout.html;
      location = /not.html { root   html; }
      location = /fail.html { root   html; }
      location = /timeout.html { root   html; }
  }
}
```

## debugging

### nginx.conf

* edit nginx.conf
* error_log  logs/error.log debug;
* restarting nginx

# Install tinyurl php module

## generally

```bash

export PHP_HOME=/usr/local/php

cd php_tdtinyurl

$PHP_HOME/bin/phpize

./configure --with-php-config=$PHP_HOME/bin/php-config --with-uqid=/usr/local/ --with-hiredis=/usr/local/hiredis/

gmake
gmake install

cp -rf tdtinyurl.ini $PHP_HOME/etc/tdtinyurl.ini

vi $PHP_HOME/etc/tdtinyurl.ini

#restart your web server

```

## quick

```bash
vi BUILD.sh
./BUILD.sh
```

## testing

* $PHP_HOME/bin/php ./test.php


```bash
for i in 1 2 3
> do
> /usr/local/php-fpm/bin/php ./test.php
> done

tdtinyurl_set_url : xrizSizShyPhyPgxOgxO
tdtinyurl_get_url : http://www.yahoo.com
tdtinyurl_set_rid : 1
tdtinyurl_get_rid : 1386096535

tdtinyurl_set_url : aS408b09beDjL2jK1iK1
tdtinyurl_get_url : http://www.yahoo.com
tdtinyurl_set_rid : 1
tdtinyurl_get_rid : 1386096535

tdtinyurl_set_url : Mj656df79azH6nG6nE5m
tdtinyurl_get_url : http://www.yahoo.com
tdtinyurl_set_rid : 1
tdtinyurl_get_rid : 1386096535
```

# API Information

## tdtinyurl_set_url

```php
url into redis and return tinyurl key string

sting tdtinyurl_set_url(string $sURL, [cActionMode = TDTINYURL_LIVE, int $nExpireTime = -1, cWorkingMode = TDTINYURL_MODE_DEFAULT_REDIRECTION])
$sURL: Url ex) http://www.yahoo.com


cActionMode : TDTINYURL_LIVE is working mode and default action mode value, TDTINYURL_GET_ONLY_KEY is get only tinyurl key

$nExpireTime : expire time, if set this argument, this record will be destroy after (current time + your into expire time)m

cWorkingMode : TDTINYURL_MODE_DEFAULT_REDIRECTION is default tinyurl service mode , TDTINYURL_MODE_SECUER_TUNNELING is web page fetching and printing mode

```

```php
<?php
$sKey = tdtinyurl_set_url($sUrl);
print 'tdtinyurl_set_url : '. $sKey ."\n";
?>
```

## tdtinyurl_get_url

* get a url by tinyurl key string
```php
<?php
$sUrl = tdtinyurl_get_url($sKey);
print 'tdtinyurl_get_url : '. $sUrl."\n";
?>
```

## tdtinyurl_set_rid

* set relation id to redis , related is means database primary key or uniq id

```php
<?php
$mRet = tdtinyurl_set_rid($sKey, time());
print 'tdtinyurl_set_rid : '. $mRet."\n";
?>
```

## tdtinyurl_get_rid

* get a tinyurl key string by url

```php
<?php
$sRid = tdtinyurl_get_rid($sKey);
print 'tdtinyurl_get_rid : '. $sRid ."\n";
?>
```

---
moved from bitbucket.org
