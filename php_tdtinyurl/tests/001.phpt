--TEST--
Check for tinyurl presence
--SKIPIF--
<?php if (!extension_loaded("tinyurl")) print "skip"; ?>
--FILE--
<?php 
print '<h1>Bucket API Test</h1><br>';
?>
--EXPECT--
tinyurl extension is available
