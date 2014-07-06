/**
 * Torden TinyURL
 *
 * @license : GPLv3, http://www.gnu.org/licenses/gpl.html
 * @author : torden <https://github.com/torden/>
 */
import java.util.*;
import java.io.File;
import java.io.FilePermission;
import java.nio.file.Paths;
import java.nio.file.FileSystems;
import java.security.AccessController;
import java.text.NumberFormat;

//Redis library : https://github.com/xetorthio/jedis
import redis.clients.jedis.Jedis; 

//KytoCabinet library : http://fallabs.com/kyotocabinet/javadoc/
import kyotocabinet.*;

class RedisToKch{

    private static Jedis oJedis;
    private static kyotocabinet.DB oKcDB = new kyotocabinet.DB();

    private static Set<String> soKeys;
    private static Iterator ioIterKeys;
    private static NumberFormat oNf = NumberFormat.getNumberInstance();

    private final static char DELIMITER = 0x07;
    private static long nAddedCnt = 0;
    private static long nProgressCnt = 0;
    private static long nTotalRedisDataCnt = 0;
    private static long nFailedAddCnt = 0;
    private static long nCkProgressCnt = 0;
    private static long nCkFailedCnt = 0;

    private static Map<String, String> moErrLines = new HashMap<String, String>();
    private static Set<String> soCkSetList = new HashSet<String>();
    private static Iterator ioIterCkKeys;

    private static long lStartTime;
    private static long lEndTime;

    private static String sDbName = "tdtinyurl_kcdb.kch";
    private static String sRedisConString = "redis://localhost:6379/";

    /**
     * test writable about kyotocabinet file path
     *
     * @param   String  kyotocabinet file path, sDbName
     * @return  boolean
     */
    private static boolean isWritablePath(String sPath) {

        try {
            AccessController.checkPermission(new FilePermission(sPath, "read,write"));
            return true;
        } catch (SecurityException e) {

            if(!oKcDB.open(sDbName, DB.OWRITER | DB.OCREATE)){ //C access function
                return false;
            }
            oKcDB.close();
            return true;
        }
    }

    /**
     * test connection to redis server
     *
     * @param   String  redis connection uri string, sRedisConString
     * @return  boolean
     */
    private static boolean isRedisConnectable(String sConString) {

        try {

            oJedis = new Jedis(sConString);
            oJedis.ping();

        } catch (redis.clients.jedis.exceptions.JedisConnectionException e) {
            System.out.printf("can not connect redis server : %s\n", e.getMessage());
            return false;
        } catch (Exception e) {
            return false;
        }

        try {
            oJedis.disconnect();
        } catch (Exception e) {
            return false; //wrong way.
        }

        return true;
    }

    /**
     * print message
     *
     * @param   String  Error Message
     * @return  void
     */
    private static void setAlertPrefix(String sMsg) {

        System.out.println("TdTinyURL Redis Data into KyotoCabinet.");
        System.out.printf("\nERROR : %s\n", sMsg);
        System.out.flush();
    }

    /**
     * Set start time for get a elapsed time
     *
     * @param   void
     * @return  void
     */
    private static void setStartTime() {

        lStartTime = new Date().getTime();
    }

    /**
     * Get Elaspsed time
     *
     * @param   void
     * @return  Time    ms
     */
    private static long getElapsedTime() {

        lEndTime = new Date().getTime();
        return (lEndTime - lStartTime);
    }

    /**
     * Argument parser
     *
     * @param   String[]    CLI argument
     * @return  void
     */
    private static void setArgumentParse(String args[]) {

        if(2 != args.length) {

            setAlertPrefix("Too few arguments.");
            System.out.println("\nOPTIONS : $JAVA_HOME/bin/java RedisToKch [KYOTO CABINET FILENAME] [REDIS CONNECTION STRING]\n");
            System.out.println("\nEXAMPLE :");
            System.out.println("\tjava RedisToKcj ./tdtinyurl_db.kcj redis://:foobar@localhost:6379/\n");
            System.out.println("\tjava RedisToKcj ./tdtinyurl_db.kcj redis://:foobar@localhost:6379/1\n");
            System.out.println("\tjava RedisToKcj ./tdtinyurl_db.kcj localhost/\n");
            System.out.flush();
            System.exit(1);
        }

        sDbName = args[0];
        sRedisConString = args[1];

        if(false == isWritablePath("./*")) {
            setAlertPrefix("Have not read/write permit : " + sDbName);
            System.exit(1);
        }
 
        if(false == isRedisConnectable(sRedisConString)) {
            setAlertPrefix("Can not connect to redis server : " + sRedisConString);
            System.exit(1);
        }
    }

    /**
     * Printing Failure list when Failure 
     *
     * @param   void
     * @return  void
     */
    private static void setFailureListPrint() {

        String sKey = null;
        String sValue = null;
        Map.Entry moEntry;

        if(0 < moErrLines.size()) {

            Iterator ioEntries = moErrLines.entrySet().iterator();

            while (ioEntries.hasNext()) {
                moEntry = (Map.Entry) ioEntries.next();

                sKey = (String)moEntry.getKey();
                sValue = (String)moEntry.getValue();

                System.out.printf("Key = %s, Message = %s\n", sKey, sValue);
                System.out.flush();
            }
        }
    }

    /**
     * Fetching all keys on redis
     *
     * @param   void
     * @return  void
     */
    private static void setRedisFetchAllKeys() {

        try {
            oJedis = new Jedis(sRedisConString);
            soKeys = oJedis.keys("*");
        } catch (redis.clients.jedis.exceptions.JedisConnectionException e) {
            System.out.printf("can not connect redis server\n%s\n", e.getMessage());
            System.exit(1);
        } catch (Exception e) {
            System.out.println(e.getMessage());
            System.exit(1);
        }
    }


    /**
     * init kch
     *
     * @param   void
     * @return  void
     */
    private static void setReadyKCH() {

        if(!oKcDB.open(sDbName, DB.OWRITER | DB.OCREATE)){
            System.err.printf("Can not open : %s\r\nError : %s\n", sDbName, oKcDB.error());
            System.exit(1);
        }
    }


    /**
     * Redis data into kch
     *
     * @param   void
     * @return  void
     */
    private static void setRedisToKCH() {

        Map<String, String> moValue;
        String sKey = null;
        String sAddVal = null;


        //init for iteration
        ioIterKeys = soKeys.iterator();

        //get count of redis all data
        nTotalRedisDataCnt = soKeys.size();

        System.out.printf("\033[94mRedis Data into KyotoCabinet\033[0m (\033[1m\033[92m%s\033[0m)\n", oNf.format(nTotalRedisDataCnt));

        //main line : redis data to kyotocabinet
        while (ioIterKeys.hasNext()) {

            sKey = ioIterKeys.next().toString();
            moValue = oJedis.hgetAll( sKey );

            soCkSetList.add(sKey);
            sAddVal = null;

            //check keys about redis data
            if(true == moValue.containsKey("secure") && true == moValue.containsKey("url")) {

                //make a added data for data into kyotocabinet
                sAddVal = moValue.get("secure") + DELIMITER + moValue.get("url");
                if(!oKcDB.set(sKey, sAddVal)) {
                    System.err.println("set error: " + oKcDB.error());
                    moErrLines.put(sKey, oKcDB.error().toString());

                } else {
                    nAddedCnt++;
                }
            } else { // error

                if(false == moValue.containsKey("secure")) {
                    moErrLines.put(sKey, "not valid, secure field not exists");
                }

                if(true == moValue.containsKey("url")) {
                    moErrLines.put(sKey, "not valid, url field not exists");
                }
            }

            nProgressCnt++;
            System.out.printf("\r[%s/%s] %s", oNf.format(nTotalRedisDataCnt), oNf.format(nProgressCnt), sKey);
            System.out.flush();
        }

        nFailedAddCnt = nTotalRedisDataCnt - nAddedCnt;

        System.out.println();
        System.out.printf("\033[94mComplete Converting\033[0m (\033[1m\033[92m%s\033[0m)\n", oNf.format(nProgressCnt));

    }


    /**
     * Vaildation KCH about added data from redis data
     *
     * @param   void
     * @return  void
     */
    private static void checkValidationKCH() {

        String sKey = null;
        String sVal = null;
 
        System.out.println("\033[94mCheck Validation KCH\033[0m");

        //validation
        ioIterCkKeys = soCkSetList.iterator();
        while (ioIterCkKeys.hasNext()) {

            sKey = ioIterCkKeys.next().toString();
            sVal = oKcDB.get(sKey);
            if(null == sVal) {
                nCkFailedCnt++;
                System.out.printf("\r[%s/%s] %s", oNf.format(nProgressCnt), oNf.format(nCkProgressCnt), sKey);
                continue;
            }

            System.out.printf("\r[%s/%s] %s : %s", oNf.format(nProgressCnt), oNf.format(nCkProgressCnt), sKey, sVal);
            System.out.flush();

            nCkProgressCnt++;
        }
    }


    /**
     * Printing final footprint
     *
     * @param   void
     * @return  void
     */
    private static void setFinalResultPrint() {

        System.out.println();

        //footer print
        System.out.printf("Total Count of Redis Data : %s\n", oNf.format(soKeys.size()));
        System.out.printf("Total Count of Added KyotoCabinet Data : %s\n", oNf.format(nAddedCnt));
        System.out.printf("Total Elapsed ms : %s\n", oNf.format(getElapsedTime()));

        // footprint result of redis data into kch
        System.out.println("\033[94m\nResult of Redis to KCH\033[0m\n");
        System.out.printf("\033[92mAdded : %s%% complete\033[0m\n", (nAddedCnt/nTotalRedisDataCnt*100));
        System.out.printf("\033[95mFailure Count \033[0m: %s\n", oNf.format(nFailedAddCnt));

        //verbose failure data
        setFailureListPrint();

        // footprint result of validation about kch
        System.out.println("\033[94m\nResult of Vaildation about KCH\033[0m\n");
        System.out.printf("\033[92mAdded : %s%% complete\033[0m\n", (nCkProgressCnt/nProgressCnt*100));
        System.out.printf("\033[95mFailure Count \033[0m: %s\n", oNf.format(nCkFailedCnt));
    }

    /**
     * finally~
     *
     * @param   void
     * @return  void
     */
    private static void setFinalClosing() {

        // closing
        if(!oKcDB.close()){
            System.err.printf("KCH db close error: %s\n", oKcDB.error());
        }

        // closing
        try {
            oJedis.disconnect();
        } catch (Exception e) {
            System.out.printf("Redis close error : %s\n", e.getMessage());
        }
    }

    // MAIN
    public static void main(String args[]) {

        //check argument
        setArgumentParse(args);

        //set start time for get elapsed time
        setStartTime();

        //print header
        System.out.println("\033[95mConverting Redis Data to KCH\033[0m");

        // open kyotocabinet db
        setReadyKCH();

        // connecting redis and get all keys
        setRedisFetchAllKeys();

        //Redis To KCH
        setRedisToKCH();

        //validation
        checkValidationKCH();

        //final footprint
        setFinalResultPrint();
 
        //closing
        setFinalClosing();
    }
}

