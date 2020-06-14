CC=CC
CCC=cc
CFLAGS=-I. -I.. -g -DTRACE #-DUMEM_DEUBG
LIB=../host/libpads.a
UMEM_DEBUG=default
UMEM_LOGGING=contents,fail
LDLIB=-lumem
TARGETS= alloc gcalloc middle null now news misc \
	creat test stats padstats comp draft change

all: $(TARGETS)

alloc:	   alloc.c;  $(CCC) $(CFLAGS) alloc.c -o $@
gcalloc: gcalloc.c;  $(CCC) $(CFLAGS) gcalloc.c -o $@
needs:	   needs.c;  $(CCC) $(CFLAGS) needs.c -o $@

middle    :  ${LIB} middle.o      ;  $(CC) middle.o -o $@ ${LIB}
middle.o  :  story.h storyopen.c
null      :  ${LIB} null.o        ;  $(CC) null.o -o $@ ${LIB}
now       :  ${LIB} now.o         ;  $(CC) now.o -o $@ ${LIB}
news      :  ${LIB} news.o        ;  $(CC) news.o -o $@ ${LDLIB} ${LIB}
misc      :  ${LIB} misc.o        ;  $(CC) misc.o -o $@ ${LIB}
comp      :  ${LIB} comp.o        ;  $(CC) comp.o -o $@ ${LIB}
creat     :  ${LIB} creat.o       ;  $(CC) creat.o -o $@ ${LIB}
draft     :  ${LIB} draft.o       ;  $(CC) draft.o -o $@ ${LIB}
stats     :  ${LIB} stats.o       ;  $(CC) stats.o -o $@ ${LIB}
padstats  :  ${LIB} padstats.o    ;  $(CC) padstats.o -o $@ ${LIB}
test      :  ${LIB} test.o        ;  $(CC) test.o -o $@ ${LIB}

clean: ; rm -f $(TARGETS) *.o a.out core

${LIB}: frc ; cd ../host; $(MAKE)

frc:
