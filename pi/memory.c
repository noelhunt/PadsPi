#include "core.h"
#include "symtab.h"
#include "memory.h"
#include "parse.h"
#include "expr.h"
#include "frame.h"
#include "process.h"
#include "format.h"
#include "journal.h"
SRCFILE("memory.c")

static char sccsid[ ] = "%W%\t%G%";

int Memory::disc()	{ return U_MEMORY; }

Memory::Memory(Core *c){
	core = c;
	prevpat = 0;
}

void Memory::userclose(){
	trace("%d.userclose()", this);	VOK;
	delete pad;
	pad = 0;
	invalidate();
	Cell *cellsetsib;
	for( ; cellset; cellset = cellsetsib ){
		cellsetsib = cellset->sib;		// new malloc
		delete cellset;
	}
	current = 0;
}

void Memory::makecell(Cell *model, long addr ){
	Cell *c;

	trace( "%d.makecell(%d,%d)", this, model, addr );	VOK;
	if( !model ){
		model = new Cell(this);
		model->fmt = F_HEX;
		model->size = 1;
	}
	for( c = cellset; c; c = c->sib )
		if( addr == c->addr ) break;
	if( !c ){
		c = new Cell(this);
		c->addr = addr;
		c->sib = cellset;
		cellset = c;
	}
	c->fmt = model->fmt;
	c->size = model->size;
	c->display();
	current = c;
}

void Memory::banner(){
	trace( "%d.banner()", this );	VOK;
	if( pad ){
		pad->banner("Memory: %s", core->procpath());
		pad->name("Mem %s", basename(core->procpath()));
	}
}

void Memory::open(long a){
	trace( "%d.open(%d)", this, a );		VOK;
	if( !pad ){
		pad = new Pad((PadRcv*) this);
		pad->options( ACCEPT_KBD|USERCLOSE );
		banner();
	}
	pad->makecurrent();
	if( a ) makecell(current,a);
}

char *Memory::help(){
	return ".=<expr> {show memory cell at address}";
}

const char *Memory::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Memory Window";
		case HELP_KEY:		return "Memory Keyboard";
		case HELP_LMENU:	return "Memory Line Menus";
		case HELP_LKEY:		return "Memory Line Keyboard";
		default:		return 0;
	}
}

char *Memory::kbd(char *s){
	Parse y(G_DOTEQ_CONEX,0);
	Expr *e;
	Bls error;

	trace( "%d.kbd(%s)", this, s );	OK(0);
	if( !(e = (Expr*)y.parse(s)) )
		return sf("%s: %s", y.error, s);
	e->evaltext(core->process()->globals, error);
	if( e->evalerr )
		return sf("%s: %s", s, error.text);
	makecell(current,e->val.lng);
	return 0;
}

struct FmtSize {
	long	fmt;
	long	size;
};

#ifdef FS
# undef FS
#endif

FmtSize FS[] = {
	F_DECIMAL,	7,
	F_SIGNED,	7,
	F_OCTAL,	7,
	F_HEX,		7,
	F_ASCII,	7,
	F_SYMBOLIC,	7,
	F_TIME,		4,
	F_FLOAT,	4,
	F_DOUBLE,	8,
	F_DBLHEX,	8,
	0,		0
};

int Cell::disc()	{ return U_CELL; }

Index Cell::carte(){
	Menu m, f;
	static Index sz, next, prev;

	OK(ZIndex);
	if (sz.null()) {
		Menu s, n, p;
		s.last("1 byte ", (Action)&Cell::resize, 1);
		s.last("2 bytes", (Action)&Cell::resize, 2);
		s.last("4 bytes", (Action)&Cell::resize, 4);
		s.last("8 bytes", (Action)&Cell::resize, 8);
		sz = s.index("size");
		n.last( "next   1", (Action)&Cell::relative, 1);
		n.last( "next   5", (Action)&Cell::relative, 5);
		n.last( "next  10", (Action)&Cell::relative, 10);
		n.last( "next  25", (Action)&Cell::relative, 25);
		n.last( "next  50", (Action)&Cell::relative, 50);
		n.last( "next 100", (Action)&Cell::relative, 100);
		next = n.index("next");
		p.last( "prev   1", (Action)&Cell::relative, -1);
		p.last( "prev   5", (Action)&Cell::relative, -5);
		p.last( "prev  10", (Action)&Cell::relative, -10);
		p.last( "prev  25", (Action)&Cell::relative, -25);
		p.last( "prev  50", (Action)&Cell::relative, -50);
		p.last( "prev 100", (Action)&Cell::relative, -100);
		prev = p.index("prev");
	}
	m.last( spy ? "unspy" : "spy on", (Action)&Cell::setspy, !spy );
	m.last(sz);

	for( int i = 0; FS[i].fmt; ++i )
		if( FS[i].size & size ){
			long b = FS[i].fmt;
			if( fmt&b ) b |= F_TURNOFF;
			f.last(FmtName((int)b), (Action)&Cell::reformat,	b);
		}
	m.last(f.index("format"));
	m.last(next);
	m.last(prev);
	if( size < 8 )
		m.last( "* thru .", (Action)&Cell::indirect, 0);
	m.last( "asmblr", (Action)&Cell::asmblr, 0);

	return m.index();
}

void Cell::dodisplay(Bls &t){
	Cslfd *m;
	long afmt = fmt&~(F_ASCII|F_DOUBLE|F_FLOAT|F_DBLHEX|F_TIME);
	long cfmt = fmt&~F_SYMBOLIC;
	switch( size ){
	case 8:
		if( !(cfmt &= F_DOUBLE|F_DBLHEX) )
			cfmt = F_DOUBLE;
		break;
	default:
		if( !(cfmt &= ~(F_DOUBLE|F_DBLHEX)) )
			cfmt = F_HEX;
	}
	Format a(afmt?afmt:F_HEX, memory->core->symtab());
	Format c(cfmt);
	if( spy ) t.af( ">>> " );
	t.af( "%s/%d: ", a.f(addr), size );
	switch( size ){
		case 1: c.format |= F_MASKEXT8;  break;
		case 2: c.format |= F_MASKEXT16; break;
	}
	if( m = memory->core->peek(addr,0) ){
		long val;
		switch( size ){
			case 1: c.format |= F_MASKEXT8;
				val = m->chr;
				break;
			case 2: c.format |= F_MASKEXT16;
				val = m->sht;
				break;
			case 4: val = m->lng;
				m->dbl = m->flt;
			}
		t.af( "%s", c.f(val, m->dbl) );
		if( spy ) *spy = *m;
	} else
		t.af( "cannot read" );
}

void Cell::display(const char *error, int j){
	Bls t;

	trace( "%d.display()", this );	VOK;
	dodisplay(t);
	if( error ) t.af( " %s", error );
	Attrib attrib = ACCEPT_KBD|SELECTLINE;
	if( spy ) attrib |= DONT_CUT;
	memory->pad->insert(addr, attrib, (PadRcv*)this, carte(), "%s", t.text);
	if( j )
		memory->core->process()->journal()->insert("%s", t.text);
}

int Cell::changed(){
	int changed = 0;
	Cslfd *m;

	trace( "%d.changed()" ); OK(0);
	if( !spy ) return 0;
	m = memory->core->peek(addr);
	switch( size ){
		case 1: if( m->chr != spy->chr ) changed = 1;  break;
		case 2: if( m->sht != spy->sht ) changed = 1;  break;
		case 4: if( m->lng != spy->lng ) changed = 1;  break;
		case 8: if( m->dbl != spy->dbl ) changed = 1;  break;
	}
	if( changed ) display(0,1);
	return changed;
}


void Cell::setspy(long s){
	trace( "%d.setspy(%d)", this, s );	VOK;
	if( !s && spy ){
		delete spy;
		spy = 0;
	} else if( s )
		spy = new Cslfd;
	display();
}

void Cell::relative(long a){
	trace( "%d.relative(%d)", this, a );	VOK;
	long s = addr;
	long e = s + a * size;
	int incr = (a < 0) ? -size : size;
	while(s != e) {
		s += incr;
		memory->makecell(this, s);
	}
}

void Cell::indirect(){
	Cslfd *m;

	trace( "%d.indirect()", this );		VOK;
	if( !(m = memory->core->peek(addr,0)) )
	    display();			/* will have same problem */
	else
	    memory->makecell(this, size==1 ? (unsigned  char) m->chr
				 : size==2 ? (unsigned short) m->sht : m->lng);
}

char *Cell::help(){
    return "$=<expr> {update cell} | .=<expr> {open cell} | [/?]<string> {search}";
}

const char *Cell::enchiridion(long l){
	if (l == HELP_KEY)
		return "Memory Line Keyboard";
	return 0;
}

char *Cell::search(int dir, char *pat){
	Cell s(memory);

	trace("%d.search(%d,%s)", this, dir, pat?pat:"0"); OK("Cell::search");
	if( !pat || !*pat || !strcmp(pat,"?") || !strcmp(pat,"/") )
		pat = memory->prevpat;
	memory->prevpat = sf("%s", pat);
	int patlen = strlen(pat);
	s = *this;
	long t0 = time(0);
	for( s.addr = addr+dir*size; s.addr != addr; s.addr += dir*size ){
		Cslfd *m = memory->core->peek(s.addr, 0);
		if( !m ){
			memory->makecell(this,s.addr);
			return "memory not contiguously readable for search";
		}
		Bls t;
		s.dodisplay(t);
		char *p, pat0 = *pat;
		for( p = t.text; *p; ++p ){
			if( *p == pat0 && !strncmp(p, pat, patlen) ){
				memory->makecell(this,s.addr);
				return 0;
			}
		}
		const int timeout = 10;
		if( time(0) > t0+timeout ){
			memory->makecell(this,s.addr);
			return sf("search timeout (%d secs)", timeout);
		}
	}
}

char *Cell::kbd(char *s){	/* should eval against loader symbol frame */
	Parse y(G_DOLEQ_CONEX, 0);
	Expr *e;
	Bls t;

	trace( "%d.kbd(%s)", this, s );		OK("kbd");
	if( !*s ){
		relative(1);
		return 0;
	}
	if( s[0] == '/' )
		return search( 1, s+1);
	if( s[0] == '?' )
		return search(-1, s+1);
	if( (e = (Expr*)y.parse(s)) ){
		e->evaltext(memory->core->process()->globals, t);
		display(e->evalerr ? t.text
				   : memory->core->poke(addr,e->val.lng,size));
		return 0;
	}
	return memory->kbd(s);
}

void Cell::reformat(long f){
	trace( "%d.reformat(0x%X) 0x%X ", this, f, fmt ); VOK;
	fmt ^= int(f);
	if( f&F_TURNOFF )
		fmt &= int(~f);
	else
		fmt |= int(f);
	display();
}

void Cell::resize(int s){
	trace( "%d.resize(%d) %d", this, s, size ); VOK;
	size = s;
	reformat(0);
}

void Cell::asmblr(){
	trace( "%d.asmblr()" );	VOK;
	memory->core->process()->openasm(addr);
}

int Memory::changes(long verbose){
	Cell *c;
	long changes = 0, key = 0x40000000, spies = 0;	/* !!! */
	trace( "%d.changes()", this ); OK(0);

	pad->removeline(key);	
	for( c = cellset; c; c = c->sib )
		if( c->spy ){
			++spies;
			changes += c->changed();
		}
	if( verbose ){
		if( spies == 0 )
			pad->insert( key, SELECTLINE, "no spies" );
		else if( changes==0 )
			pad->insert( key, SELECTLINE, "no spies changed" );
	}
	return changes;
}	
