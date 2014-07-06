#!/bin/env python
"""
 * Torden TinyURL
 *
 * @license : GPLv3, http://www.gnu.org/licenses/gpl.html
 * @author : torden <https://github.com/torden/>
"""
from os import system
from sys import stdout

for i in range(1,100000):
    system("/usr/local/php-fpm/bin/php test.php > /dev/null 2>&1")
    stdout.write("\r")
    stdout.write("[%s] Test Data into Redis via php api" % i)
    stdout.flush()

