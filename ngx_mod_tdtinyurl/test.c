#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <kclangc.h>

int main(void) {


    KCDB *dbm = kcdbnew();
    int32_t retval = 0;


    retval = kcdbopen(dbm, "./test.kch", KCOWRITER | KCOCREATE | KCOAUTOSYNC );
    printf("WOPEN : %d\n", retval);

    
    retval = kcdbset(dbm, "testkey", 7, "testval1", 8);
    printf("SET : %d\n", retval);
    retval = kcdbset(dbm, "testkey", 7, "testval2", 8);
    printf("SET : %d\n", retval);

    kcdbclose(dbm);

    size_t sp;
    char *val = NULL;
 
    retval = kcdbopen(dbm, "./test.kch", KCOREADER);
    printf("ROPEN : %d\n", retval);

    val = kcdbget(dbm, "testkey", 7, &sp);
    kcdbclose(dbm);

    printf("%s\n", val);
    printf("%d\n", sp);


    return 0;
}
