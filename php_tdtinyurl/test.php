#!/usr/local/php-fpm/bin/php
<?php

$sUrl = 'http://www.yahoo.com/'.time().'/'.uniqid();

print "\n\n";
$sKey = tdtinyurl_set_url($sUrl, TDTINYURL_MODE_SECUER_TUNNELING);
print 'tdtinyurl_set_url : '. $sKey ."\n";

$sUrl = tdtinyurl_get_url($sKey);
print 'tdtinyurl_get_url : '. $sUrl."\n";

$mRet = tdtinyurl_set_rid($sKey, time());
print 'tdtinyurl_set_rid : '. $mRet."\n";

$sRid = tdtinyurl_get_rid($sKey);
print 'tdtinyurl_get_rid : '. $sRid ."\n";


print "\n\n";
?>
