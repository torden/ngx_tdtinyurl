/*
 * @author : torden <https://github.com/torden/>
 */
#include <tduqid.h>

int main(void) {

    char *pTest = NULL;
    long int nSeedNum = 1234567891234;
    int nloopCnt = 1000000;
    //int nloopCnt = 10;
    int i=1;

    for(;i<nloopCnt;i++) {

        pTest = calloc(sizeof(char), UQID_MALLOC_LEN);
        fprintf(stdout, "[%d] %s\r", i, uqid_get_str(pTest, UQID_MAX_LEN, nSeedNum));
        free(pTest);
    }

    pTest = calloc(sizeof(char), UQID_MALLOC_LEN);
    for(i=1;i<nloopCnt;i++) {

        fprintf(stdout, "[%d] %s\r", i, uqid_get_str(pTest, UQID_MAX_LEN, nSeedNum));
    }

    free(pTest);
    printf("\n");
    return 0;
}
