all: ladmin
txt: ladmin
sql: ladmin

COMMON_OBJ = ../common/core.o ../common/socket.o ../common/timer.o ../common/db.o ../common/malloc.o ../common/utils.o ../common/showmsg.o ../common/strlib.o ../common/md5calc.o
COMMON_H = ../common/core.h ../common/socket.h ../common/timer.h ../common/mmo.h ../common/version.h ../common/db.h ../common/malloc.h ../common/utils.h ../common/showmsg.h ../common/strlib.h ../common/md5calc.h

ladmin: ladmin.o $(COMMON_OBJ)
	$(CC) -o ../../$@ ladmin.o $(COMMON_OBJ)
ladmin.o: ladmin.c ladmin.h $(COMMON_H)

clean:
	rm -f *.o ../../ladmin
