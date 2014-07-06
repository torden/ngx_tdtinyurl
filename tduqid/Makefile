VERSION		:= "0.2"
PLATFORM	:=$(shell uname -a)

CMD_SUDO    :=$(shell which sudo)
CMD_RM      :=$(shell which rm)
CMD_CP      :=$(shell which cp)
CMD_AR      :=$(shell which ar)
CMD_RANLIB  :=$(shell which ranlib)
CMD_MV      :=$(shell which mv)
CMD_STRIP   :=$(shell which strip)
CMD_TAIL    :=$(shell which tail)
CMD_FIND    :=$(shell which find)
CMD_CURL    :=$(shell which curl)
CMD_LDD     :=$(shell which ldd)
CMD_MKDIR   :=$(shell which mkdir)
CMD_CC		:=$(shell which gcc)
CMD_TEST    :=$(shell which test)
CMD_PATCH   :=$(shell which patch)
CMD_SLEEP   :=$(shell which sleep)
CMD_SYNC	:=$(shell which sync)
CMD_LN		:=$(shell which	ln)
CMD_VALGRIND = "/usr/local/valgrind/bin/valgrind"
OS_DIST		:=$(shell lsb_release -is)

PREFIX?=/usr/local/

GOOGL_PERFORMANCE_TOOLS_DIR=/usr/local/devel/gperftools/

COMMON_INC_PATH=-I$(GOOGL_PERFORMANCE_TOOLS_DIR)/include/gperftools/ -I./inc/ -I/usr/include/ -I/usr/local/include/ 
ifeq ($(findstring x86_64,$(PLATFORM)), x86_64)
    CMD_CC_FLAG=-O2 -g -Wall -m64 -fPIC
    COMMON_LIB_PATH=-L$(GOOGL_PERFORMANCE_TOOLS_DIR)/lib/ -L./lib/ -L/usr/lib64/ -L/usr/local/lib64/
    INSTALL_LIB_PATH=$(PREFIX)/lib/
    INSTALL_INC_PATH=$(PREFIX)/include/
else
    CMD_CC_FLAG=-O2 -g -Wall -m32 -fPIC
    COMMON_LIB_PATH=-L$(GOOGL_PERFORMANCE_TOOLS_DIR)/lib/ -L./lib/ -L/usr/lib/ -L/usr/local/lib/
    INSTALL_LIB_PATH=$(PREFIX)/lib/
    INSTALL_INC_PATH=$(PREFIX)/include/
endif

ifeq ($(OS_DIST),Ubuntu)
	ECHO=echo
else
	ECHO=echo -e
endif



all: clean libs tduqid install clean
dev: clean setenv libs tduqid install
test: setenv libs tduqid test_tduqid test_valgrind

install::
	@$(ECHO) "\033[1;40;32mInstalling libs to /usr/local/lib/..\033[01;m"
	@$(CMD_SUDO) $(CMD_RM) -f $(INSTALL_LIB_PATH)/libtduqid.so $(INSTALL_LIB_PATH)/libtduqid.so.$(VERSION) $(INSTALL_LIB_PATH)/libtduqid.a
	@$(CMD_SUDO) $(CMD_MKDIR) -p $(INSTALL_LIB_PATH) $(INSTALL_INC_PATH)
	@$(CMD_SUDO) $(CMD_MV) -f tmp/libtduqid.a $(INSTALL_LIB_PATH)
	@$(CMD_SUDO) $(CMD_MV) -f tmp/libtduqid.so.$(VERSION) $(INSTALL_LIB_PATH)
	@$(CMD_SUDO) $(CMD_LN) -s $(INSTALL_LIB_PATH)/libtduqid.so.$(VERSION) $(INSTALL_LIB_PATH)/libtduqid.so
	@$(CMD_SUDO) $(CMD_CP) -rf inc/tduqid.h $(INSTALL_INC_PATH)
	@$(ECHO) "\033[1;40;36mDone\033[01;m"

clean::
	@$(ECHO) "\033[1;40;32mCleaning Previusly module/daemon/util objects..\033[01;m"
	@$(CMD_SUDO) $(CMD_RM) -f *.so *.o core.* *~
	@$(CMD_SUDO) $(CMD_RM) -f lib/*.so lib/*.o lib/core.* lib/*.a lib/*.so.*
	@$(CMD_SUDO) $(CMD_RM) -f module/*.so module/*.o module/core.*
	@$(CMD_SUDO) $(CMD_RM) -f tmp/*.so tmp/*.o tmp/core.*
	@$(CMD_SUDO) $(CMD_RM) -f daemon/*.so daemon/*.o daemon/core.*
	@$(ECHO) "\033[1;40;36mOK\033[01;m"

cleanall:: clean cleanlibs

setenv::
	@$(ECHO) "\033[1;40;32mSet Debug mode enviroment..\033[01;m"
	@$(CMD_SUDO) sysctl -e -q -w kernel.core_pattern="/tmp/%e.core.%u" kernel.suid_dumpable=1 fs.suid_dumpable=1
	@$(CMD_SUDO) sysctl -q -w kernel.core_uses_pid=1;
	@$(ECHO) "\tkernel.core_pattern=/tmp/%e.core.%u"
	@$(ECHO) "\tkernel.suid_dumpable=1"
	@$(ECHO) "\tfs.suid_dumpable=1"
	@$(ECHO) "\tkernel.core_users_pid=1"
	@$(ECHO) "\033[1;40;36mComplete\033[01;m"

libs:: cleanlibs tduqid

cleanlibs::
	@$(ECHO) "\033[1;40;32mCleaning Previusly library objects..\033[01;m"
	@$(CMD_RM) -f lib/*.so* lib/*.o lib/core.* lib/*~ lib/*.core* lib/gmon.*
	@$(ECHO) "\033[1;40;36mDone\033[01;m"

tduqid::
	@$(ECHO) "\033[1;40;33mBuilding [$@] library..\033[01;m"
	@$(CMD_SUDO) $(CMD_MKDIR) -p tmp/
	@$(CMD_CC) $(CMD_CC_FLAG) -fPIC -o lib/$@.o -c lib/$@.c -I./inc/ $(COMMON_INC_PATH) $(COMMON_LIB_PATH)
	@$(CMD_AR) rs lib/lib$@.a lib/$@.o 
	@$(CMD_CC) $(CMD_CC_FLAG) -shared -o lib/lib$@.so.$(VERSION) lib/$@.o $(COMMON_INC_PATH) $(COMMON_LIB_PATH)
	@$(CMD_MV) lib/lib$@.so.$(VERSION) lib/lib$@.a  tmp/
	@$(ECHO) "\033[1;40;36mDone\033[01;m"

test_tduqid::
	@$(ECHO) "\033[1;40;33mBuilding [$@] ..\033[01;m"
	@$(CMD_CC) $(CMD_CC_FLAG) -o tmp/$@ test/$@.c tmp/libtduqid.a -I./inc/ $(COMMON_INC_PATH) $(COMMON_LIB_PATH)
	@./tmp/$@
	@$(ECHO) "\033[1;40;36mDone\033[01;m"

test_valgrind::
	@$(ECHO) "\033[1;40;33mBuilding [$@] ..\033[01;m"
	@$(CMD_VALGRIND) --leak-check=full --show-reachable=yes -v tmp/test_tduqid
	@$(ECHO) "\033[1;40;36mDone\033[01;m"
	@$(CMD_VALGRIND) --tool=memcheck --leak-check=full --show-reachable=yes -v tmp/test_tduqid
	@$(ECHO) "\033[1;40;36mDone\033[01;m"
