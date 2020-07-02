DEMANGLE= # demangle.c

SRCS=	asm.c audit.c bpts.c core.c dtype.c dwarf.c dwarfabbrev.c \
	dwarfline.c dwarfrec.c dwarfsymtab.c dwarftype.c elf.c expr.c \
	format.c frame.c help.c hostconf.c hostcore.c hostfunc.c \
	hostmaster.c hostproc.c i686asm.c i686core.c journal.c lib.c \
	master.c memory.c monitor.c parse.c phrase.c pi.c process.c \
	sigmask.c skiplist.c srcdir.c srctext.c symbol.c symtab.c wd.c \
	$(DEMANGLE)

OBJS=	gram.o $(SRCS:.c=.o)

DSRC=	asm.c audit.c bpts.c core.c dtype.c dwarf.c dwarfabbrev.c \
	dwarfline.c dwarfrec.c dwarfsymtab.c dwarftype.c elf.c expr.c \
	format.c frame.c help.c hostconf.c hostcore.c hostfunc.c \
	hostmaster.c hostproc.c i686asm.c i686core.c journal.c lib.c \
	master.c memory.c monitor.c parse.c phrase.c pi.c process.c \
	sigmask.c skiplist.c srcdir.c srctext.c symbol.c symtab.c wd.c \
	$(DEMANGLE) dwarfdebug.c

DOBJ=	gram.o $(DSRC:.c=.o)

# SRCFILES and HFILES are for make depend

SRCFILES:sh= ls *.c
HFILES:sh= ls *.h

TIME:sh= date +'%a %b %e %Y'
OBJTYPE:sh= uname -s -r -m

PADS=	../pads
PADS=	../../libblit/pads

RM= rm -f
INC=	-I${HOME}/include
DEFINES= -DHASHSTATS # -DTRACE

CC=	gcc
CCC=	g++
XARCH=	-m32
CFLAGS=	${XARCH} ${INC} ${DEFINES} -Wall -fpermissive -Wno-write-strings -g -DDWARF
LDFLAGS= ${XARCH}

CC=	cc
CCC=	CC
CFLAGS=	${INC} ${DEFINES} -g -erroff=badinitstr

DEPFLAGS= -xM1 ${INC} ${DEFINES} 

LIB=	-L${HOME}/lib
LDLIBS=	-L${HOME}/lib -lpads
STRIP=	:
BINDIR=	$(HOME)/bin
INSTALL=	/usr/gnu/bin/install -c
INSTALLDIR=/n/lucian/sd4d/jtools
CLEANFILES=core a.out *.o pi piv.c gram.c y.tab.h cscope.out \
	makedep eddep makefile.bak TRACE trace.log dwarfdebug

all:: pi

pi:	${OBJS}
	rm -f pi
	${CCC} -o pi -g ${LDFLAGS} ${OBJS} ${LDLIBS}

# %.o : %.c ; ${CC} -c ${CFLAGS} $<

%.o : %.c ; ${CCC} -c ${CFLAGS} $<

cscope.files: ${SRCS}
	ls *.cc *.[hcyl] > $@

srcfiles: ; @for f in $(HFILES) $(SRCS); do echo $$f; done

pi.o : pi.c
	${CCC} -c ${CFLAGS} -DVERSION="\"$(OBJTYPE) $(TIME)\"" pi.c

piv.c:
	@echo 'char version[] = "Name:\\tpi\\nDate:\\t'`date` \
	'\\nHost:\\t'`uname`'\\nUser:\\t$(LOGNAME)\\n";' > $@

piv.o: piv.c; $(CC) -c ${XARCH} piv.c

gram.o:	gram.c

gram.c:	gram.y
	yacc -d gram.y
	mv y.tab.c gram..c
	sed -e '/^# *line/d' gram..c > gram.c
	rm gram..c

y.tab.h: gram.c

dwarfdebug: $(DOBJ) ; ${CCC} -o $@ -g ${LDFLAGS} ${DOBJ} ${LIB}

install: $(BINDIR)/pi

$(BINDIR)/pi : pi ; $(RM) $@; $(INSTALL) -c -m 755 pi $@

clean:
	rm -f $(CLEANFILES)

depend: gram.c
	for f in ${SRCFILES}; do \
		${CCC} ${DEPFLAGS} $$f; \
	done | \
	nawk 'BEGIN { TS=8; LL=78 } \
	      { if ($$1 != prev) {  \
			if (rec != "") print rec; \
			rec = $$1 ":"; prev = $$1; \
			RL = LL-length($$1)-1 } \
	        else if (length(rec $$3) > RL) {  \
			print rec " \\"; rec = "\t" $$3; \
			RL = LL-TS } \
	        else rec = rec " " $$3 } \
		END { print rec }' > makedep
	@echo '/^# DO NOT DELETE THIS LINE/+2,$$d' > eddep
	@echo '$$r makedep' >> eddep
	@echo w >>eddep
	cp makefile makefile.bak
	ed - makefile < eddep
	rm eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

asm.o: asm.h univ.h /export/home/noel/include/pads.h lib.h core.h elf.h \
	format.h parse.h gram.h y.tab.h expr.h dtype.h frame.h process.h \
	symtab.h mip.h symbol.h dwarf.h dwarfrec.h bpts.h
audit.o: univ.h /export/home/noel/include/pads.h lib.h audit.h
avl.o: dtype.h univ.h /export/home/noel/include/pads.h lib.h avl.h
bpts.o: univ.h /export/home/noel/include/pads.h lib.h core.h elf.h \
	format.h bpts.h symbol.h dtype.h dwarf.h dwarfrec.h expr.h process.h \
	symtab.h mip.h parse.h gram.h y.tab.h
core.o: process.h univ.h /export/home/noel/include/pads.h lib.h format.h \
	frame.h symtab.h elf.h mip.h symbol.h dtype.h dwarf.h dwarfrec.h \
	core.h master.h bpts.h
demangle.o: univ.h /export/home/noel/include/pads.h lib.h
dtype.o: dtype.h univ.h /export/home/noel/include/pads.h lib.h symbol.h \
	elf.h format.h dwarf.h dwarfrec.h mip.h phrase.h expr.h
dwarf.o: lib.h elf.h format.h univ.h /export/home/noel/include/pads.h \
	dwarf.h dwarfabbrev.h dwarfrec.h
dwarfabbrev.o: elf.h format.h univ.h \
	/export/home/noel/include/pads.h lib.h dwarf.h dwarfabbrev.h
dwarfdebug.o: format.h univ.h /export/home/noel/include/pads.h \
	lib.h core.h elf.h symtab.h mip.h dtype.h symbol.h dwarf.h dwarfrec.h \
	master.h process.h hostcore.h hostfunc.h unix.h dwarftype.h skiplist.h \
	dwarfsymtab.h dwarfline.h
dwarfline.o: elf.h format.h univ.h /export/home/noel/include/pads.h \
	lib.h dwarf.h symbol.h dtype.h dwarfrec.h dwarfline.h
dwarfrec.o: elf.h format.h univ.h /export/home/noel/include/pads.h \
	lib.h dwarf.h dwarfrec.h dwarfabbrev.h
dwarfsymtab.o: format.h univ.h /export/home/noel/include/pads.h \
	lib.h core.h elf.h symtab.h mip.h dtype.h symbol.h dwarf.h dwarfrec.h \
	dwarftype.h skiplist.h dwarfsymtab.h dwarfline.h
dwarftype.o: mip.h elf.h format.h univ.h \
	/export/home/noel/include/pads.h lib.h dwarf.h symtab.h mip.h symbol.h \
	dtype.h dwarfrec.h dwarftype.h skiplist.h dwarfsymtab.h
elf.o: elf.h format.h univ.h /export/home/noel/include/pads.h lib.h
expr.o: expr.h univ.h /export/home/noel/include/pads.h lib.h dtype.h \
	format.h gram.h symbol.h elf.h dwarf.h dwarfrec.h frame.h phrase.h \
	symtab.h mip.h core.h
format.o: univ.h /export/home/noel/include/pads.h lib.h format.h \
	symtab.h elf.h mip.h symbol.h dtype.h dwarf.h dwarfrec.h
frame.o: frame.h univ.h /export/home/noel/include/pads.h lib.h core.h \
	elf.h format.h symbol.h dtype.h dwarf.h dwarfrec.h symtab.h mip.h \
	expr.h phrase.h parse.h gram.h y.tab.h process.h bpts.h journal.h
gram.o: gram.h expr.h univ.h /export/home/noel/include/pads.h lib.h \
	dtype.h format.h bpts.h
help.o: /export/home/noel/include/pads.h help.pi
hostconf.o: core.h univ.h /export/home/noel/include/pads.h lib.h \
	elf.h format.h process.h master.h frame.h i686core.h hostcore.h \
	hostfunc.h unix.h
hostcore.o: core.h univ.h /export/home/noel/include/pads.h lib.h \
	elf.h format.h process.h master.h sigmask.h frame.h memory.h symtab.h \
	mip.h symbol.h dtype.h dwarf.h dwarfrec.h srcdir.h asm.h bpts.h expr.h \
	hostcore.h hostfunc.h unix.h
hostfunc.o: elf.h format.h univ.h /export/home/noel/include/pads.h \
	lib.h hostfunc.h unix.h
hostmaster.o: core.h univ.h /export/home/noel/include/pads.h lib.h \
	elf.h format.h process.h master.h sigmask.h frame.h memory.h symtab.h \
	mip.h symbol.h dtype.h dwarf.h dwarfrec.h srcdir.h asm.h bpts.h expr.h \
	hostcore.h hostfunc.h unix.h
hostproc.o: core.h univ.h /export/home/noel/include/pads.h lib.h \
	elf.h format.h process.h master.h sigmask.h frame.h memory.h symtab.h \
	mip.h symbol.h dtype.h dwarf.h dwarfrec.h srcdir.h asm.h bpts.h expr.h \
	hostcore.h hostfunc.h unix.h
i686asm.o: univ.h /export/home/noel/include/pads.h lib.h asm.h \
	i686asm.h core.h elf.h format.h i686table.h
i686core.o: process.h univ.h /export/home/noel/include/pads.h lib.h \
	format.h frame.h symtab.h elf.h mip.h symbol.h dtype.h dwarf.h \
	dwarfrec.h core.h asm.h i686asm.h i686core.h dwarftype.h skiplist.h \
	dwarfsymtab.h
journal.o: journal.h univ.h /export/home/noel/include/pads.h lib.h \
	core.h elf.h format.h
lib.o: /export/home/noel/include/pads.h lib.h
master.o: master.h univ.h /export/home/noel/include/pads.h lib.h \
	process.h format.h
memory.o: core.h univ.h /export/home/noel/include/pads.h lib.h elf.h \
	format.h symtab.h mip.h memory.h parse.h gram.h y.tab.h expr.h dtype.h \
	frame.h process.h journal.h
monitor.o:
parse.o: parse.h gram.h y.tab.h expr.h univ.h \
	/export/home/noel/include/pads.h lib.h dtype.h format.h
phrase.o: expr.h univ.h /export/home/noel/include/pads.h lib.h dtype.h \
	format.h phrase.h frame.h parse.h gram.h y.tab.h symtab.h elf.h mip.h \
	symbol.h dwarf.h dwarfrec.h core.h process.h
pi.o: univ.h /export/home/noel/include/pads.h lib.h audit.h core.h elf.h \
	format.h master.h process.h hostcore.h hostfunc.h unix.h
process.o: frame.h univ.h /export/home/noel/include/pads.h lib.h \
	core.h elf.h format.h process.h srcdir.h expr.h dtype.h master.h \
	bpts.h memory.h asm.h symtab.h mip.h symbol.h dwarf.h dwarfrec.h \
	srctext.h journal.h
sigmask.o: core.h univ.h /export/home/noel/include/pads.h lib.h elf.h \
	format.h master.h process.h srctext.h sigmask.h
skiplist.o: skiplist.h
srcdir.o: srcdir.h univ.h /export/home/noel/include/pads.h lib.h \
	process.h format.h symbol.h dtype.h elf.h dwarf.h dwarfrec.h srctext.h \
	symtab.h mip.h
srctext.o: lib.h symbol.h univ.h /export/home/noel/include/pads.h \
	dtype.h elf.h format.h dwarf.h dwarfrec.h srctext.h core.h symtab.h \
	mip.h process.h bpts.h expr.h frame.h
symbol.o: univ.h /export/home/noel/include/pads.h lib.h dtype.h \
	symbol.h elf.h format.h dwarf.h dwarfrec.h symtab.h mip.h srctext.h \
	core.h process.h bpts.h phrase.h parse.h gram.h y.tab.h expr.h
symtab.o: symtab.h univ.h /export/home/noel/include/pads.h lib.h elf.h \
	format.h mip.h dtype.h symbol.h dwarf.h dwarfrec.h srctext.h phrase.h \
	core.h
wd.o: univ.h /export/home/noel/include/pads.h lib.h wd.h
