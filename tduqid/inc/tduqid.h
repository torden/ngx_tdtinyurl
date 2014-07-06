/*
 * @author : torden <https://github.com/torden/>
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stddef.h>
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#define TDUQID_MAX_LEN 20
#define TDUQID_MALLOC_LEN TDUQID_MAX_LEN+1
extern long int tduqid_get_str_to_number(unsigned char *pStr);
extern unsigned int tduqid_get_identity_grp_no(const char *pUidStr);
extern const char *tduqid_get_str(char *pUidStr, unsigned int buflen, long int seedNum);
