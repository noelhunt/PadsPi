#include "lib.h"
#include "elf.h"
#include "dwarf.h"
#include "dwarfabbrev.h"
#include "dwarfrec.h"
#include <stdio.h>
SRCFILE("dwarf.c")

static char sccsid[ ] = "%W%\t%G%";

Dwarf::Dwarf(){
	abbrevs = new DwarfAbbrevs( this );
}

Dwarf::~Dwarf(){ }

MemLayout Dwarf::memlayout(){
	return elf->memlayout();
}

int Dwarf::readblock(DwarfBlock *b, ulong off, ulong len){
	b->data = new uchar[len];
	if(lseek(elf->fd, off, 0)<0 || readn(elf->fd, b->data, len)!=len){
		delete (uchar*)b->data;
		b->data = 0;
		return -1;
	}
	b->len = len;
	return 0;
}

int Dwarf::findsect(const char *name, ulong *off, ulong *len){
	ElfSect *s;

	if((s = elf->findsect(name)) == 0)
		return -1;

	*off = s->offset;
	*len = s->size;
	return 1;
}
	
int Dwarf::loadsect(const char *name, DwarfBlock *b){
	ulong off, len;

	if(findsect(name, &off, &len) < 0)
		return -1;

	return readblock(b, off, len);
}

int Dwarf::open(Elf *e){
	if(e == 0){
		errstr("0 elf passed to Dwarf::open");
		return -1;
	}

	elf = e;
	if(loadsect(".debug_abbrev", &sect[D_ABBREV]) < 0
	  || loadsect(".debug_info", &sect[D_INFO]) < 0
	  || loadsect(".debug_line", &sect[D_LINE]) < 0){
		errstr("no required sections");
		goto error;
	}
#ifdef EXTRASECTS
	loadsect(".debug_pubnames", &sect[D_PUBNAMES]);
	loadsect(".debug_aranges", &sect[D_ARANGES]);
	loadsect(".debug_frame", &sect[D_FRAME]);
	loadsect(".debug_ranges", &sect[D_RANGES]);
	loadsect(".debug_str", &sect[D_STRINGS]);
#endif
	return 0;

error:
	return -1;
}

/*
 * Dwarf data format parsing routines.
 */

ulong Dwarf::get1(DwarfBuf *b){
	if(b->bp==0 || b->bp+1 > b->ep){
		b->bp = 0;
		return 0;
	}
	return *b->bp++;
}

int Dwarf::getn(DwarfBuf *b, uchar *a, int n){
	if(b->bp==0 || b->bp+n > b->ep){
		b->bp = 0;
		memset(a, 0, n);
		return -1;
	}
	memmove(a, b->bp, n);
	b->bp += n;
	return 0;
}

uchar* Dwarf::getnref(DwarfBuf *b, ulong n){
	uchar *p;

	if(b->bp==0 || b->bp+n > b->ep){
		b->bp = 0;
		return 0;
	}
	p = b->bp;
	b->bp += n;
	return p;
}

char *Dwarf::getstring(DwarfBuf *b){
	char *s;

	if(b->bp == 0)
		return 0;
	s = (char*)b->bp;
	while(b->bp < b->ep && *b->bp)
		b->bp++;
	if(b->bp >= b->ep){
		b->bp = 0;
		return 0;
	}
	b->bp++;
	return s;
}

void Dwarf::skip(DwarfBuf *b, int n){
	if(b->bp==0 || b->bp+n > b->ep)
		b->bp = 0;
	else
		b->bp += n;
}

ulong Dwarf::get2(DwarfBuf *b){
	ulong v;

	if(b->bp==0 || b->bp+2 > b->ep){
		b->bp = 0;
		return 0;
	}
	v = elf->e2(b->bp);
	b->bp += 2;
	return v;
}

ulong Dwarf::get4(DwarfBuf *b){
	ulong v;

	if(b->bp==0 || b->bp+4 > b->ep){
		b->bp = 0;
		return 0;
	}
	v = elf->e4(b->bp);
	b->bp += 4;
	return v;
}

uvlong Dwarf::get8(DwarfBuf *b){
	uvlong v;

	if(b->bp==0 || b->bp+8 > b->ep){
		b->bp = 0;
		return 0;
	}
	v = elf->e8(b->bp);
	b->bp += 8;
	return v;
}

ulong Dwarf::getaddr(DwarfBuf *b){
	static int nbad;

	if(b->addrsz == 0)
		b->addrsz = addrsz;

	switch(b->addrsz){
	case 1:	return get1(b);
	case 2:	return get2(b);
	case 4:	return get4(b);
	case 8:	return get8(b);
	default:
		if(++nbad == 1)
			errstr("dwarf: unexpected address size %lud in Dwarf::getaddr\n", b->addrsz);
		b->bp = 0;
		return 0;
	}
}

static int n1, n2, n3, n4, n5;

/* An inline function picks off the calls to Dwarf::get128 for 1-byte encodings,
 * more than by far the common case (99.999% on most binaries!). */

ulong Dwarf::get128(DwarfBuf *b){
	static int nbad;
	ulong c, d;

	if(b->bp == 0)
		return 0;
	c = *b->bp++;
	if(!(c&0x80)){
		n1++; return c;
	}
	c &= ~0x80;
	d = *b->bp++;
	c |= (d&0x7F)<<7;
	if(!(d&0x80)){
		n2++; return c;
	}
	d = *b->bp++;
	c |= (d&0x7F)<<14;
	if(!(d&0x80)){
		n3++; return c;
	}
	d = *b->bp++;
	c |= (d&0x7F)<<21;
	if(!(d&0x80)){
		n4++; return c;
	}
	d = *b->bp++;
	c |= (d&0x7F)<<28;
	if(!(d&0x80)){
		n5++; return c;
	}
	while(b->bp<b->ep && *b->bp&0x80)
		b->bp++;
	if(++nbad == 1)
		errstr("dwarf: overflow during parsing of uleb128 integer\n");
	return c;
}

long Dwarf::get128s(DwarfBuf *b){
	int nb, c;
	ulong v;
	static int nbad;

	v = 0;
	nb = 0;
	if(b->bp==0)
		return 0;
	while(b->bp<b->ep){
		c = *b->bp++;
		v |= (c & 0x7F)<<nb;
		nb += 7;
		if(!(c&0x80))
			break;
	}
	if(v&(1<<(nb-1)))
		v |= ~(((ulong)1<<nb)-1);
	if(nb > 8*sizeof(ulong)){
		if(++nbad == 1)
			errstr("dwarf: overflow during parsing of sleb128 integer: got %d bits\n", nb);
	}
	return v;
}

void Dwarf::errstr(const char *fmt, ...){
	va_list arg;
	char buf[ERRMAX];

	va_start(arg, fmt);
	vsnprintf(buf, ERRMAX, (char*)fmt, arg);
	va_end(arg);
	strncpy(syserr, buf, ERRMAX);
}

Place *Dwarf::location(DwarfRec& r){
	long l = 0;
	char *s = 0;
	if( r.attrs.have.location == TConstant ){
		l = r.attrs.location.c;
	}else if( r.attrs.have.location == TBlock ){
		DwarfBuf buf;
		DwarfBlock b = r.attrs.location.b;
		if( b.len == 0 ) goto Done;
		buf.bp = b.data+1;
		buf.ep = b.data+b.len;
		buf.addrsz = 0;
		if( b.data[0]==OpAddr ){
			if(b.len != 5) goto Done;
			l = getaddr(&buf);
		}else if( OpReg0 <= b.data[0] && b.data[0] < OpReg0+0x20 ){
			if(b.len == 1) goto Done;
			s = sf("reg.%d", b.data[0]-OpReg0);
		}else if( OpBreg0 <= b.data[0] && b.data[0] < OpBreg0+0x20 ){
			l = get128s(&buf);
		}else if( b.data[0] == OpRegx ){
			s = sf("reg.%d", get128(&buf));
		}else if( b.data[0] == OpFbreg ){
			l = get128s(&buf);
		}else if( b.data[0] == OpBregx ){
			l = get128s(&buf);
		}
	}
Done:
	return new Place( l, s );
}

Place::Place(ulong l, char *s){
	lng = l;
	_isreg = (reg = s) ? 1 : 0;
};
