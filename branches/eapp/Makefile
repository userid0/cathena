CCC = g++ -pipe
PACKETDEF = -DPACKETVER=6 -DNEW_006b
OPT = -g -O3 -ffast-math -D_REENTRANT
LIBS =  -lpthread -lz

##########################################################
PLATFORM = $(shell uname)
ifeq ($(findstring Linux,$(PLATFORM)), Linux)
   LIBS += -ldl
endif
ifeq ($(findstring SunOS,$(PLATFORM)), SunOS)
   LIBS += -lsocket -lnsl -ldl -lrt
endif
ifeq ($(findstring FreeBSD,$(PLATFORM)), FreeBSD)
   MAKE = gmake
endif
ifeq ($(findstring NetBSD,$(PLATFORM)), NetBSD)
   MAKE = gmake
   OS_TYPE = -D__NETBSD__
endif
ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
   OS_TYPE = -DCYGWIN
endif

##########################################################
# set some path variables for convenience
RETURN_PATH = ../..
BASE_PATH   = src/basics
COMMON_PATH = src/common

##########################################################
CPPFLAGS = $(OPT) -I$(RETURN_PATH)/$(COMMON_PATH) -I$(RETURN_PATH)/$(BASE_PATH) $(OS_TYPE)

##########################################################
# my defaults for mysql libs
MYSQL_INCLUDE = -I../../../mysql/include/ 
MYSQL_LIB     = -L../../../mysql/lib -lmysqlclient -lposix4 -lcrypt -lgen -lnsl -lm

##########################################################
ifdef SQLFLAG
MYSQLFLAG_CONFIG = $(shell which mysql_config)

ifeq ($(findstring /mysql_config,$(MYSQLFLAG_CONFIG)), /mysql_config)

MYSQLFLAG_VERSION = $(shell $(MYSQLFLAG_CONFIG) --version | sed s:\\..*::) 

ifeq ($(findstring 4,$(MYSQLFLAG_VERSION)), 4)
MYSQLFLAG_CONFIG_ARGUMENT = --cflags
endif

ifeq ($(findstring 5,$(MYSQLFLAG_VERSION)), 5)
MYSQLFLAG_CONFIG_ARGUMENT = --include
endif

ifndef MYSQLFLAG_CONFIG_ARGUMENT
MYSQLFLAG_CONFIG_ARGUMENT = --cflags
endif

MYSQL_INCLUDE = $(shell $(MYSQLFLAG_CONFIG) $(MYSQLFLAG_CONFIG_ARGUMENT)) 
MYSQL_LIB     = $(shell $(MYSQLFLAG_CONFIG) --libs)
endif

MKDEF = CCC="$(CCC)" CPPFLAGS="$(CPPFLAGS) $(MYSQL_INCLUDE)" LIBS="$(MYSQL_LIB) $(LIBS) $(GCLIB)" RETURN_PATH="$(RETURN_PATH)" BASE_PATH="$(BASE_PATH)" COMMON_PATH="$(COMMON_PATH)"
else
MKDEF = CCC="$(CCC)" CPPFLAGS="$(CPPFLAGS) -DTXT_ONLY" LIBS="$(LIBS) $(GCLIB)" RETURN_PATH="$(RETURN_PATH)" BASE_PATH="$(BASE_PATH)" COMMON_PATH="$(COMMON_PATH)"
endif



all: conf txt

conf:
	cp -r conf-tmpl conf
	rm -rf conf/.svn conf/*/.svn

.PHONY : basics
basics: basics/GNUmakefile
	cd src ; cd basics ; $(MAKE) $(MKDEF) all ; cd .. ; cd ..

txt : basics src/common/GNUmakefile src/login/GNUmakefile src/char/GNUmakefile src/map/GNUmakefile src/ladmin/GNUmakefile src/scriptchk/GNUmakefile conf 
	cd src ; cd common ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd login ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd char ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd map ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd ladmin ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd scriptchk ; $(MAKE) $(MKDEF) all ; cd .. ; cd ..

ifdef SQLFLAG
sql: basics src/common/GNUmakefile src/login/GNUmakefile src/char/GNUmakefile src/map/GNUmakefile src/ladmin/GNUmakefile src/scriptchk/GNUmakefile conf
	cd src ; cd common ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd login ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd char ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd map ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd ladmin ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd scriptchk ; $(MAKE) $(MKDEF) all ; cd .. ; cd ..
else
sql:
	$(MAKE) CCC="$(CCC)" OPT="$(OPT)" SQLFLAG=1 $@
endif

clean: src/basics/GNUmakefile src/common/GNUmakefile src/login/GNUmakefile src/char/GNUmakefile src/map/GNUmakefile src/ladmin/GNUmakefile src/scriptchk/GNUmakefile
	cd src ; cd basics ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd common ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd login ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd char ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd map ; $(MAKE) $(MKLIB) $@ ; cd .. ; cd ..
	cd src ; cd ladmin ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..
	cd src ; cd scriptchk ; $(MAKE) $(MKDEF) $@ ; cd .. ; cd ..

src/basics/GNUmakefile: src/basics/Makefile
	sed -e 's/$$>/$$^/' src/basics/Makefile > src/basics/GNUmakefile
src/common/GNUmakefile: src/common/Makefile
	sed -e 's/$$>/$$^/' src/common/Makefile > src/common/GNUmakefile
src/login/GNUmakefile: src/login/Makefile
	sed -e 's/$$>/$$^/' src/login/Makefile > src/login/GNUmakefile
src/char/GNUmakefile: src/char/Makefile
	sed -e 's/$$>/$$^/' src/char/Makefile > src/char/GNUmakefile
src/map/GNUmakefile: src/map/Makefile
	sed -e 's/$$>/$$^/' src/map/Makefile > src/map/GNUmakefile
src/ladmin/GNUmakefile: src/ladmin/Makefile
	sed -e 's/$$>/$$^/' src/ladmin/Makefile > src/ladmin/GNUmakefile
src/scriptchk/GNUmakefile: src/scriptchk/Makefile
	sed -e 's/$$>/$$^/' src/scriptchk/Makefile > src/scriptchk/GNUmakefile
