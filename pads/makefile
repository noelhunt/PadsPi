SUBDIRS= host term demo
INCLDIR= $(HOME)/include
INSTALL= /usr/gnu/bin/install

all install clean ::
	for d in $(SUBDIRS); do \
		(cd $$d; echo making $@ in $$d; ${MAKE} $@); \
	done

install :: $(INCLDIR)/pads.h

$(INCLDIR)/pads.h: pads.h
	$(INSTALL) pads.h $(INCLDIR)

pads.pri: pads.h
	/usr/jerq/bin/cleave  pads
	touch pads.pri

linstall:
	cp host/libpads.a /usr/jerq/lib
	ranlib	/usr/jerq/lib/libpads.a
	cp pads.pub /usr/jerq/include
	cp pads.pri /usr/jerq/include
	cp term/pads.m	/usr/jerq/mbin

lclean:
	rm -f *junk* core jim.recover
