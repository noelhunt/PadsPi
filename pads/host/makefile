RM= rm -f
MALLOC=malloc.o
MALLOC=
OFILES=cache.o lib.o pad.o popen.o term.o menu.o remote.o \
	sf.o padrcv.o stats.o help.o trace.o popen.o $(MALLOC)
CFLAGS=-I. -I.. -I$(HOME)/include -g -DBINCTOR -DTRACE2
CFLAGS=-I. -I.. -g -DTRACE
CC=CC
LIB=libpads.a
LIBDIR= $(HOME)/lib
INSTALL= /usr/gnu/bin/install

all: ${LIB}

${LIB}: ${OFILES}
	ar cr ${LIB} ${OFILES}

${OFILES}: ../pads.h

trace.o: trace.c
	cc ${CFLAGS} -c trace.c

malloc.o: malloc.c
	cc ${CFLAGS} -c malloc.c

install: $(LIBDIR)/libpads.a

$(LIBDIR)/libpads.a: libpads.a
	$(RM) $@
	$(INSTALL) libpads.a $(LIBDIR)

clean:
	rm -f *.i core *.o ${LIB} a.out cscope.out

cache.o: ../pads.h
lib.o: ../pads.h
pad.o: ../pads.h
popen.o: ../pads.h
term.o: ../pads.h
menu.o: ../pads.h
remote.o: ../pads.h
sf.o: ../pads.h
padrcv.o: ../pads.h
stats.o: ../pads.h
help.o: ../pads.h
trace.o: ../pads.h
popen.o: ../pads.h
extern.o: ../pads.h
