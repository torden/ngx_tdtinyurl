#!/usr/bin/python -O
# tdtinyurl : redis data convert to kyotocabinet sample code
# @license : GPLv3, http://www.gnu.org/licenses/gpl.html
# @author : torden <https://github.com/torden/>

#redis python module : https://github.com/andymccurdy/redis-py/blob/master/redis/client.py
import redis
# kytocabinet python module : http://fallabs.com/kyotocabinet/pythonlegacypkg/
from kyotocabinet import *
import sys,time

def set_print(printstr):
   sys.stdout.write(printstr)
   sys.stdout.flush()

oRedisCon = redis.Redis(unix_socket_path='/tmp/redis.sock')

dbfileName = "tdtinyurl_kcdb.kch"

kcdb = DB()

if not kcdb.open(dbfileName, DB.OWRITER | DB.OCREATE | DB.OTRUNCATE):
    print >>sys.stderr, 'open error: ' + str(kcdb.error())
    raise SystemExit

aList = oRedisCon.keys('*')
nListCnt = len(aList)
ckDict = {}

set_print("\033[95mConverting Redis Data to KCH\033[0m\n")
for sKey in aList:
    if 'hash' == oRedisCon.type(sKey):
        if True == oRedisCon.hgetall(sKey).has_key('url') and True == oRedisCon.hgetall(sKey).has_key('secure'):

            sVal = oRedisCon.hgetall(sKey)['secure'] + chr(7) + oRedisCon.hgetall(sKey)['url']

            if not kcdb.set(sKey, sVal):
                print >>sys.stderr, "set error: " + str(kcdb.error())
                kcdb.close();
            ckDict[sKey] = sVal
            set_print(sKey)
            set_print(" - ")
            set_print(sVal)
            set_print("\n")

if not kcdb.close():
    print >>sys.stderr, 'close error: ' + str(kcdb.error())

set_print("="*70)
set_print("\n")

set_print("\033[94mComplete Converting\033[0m (\033[1m\033[92m"+str(len(ckDict))+"\033[0m)\n")
set_print("\033[94mCheck Validation KCH\033[0m\n")

kcdb = DB()
if not kcdb.open(dbfileName, DB.OREADER):
    print >>sys.stderr, 'open error: ' + str(kcdb.error())
    raise SystemExit

failCnt = 0
for sKey in ckDict.keys():
    set_print("Check : ")
    set_print(sKey)
    set_print(" - ")
    sVal = kcdb.get(sKey)
    if not sVal:
        set_print("\033[93mFAIL\033[0m\n")
        failCnt = failCnt+1
    else:
        if sVal != ckDict[sKey]:
            set_print("\033[93mNot Valid\033[0m\n")
            failCnt = failCnt+1
        else:
            set_print("\033[92mValid\033[0m\r")

if not kcdb.close():
    print >>sys.stderr, 'close error: ' + str(kcdb.error())

if 0 == failCnt:
    set_print("\033[92m\nDONE Enjoy\033[0m\n\n")
else:
    set_print("\033[92m\nOOPS "+failCnt+" failed, your YA-GEUN\033[0m\n\n")

