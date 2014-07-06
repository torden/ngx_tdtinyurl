#!/bin/bash
JAVA_HOME=/usr/
CP=".:./jedis-2.1.0.jar:./kyotocabinet.jar"
$JAVA_HOME/bin/javac -cp $CP RedisToKch.java ; time $JAVA_HOME/bin/java -Djava.library.path=.:/usr/local/kyotocabinet-java/lib/ -cp $CP RedisToKch ./tdtinyurl_kcdb.kch localhost
