#include "frame.h"
#include "core.h"
#include "symbol.h"
#include "symtab.h"
#include "format.h"
#include "expr.h"
#include "phrase.h"
#include "parse.h"
#include "process.h"
#include "bpts.h"
#include "journal.h"
SRCFILE("frame.c")

static char sccsid[ ] = "%W%\t%G%";

CallStk::CallStk(long s, Core *c){
	fpf = new FpFrame[size = s];
	core = c;
}

CallStk::~CallStk(){
	delete fpf;
}

Frame CallStk::frame(long l){
	Frame f = core->frameabove(l==0 ? 0 : fpf[l-1].fp);	// cfront bug
	return f;
}

int Frame::disc()	{ return U_FRAME; }

Frame::Frame(Core *c){	// cfront bug - c = 0
	core = c;
	level = 0;
	fp = ap = pc = nargs = regsave = regbase = 0;
	func = 0;
	pad = 0;
	phraset = 0;
}

Frame::Frame(){		// cfront bug - c = 0
	core = 0;
	level = 0;
	fp = ap = pc = nargs = regsave = regbase = 0;
	func = 0;
	pad = 0;
	phraset = 0;
}

void Frame::pop(){
	trace("%d.pop()", this); VOK;
	core->process()->pop(0);
}

SymTab *Frame::symtab()	{ OK(0); return core->symtab(); }
	
int Frame::addsymbols(){
	trace("%d.addsymbols()", this);	OK(0);
	func = (Func*) symtab()->loctosym(U_FUNC, pc);
	return func!=0;
}

void Frame::hostclose(){
	Phrase *p, *psib;

	trace("%d.hostclose()", this); VOK;
	if( pad ) delete pad;
	pad = 0;
	invalidate();
	for( p = phraset; p; p = psib ){
		psib = p->sib;			// new malloc
		p->expr->setspy(0);		// why?
		delete p;
	}
}

void Frame::banner(){
	Bls t;
	trace("%d.banner()", this); VOK;
	if( pad ){
		pad->banner("%s:", text(t));
		pad->name(func ? func->text() : "?()");
	}
}

void Frame::opencallersframe(){
	trace("%d.opencallersframe()", this); VOK;
	Frame *c = caller();
	if( c ) c->open();
}

Frame *Frame::caller(){
	trace("%d.caller()", this); OK(0);
	return core->process()->frame(level+1);
}

void Frame::open(long svp){
	Menu m;

	trace("%d.open()", this); VOK;
	if( !pad ){
		pad = new Pad( (PadRcv*) this );
		pad->options( ACCEPT_KBD );
		if( func->stmt(pc) )
			m.last( sf("src text",func->text()), (Action)&Frame::select );
		if( core->process()->frame(level+1) )
			m.last( "caller's frame", (Action) &Frame::opencallersframe );
		m.last( "changed spies", (Action)&Frame::changes, 1 );
		m.last( varcarte() );
		m.last( regcarte() );
		pad->menu(m.index("frame"));
	}
	banner();
	if( svp!=SVP ) pad->makecurrent();
}

int Frame::changes(long verbose){
	Phrase *p;
	long changes = 0, key = 0x40000000, spies = 0;

	trace("%d.changes()", this); OK(0);

	for( p = phraset; p; p = p->sib )
		if( p->expr->spy ){
			Bls b;
			++spies;
			if( p->changed(b) ){
				++changes;
				core->process()->journal()->insert("%s",b.text);
			}
		}
	if( verbose && pad ){
		pad->removeline(key);	
		if( spies == 0 )
			pad->insert( key, SELECTLINE, "no spies in this frame" );
		else if( changes==0 )
			pad->insert( key, SELECTLINE, "no spies changed" );
	}
	return changes;
}	

char *Frame::text(Bls &buf){
	Var *arg;
	Stmt *stmt = 0;

	trace("%d.text()", this);	OK("Frame::text");
	if( !func ) return sf( "pc=0x%X ?()", pc );
	if( core->online() )
		stmt = core->process()->bpts()->bptstmt(pc);
	if( !stmt ) stmt = func->stmt(pc);
	if( stmt )
		buf.af("%s", stmt->text(pc));
	else
		buf.af( "%s+%d", func->_text, pc - func->range.lo );
	buf.af(" %s(", func->_text);
	for( int i = 1; arg = func->argument(i); ++i ){
		if( i>1 ) buf.af(",");
		E_Sym(arg)->evaltext(this,buf);		// should use a stack expr
	}
	return buf.af( ")" );
}

void Frame::select(long svp){
	Stmt *stmt = 0;

	trace("%d.select(%d)", this, svp);	VOK;
	if( core->online() )
		stmt = core->process()->bpts()->bptstmt(pc);
	if( !stmt && func ) stmt = func->stmt(pc);
	if( stmt ) stmt->select(svp);
}

long Frame::saved(int r, int size) { return core->saved(this,r,size); }

long Frame::regloc(int r, int size){		/* what about r>=12 on vax ? */
	if( !this ) return 0;
	long loc = saved(r,size);
	if( loc ) return loc;
	if( level == 0 ) return core->regloc(r,size);
	return core->process()->frame(level-1)->regloc(r,size);
}

long Frame::locate(Var *v){
	trace("%d.locate(%d) %s %d", this, v, v->_text, v->range.lo);
	IF_LIVE( !v ) return 0;
	switch( (int) v->disc() ){
	case U_GLB:
	case U_STA:
	case U_FST:
	case U_FUNC:
		IF_LIVE(!v->range.lo) return 0;		/* this == 0 */
		return    v->range.lo;
	case U_ARG:
		IF_LIVE(!this || !ap) return 0;
		return ap+v->range.lo;
	case U_AUT:
		IF_LIVE(!this || !fp) return 0;
		return fp+v->range.lo;
	case U_REG:
		IF_LIVE(!this) return 0;
		if( level == 0 )	/* SAME AS CODE ABOVE! */
			return core->regloc((int)v->range.lo, v->type.size_of());
		return core->process()->frame(level-1)->regloc((int)v->range.lo, v->type.size_of());
	}
	IF_LIVE( 1 ) return 0;
}

Cslfd *Frame::peek(long loc, Cslfd* fail){
	trace("%d.peek(0x%X,%d)", this, loc, fail); OK(0);
	return core->peek(loc, fail);
}

char *Frame::special(char *id, long loc ){
	trace("%d.special(%s,0x%X)", this, id, loc); OK(0);
	return core->special(id, loc);
}

char *Frame::peekstring(long loc, char *fail){
	trace("%d.peekstring(0x%X,%s)", this, loc, fail?fail:"0"); OK(0);
	return core->peekstring(loc, fail);
}

char *Frame::poke(long loc, long data, int bytes){
	trace("%d.poke(0x%X,0x%X,%d)", this, loc, data, bytes); OK(0);
	return core->poke(loc, data, bytes);
}

char *Frame::pokedbl(long loc, double data, int bytes)	/* broke float poke */
{
	trace("%d.pokedbl(0x%X,%g,%d)", this, loc, data, bytes); OK(0);
	return core->pokedbl(loc, data, bytes);
}

char *Frame::blockmove(long s, long d, long ct){
	trace("%d.blockmove(0x%X,0x%X,%d)", this, s, d, ct); OK("Frame::blockmove");
	return core->blockmove(s, d, ct);
}

char *Frame::help(){
	trace("%d.help()", this);
	return "<expr> {evaluate in this scope}";
}

const char *Frame::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Frame Window";
		case HELP_MENU:		return "Frame Menu Bar";
		case HELP_KEY:		return "Frame Keyboard";
		case HELP_LMENU:
		case HELP_LKEY:		return "Frame Lines";
		default:		return 0;
	}
}

char *Frame::kbd(char *s){
	Parse y(G_EXPR,0);
	Expr *e;
	const char *error;

	trace("%d.kbd(%s)", this, s);	OK("kbd");
	if( !(e = (Expr*)y.parse(s)) ){
		pad->error( "%s: %s", y.error, s );
		return 0;
	}
	if( error = makephrase(e) ) pad->error( "%s", error );
	return 0;
}

#define GAP 2
void Frame::freekey(long k){
	Phrase *p;
	int found = 0;

	trace("%d.freekey(%d)", this, k); VOK;
	for( p = phraset; p; p = p->sib )
		if( p->key >= k ){
			p->key += GAP;
			found = 1;
		}
	if( found ) pad->makegap(k,GAP);
}

char *Frame::makephrase(Expr *e,long k){
	Bls b;
	Phrase *p;
	char *error = 0;

	trace("%d.makephrase(%d,%d)", this, e, k); OK("Frame::makephrase");
	if( !e ) return "not and expr";
	if( !k ) k = UniqueKey();
	for( p = phraset; p; p = p->sib )
		if( p->key == k || p->expr == e) break;
	if( !p ) phraset = p = new Phrase(this, phraset, e, k);
	e->evaltext(this,b);
	if( e->evalerr ){
		Attrib a = ACCEPT_KBD;
		switch( e->op ){
		case O_TYPEOF: break;
		case O_INDEX:  if( e->sub2->op == O_RANGE ) break;
		default:
		   pad->insert(p->key, a, (PadRcv*)p ,e->carte(0), "%s", e->text());
		}
		return sf( "%s", b.text );
	}
	Attrib a = ACCEPT_KBD|SELECTLINE;
	if( e->spy ) a |= DONT_CUT;
	pad->insert(p->key, a, (PadRcv*)p, e->carte(this), "%s", b.text);
	return 0;

}

void Frame::pickvar(Var *v){
	const char *error;

	trace("%d.pickvar(%d)", this, v); VOK;
	if( error = makephrase(E_Sym(v)) )		/* who owns this expr? */
		pad->error(error);
}

void Frame::pickall(){
	Var *v;
	const char *error;

	BlkVars bv(func->blk(pc));
	while (v = bv.gen())
		if (error = makephrase(E_Sym(v))) {
			pad->error(error);
			break;
		}
}

Index Frame::varcarte(){
	Var   *v, *d;
	Menu  m;
	Menu  u;
	int   n, i;
	const char *q = "'''''''''''''''";

	if( !func ) return ZIndex;
	BlkVars bv(func->blk(pc));
	for(i = 0; v = bv.gen(); i++){
		BlkVars  dup(0);
		dup = bv;
		for( n = 0; d = dup.gen(); )
			if( !strcmp(d->_text,v->_text) ) ++n;
		Bls id( "%0.60s%0.*s\240%s", v->_text, n, q, DiscName((UDisc)v->disc()));
		if (v->_text[0] == '_')
			u.sort( id.text, (Action)&Frame::pickvar, (long) v);
		else
			m.sort( id.text, (Action)&Frame::pickvar, (long) v);
	}
	m.last(u.index());
	if (i > 1)
		m.first("all\240", (Action)&Frame::pickall);
	return m.index("vars");
}

void Frame::ambiguous(const char *id, BlkVars &bl){
	Var *v;					VOK;

	while( v = bl.gen() )
		if( !strcmp(v->text(),id) ){
			PadsWarn("more than one %s in %s (use the menu)",
				id, func->text() );
			return;
		}
}

Var *Frame::idtosym(const char *id){	// ever called with id==0 ?
	Var *v;

	trace("%d.idtosym(%s)", this, id?id:"0");	OK(0);
	if( !id ) return 0;
	if( func ) {
		BlkVars bl((Block*)func->blk()->child);		// blk(pc) ?
		while( v = bl.gen() )
			if( !strcmp(v->text(),id) ){
				ambiguous(id, bl);
				return v;
			}
	}
	BlkVars bg(core->symtab()->blk());	// idtosym doesn't work for regs
	while( v = bg.gen() )
		if( !strcmp(v->text(),id) ) return v;
	return (Var*) symtab()->idtosym(SSet(U_GLB,U_FUNC), id);
}

Index Frame::carte(){
	trace("%d.carte()", this);	OK(ZIndex);
	if( !func ) return ZIndex;
		Bls o("open %s frame", func->text());
		Menu m(o.text, (Action)&Frame::open);
		Stmt *stmt = func->stmt(pc);
		if( stmt ){
			Bls s("show %s", stmt->text());
			m.last(s.text, (Action)&Frame::select);
		}
		if( core->online() && core->process()->frame(0) == this )
			m.last("return", (Action)&Frame::pop);
		return m.index();
}

int Globals::disc()	{ return U_FRAME; }

Globals::Globals(Core *c):Frame(c){
	trace("%d.Globals(%d)", this, c);	VOK;
}

void Globals::banner(){
	trace("%d.banner()", this); VOK;
	if( pad ){
		pad->banner("Globals: %s", core->procpath());
		pad->name("Globals");
	}
}

const char *Globals::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Globals Window";
		case HELP_MENU:		return "Globals Menu Bar";
		case HELP_KEY:		return "Frame Keyboard";
		case HELP_LMENU:
		case HELP_LKEY:		return "Frame Lines";
		default:		return 0;
	}
}

void Globals::open(long){
	Var *g;
	Menu m, v;
	int i;

	trace( "%d.open()", this );	VOK;
	if( !pad ){
		BlkVars bv(core->symtab()->blk());
		pad = new Pad( (PadRcv *) this );
		pad->options(ACCEPT_KBD);
		banner();
		for(i = 0; g = bv.gen(); i++){
			if( g->range.lo && g->disc() == U_GLB ){
				Bls id("%0.60s\240%s", g->_text, DiscName((UDisc)g->disc()));
				v.sort(id.text, (Action)&Frame::pickvar, (long) g);
			}
		}
		if (i > 1)
			v.first("all\240", (Action)&Globals::pickall);
		m.first("spies", (Action)&Frame::changes, 1);
		m.last(v.index("vars"));
		m.last(regcarte());
		pad->menu(m);
		ix = m.index();
	}
	pad->makecurrent();
}

void Globals::addvars(Menu *m){
	Menu combine;
	trace("%d.addvars(%d)", this, m);	VOK;
	if( !pad ) open();
	combine.first(ix);
	combine.last(m->index());
	pad->menu(combine);
	ix = combine.index();
	pad->makecurrent();
}

void Globals::pickall(){
	Var *v;
	const char *error;

	BlkVars bv(core->symtab()->blk());
	while (v = bv.gen())
		if (v->range.lo && v->disc() == U_GLB &&
		    (error = makephrase(E_Sym(v)))) {
			pad->error(error);
			break;
		}
}

void Frame::regall(){
	Var *v;
	const char *error;

	BlkVars bv(core->symtab()->blk());
	while (v = bv.gen())
		if (v->disc() == U_REG && (error = makephrase(E_Sym(v)))) {
			pad->error(error);
			break;
		}
}

Index Frame::regcarte(){
	Var *v;
	Menu m;

	trace("%d.regcarte()", this);	OK(ZIndex);
	BlkVars bv( core->symtab()->blk() );
	while( v = bv.gen() )
		if( v->disc() == U_REG )
			m.last(v->text(), (Action)&Frame::pickvar, (long)v);
	m.first("all", (Action)&Frame::regall);
	return m.index("regs");
}
