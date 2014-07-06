#!/usr/local/ruby/bin/ruby
# Torden TinyURL
#
# @license : GPLv3, http://www.gnu.org/licenses/gpl.html
# @author : torden <https://github.com/torden/>

require "redis" # gem install redis
require "set"
require "kyotocabinet" # http://fallabs.com/kyotocabinet/rubypkg/kyotocabinet-ruby-1.32.tar.gz
include KyotoCabinet

def numberfmt(number, delimiter=",", separator=".")
    begin
        parts = number.to_s.split('.')
        parts[0].gsub!(/(\d)(?=(\d\d\d)+(?!\d))/, "\\1#{delimiter}")
        parts.join separator
    rescue
        number
    end
end

starttime = Time.now

begin
    redis = Redis.new(:path => "/tmp/redis.sock")
    pong = redis.ping
    if "PONG" != pong
        STDERR.print "Redis Connection Not Vaild"
        exit
    end
rescue Exception => ex
    STDERR.print "Failure Redis Connection [ERROR] {%s} : " % "/tmp/redis.sock"
    puts ex
    exit
end


kcDb = DB::new
unless kcDb.open('tdtinyurl_kcdb.kch', DB::OWRITER | DB::OCREATE)
    STDERR.printf("Failure make a KCH db [ERROR] : %s\n", kcDb.error)
    exit
end

print "\033[95mConverting Redis Data to KCH\033[0m\n"

keylist = redis.keys("*")
keytotalcnt = keylist.size()

print "\033[94mRedis Data into KyotoCabinet\033[0m (\033[1m\033[92m", numberfmt(keytotalcnt) ,"\033[0m)\n"

progress = 0
failurecnt = 0
vaildckset =  Set.new
for key in keylist
    vals = redis.hgetall(key)

    progress += 1
    if true == vals.key?('secure') && true == vals.key?('url')

        vaildckset.add(key)

        addval = vals['secure'] + 7.chr + vals['url']
        unless kcDb.set(key, addval)
            STDERR.printf("set error: %s\n", kcDb.error)
            failurecnt += 1
        end
        STDOUT.printf("\r[%s/%s] %s", numberfmt(keytotalcnt), numberfmt(progress), key.strip)
        STDOUT.flush
    end
end


STDOUT.print("\n")
STDOUT.printf("\033[94mComplete Converting\033[0m (\033[1m\033[92m%s\033[0m)\n", numberfmt(progress));
STDOUT.printf("\033[94mCheck Validation KCH\033[0m");

ckprogress = 0
ckfail = 0
for key in vaildckset
    unless val = kcDb.get(key)
        STDOUT.printf("\r[%s/%s] FAILED %s\n", numberfmt(progress), numberfmt(ckprogress), key.strip)
        ckfail += 1
    else
        STDOUT.printf("\r[%s/%s] %s : %s", numberfmt(progress), numberfmt(ckprogress), key.strip, val.strip )
    end

    ckprogress += 1
end

STDOUT.print("\n")
STDOUT.printf("Total Count of Redis Data : %s\n", numberfmt(keytotalcnt))
STDOUT.printf("Total Count of Added KyotoCabinet Data : %s\n", numberfmt(progress))
STDOUT.printf("Elapsed ms : %s\n" , numberfmt((Time.now - starttime) * 1000.0))
STDOUT.printf("\033[92mAdded : %s%% complete\033[0m\n" , numberfmt(progress/keytotalcnt*100))
STDOUT.printf("\033[95mFailure Count \033[0m : %s\n", numberfmt(failurecnt))

# footprint result of redis data into kch
STDOUT.print("\033[94m\nResult of Redis to KCH\033[0m\n");
STDOUT.printf("\033[92mAdded : %s%% complete\033[0m\n", (progress/keytotalcnt*100));
STDOUT.printf("\033[95mFailure Count \033[0m: %s\n", numberfmt(failurecnt));

# footprint result of validation about kch
STDOUT.print("\033[94m\nResult of Vaildation about KCH\033[0m\n");
STDOUT.printf("\033[92mAdded : %s%% complete\033[0m\n", (ckprogress/progress*100));
STDOUT.printf("\033[95mFailure Count \033[0m: %s\n", numberfmt(ckfail));


unless kcDb.close
  STDERR.printf("kch db close error: %s\n", db.error)
end

begin
    redis.quit()
rescue Exception => ex
    STDERR.print "Failure Redis Closing [ERROR] {%s} : " % "/tmp/redis.sock"
    puts ex
    exit
end

