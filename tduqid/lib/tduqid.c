/*
 * @author : torden <https://github.com/torden/>
 */
#include <tduqid.h>

static const char suffixAlnum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; //62
static const char prefixAlnum[] = "abcdefghjkmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"; //50

static unsigned long long int get_mixed_ulong(unsigned long a, unsigned long b, unsigned long c) {

    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return (unsigned long long int)c;
}

static inline unsigned long long int get_random_long(long long int seed ) {

    struct timeval tv;
    struct timezone tz;
    register unsigned long long int randseed;

    gettimeofday(&tv,&tz);
    randseed = tv.tv_usec + seed + get_mixed_ulong(clock(), time(NULL), getpid());

    gettimeofday(&tv,&tz);
    randseed = randseed + tv.tv_usec;

    srand48(randseed);
    return lrand48();
}

extern unsigned int tduqid_get_identity_grp_no(const char *pUidStr) {

    const char *pprefixAlnum = prefixAlnum;
    register unsigned int num = 0;
    unsigned int sum = 0;
    unsigned int chr1 = 0;
    unsigned int chr2 = (unsigned int)pUidStr[0];
    unsigned int chr3 = (unsigned int)pUidStr[1];

    while(0 != *pprefixAlnum) {
        chr1 = *pprefixAlnum++;
        if(chr1 == chr2) {
            sum = num;
        }

        if(chr1 == chr3) {
            sum += num;
        }

        num++;
    }

    return sum;

}

extern long int tduqid_get_str_to_number(unsigned char *pStr) {

    register unsigned int chr;
    register long int seedCal = 0;

    if(0 == *pStr) {
        return -1;
    }

    while(0 != *pStr) {

        chr = *pStr++;
        if(0 == chr) { break;}
        seedCal = seedCal + chr;
    }

    return seedCal;
}

extern const char *tduqid_get_str(char *pUidStr, unsigned int buflen, long int seedNum) {

    register long long int seedCal = 0;
    register short prefixNo = 0;
    register short prefixMoreNo = 0;
    register short suffixNo = 0;
    unsigned long long int randlong;
    size_t currentLen = 0;
    register short i = 0;
    register short moreNo = 0;
    struct timeval tv;
    struct timezone tz;

    if(TDUQID_MAX_LEN < buflen) {
        return NULL;
    }
 
    gettimeofday(&tv,&tz);

    seedCal = seedNum + tv.tv_usec;

    randlong = get_random_long(seedCal);
    prefixNo = seedCal % 50;
    prefixMoreNo = lrand48() % 50+1;

    memset(pUidStr, 0x00, TDUQID_MALLOC_LEN);
    snprintf(pUidStr, buflen, "%c%c%llx", prefixAlnum[prefixNo], prefixAlnum[prefixMoreNo], randlong);
    currentLen = strlen(pUidStr);

    if(TDUQID_MAX_LEN > currentLen) {

        moreNo = TDUQID_MAX_LEN - currentLen-1;
        for(;i<=moreNo;i++) {

            randlong = get_random_long(seedCal+i);
            suffixNo = randlong % 62;
            strncat(pUidStr, &suffixAlnum[suffixNo], 1);
        }
    }

    return (const char *)pUidStr;
}

