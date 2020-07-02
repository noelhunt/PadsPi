#include "univ.h"
#include "lib.h"
#include "dtype.h"
#include "symbol.h"
#include "symtab.h"
#include "srctext.h"
#include "core.h"
#include "process.h"
#include "bpts.h"
#include "phrase.h"
#include "parse.h"
#include "format.h"
SRCFILE("symbol.c")

static char sccsid[ ] = "%W%\t%G%";

int Symbol::disc()	{ trace( "%d.disc()", this ); return U_ERROR; }
int Var::disc()		{ trace( "%d.disc()", this ); return _disc; }

int Symbol::ok(){
	if( !this ) return 0;
	switch( disc() ){
		case U_ARG:
		case U_AUT:
		case U_BLOCK:
		case U_FST:
		case U_FUNC:
		case U_GLB:
		case U_MOT:
		case U_REG:
		case U_SOURCE:
		case U_STA:
		case U_STMT:
		case U_UTYPE:
			return 1;
	}
	return 0;
}

int Var::ok(){
	if( !this ) return 0;
	switch( disc() ){
		case U_ARG:
		case U_AUT:
		case U_FST:
		case U_GLB:
		case U_MOT:
		case U_STA:
		case U_REG:
			return 1;
	}
	return 0;
}

int BlkVars::disc()	{ return U_BLKVARS; }
int Block::disc()	{ return U_BLOCK; }
int Source::disc()	{ return U_SOURCE; }
int Func::disc()	{ return U_FUNC; }
int Stmt::disc()	{ return U_STMT; }
int UType::disc()	{ return U_UTYPE; }
int TypMems::disc()	{ return U_TYPMEMS; }

Symbol::Symbol( Symbol *up, Symbol *left, const char *t ){					
	trace( "%d.Symbol(^%d <%d %s)", this, up, left, t );
	_text = StrDup(t);
	parent = up;
	if( left ) left->rsib = this;
	this->lsib = left;
}

Stmt::Stmt(SymTab *stab, Block *up, Stmt *left ):Symbol(up,left,"<stmt>"){
	if(stab) stab->enter(this);
	process = stab ? stab->core()->process(): 0;
	condition = 0;
	condtext = 0;
}

Block::Block(SymTab *, Symbol *up, Block *left, const char *t )
 :Symbol(up,left,t) {}

BlkVars::BlkVars(Block *i)	{ VOK; b = i; v = 0; }

Var *BlkVars::gen(){
	trace( "%d.gen() %d %d", this, v, b );	OK(0);
	if(v) v = (Var*)v->rsib;
	while( !v && b && b->disc()==U_BLOCK ){
		v = b->var;
		b = (Block*)b->parent;
	}
	trace( "%s", v->dump() );
	return v;
}

Source::Source(SymTab *stab, Source *left, char *t, long c):Symbol(0,left,t){
	symtab = stab;
	blk = new Block( stab, this, 0, sf("%s.sta_blk",t) );
	srctext = new SrcText(this,c);
	bname = basename(t);
}

UType::UType(SymTab *stab, long b, long s, char *id):Symbol(0,0,id){
	trace( "%d.UType(%d,%d,%d,%s)", this, stab, b, s, id );
	begin = b;
	size = s;
	symtab = stab;
	canspecial = stab->core()->specialop(_text);
	if( stab ) stab->enter(this);
}

Var::Var(SymTab *stab, Block *up, Var *left, UDisc d, const char* id)
 :Symbol(up,left,id){
	if( (_disc = d)<=TOSYM && stab ) stab->enter( this );
}

Func::Func(SymTab *stab, const char* id, Source *src, long lo):Symbol(src,0,id){
	Func *r, *l;
	if (src) {
		lines.lo = lo;
		if (l = src->linefunc) {
			/* Order by starting line number */
			for (r = (Func *)l->rsib;;) {
				/* Assume in order, check higher 1st */
				if (r && lo > r->lines.lo) {
					l = r;
					r = (Func *)r->rsib;
				} else if (l && lo < l->lines.lo) {
					r = l;
					l = (Func *)l->lsib;
				} else
					break;
			}
			lsib = l;
			rsib = r;
			if (l)
				l->rsib = this;
			if (r)
				r->lsib = this;
		}
		if (!lsib)
			src->child = this;
		src->linefunc = this;
	}
	if (stab)
		stab->enter(this);
}

char *Symbol::dump(){
	static char t[128];

	if( !this ) return "0";
	sprintf(t,"%d %s %s %d %d",this,DiscName((UDisc)disc()),_text,range.lo,range.hi);
	return t;
}

int Func::regused(int r){
	trace("%d.regused(%d)", this, r); OK(0);
	Var *v;
	BlkVars bv(blk());
	while( v = bv.gen() )
		if( v->disc()==U_REG && v->range.lo==r ) return 1;
	return 0;
}

void Func::gather(){
	Source *src = source();

	trace("%d.gather() %s", this, dump() ); VOK;
	if( _blk = src->symtab->gatherfunc(this) )
		_blk->parent = src->blk;
	else
		_blk = src->blk;
}

char* Var::fmtlist(){
	trace("%d.fmtlist()", this); OK("");
	static Bls *b;
	if( !b ) b = new Bls;
	b->clear();
	long f = type.format();
	for( long bit = 1; bit; bit <<= 1 ) if( bit&f ){
		const char *fn = FmtName((int)bit);	// yuck
		do b->af("%c", *fn);			// yuck
		while ( *fn++ != ' ' );			// yuck
	}
	return b->text;
}

void Var::showutype(UType *u){
	trace("%d.showutype(%d)", this, u); VOK;
	u->show(LEAVE, SELECTLINE);
}

void Var::reformat(long o){
	trace("%d.reformat(0X%x)", this, o); VOK;
	type.reformat((int)o);
	show(SHOW, SELECTLINE);
}

Index Var::carte(){
	trace("%d.carte()", this); OK(ZIndex);
	Menu m;
	if( showorhide==SHOW )
		m.last("  hide  ", (Action)&Var::show, HIDE);
	else
		m.last("  show  ", (Action)&Var::show, SHOW);
	long f = type.formatset();
	long o = type.format();
	if( f ){
		Menu s;
		for( long bit = 1; bit; bit <<= 1 ) if( bit & f ){
			long b = bit;
			if( b&o ) b |= F_TURNOFF;
	    		s.last(FmtName((int)b), (Action)&Var::reformat, b);
		}
		m.last(s.index("format"));
	}
	DType *d = &type;
	while( d->isaryorptr() ) d = d->decref();
	if( d->isstrun() && d->utype()){
		UType *u = d->utype();
		m.last(u->type.text(), (Action)&Var::showutype, (long)u);
	}
	return m.index();
}

void Var::show(int soh, Attrib a){
	trace("%d.show(%d)", this, soh); VOK;
	if( _disc != U_MOT ) return;
	UType *u = (UType*) parent;
	if( !u ) return;
	TypMems g(u);
	Var *v;
	long k;
	for( k = (long) u; v = g.gen(); ++k )
		if( v == this ) break;
	if( soh != LEAVE ) showorhide = soh;
	SymTab *symtab = u->symtab;
	if( !symtab ) return;
	Pad *pad = symtab->pad();
	if( !pad ) return;
	Index ix = carte();
	const char *mark = showorhide==SHOW ? ">>>"  : "";
	pad->insert(k+1, a, (PadRcv*)this ,ix,
		"%s\t%s\t%s;\t%s", mark, type.text(), text(), fmtlist());
}

void UType::show(int soh, Attrib a){
	trace("%d.display()", this); VOK;
	TypMems g(this);
	Var *v;
	long k = (long) this;
	while( v = g.gen() ){
		v->show(soh);
		++k;
	}
	Pad *pad = symtab->pad();
	if( !pad ) return;
	Menu m;
	m.first("hide all", (Action)&UType::show, HIDE);
	m.first("show all", (Action)&UType::show, SHOW);
	pad->insert((long)this, a, (PadRcv*)this, m, "%s {", type.text());
	pad->insert(++k, 0, (PadRcv*)this, m, "} %s", type.text());
	pad->insert(++k, "");
}

void UType::gather(){
	trace("%d.gather()", this); VOK;
	mem = symtab->gatherutype(this);
	if( !mem )
		return;
	TypMems g(this);
	Var *v;
	for( int i = 1; v = g.gen(); ++i ){
		v->parent = this;
		if( i <= 2 ) v->showorhide = SHOW;
	}
	if( type.isstrun() ) show();
}

Source *Symbol::source(){
	trace( "%d.source() %s", this, dump() );
	return !this ? 0 : disc() == U_SOURCE ? (Source*)this : parent->source();
}

char *Symbol::text(long) { OK("Symbol::text"); return _text; }

char *Source::text(long) { OK("Source::text"); return bname; }

char *Source::filename(){
	switch ((int)(symtab->core()->process()->fnametype)) {
		case (int)FN_BASE:
			return bname;
		case (int)FN_FULL:
			if (dir && *_text != '/') {
				const char *cp = _text;
				if (!strncmp(cp, "./", 2))
					cp += 2;
				return sf("%s%s", dir, cp);
			}
			/* Fall through */
		case (int)FN_ENTRY:
		default:
			return _text;
	}
}

Stmt *Func::stmt(long pc){
	Stmt *s, *r;

	trace( "%d.stmt(%d)", this, pc ); OK(0);
	for( s = blk()->stmt; s; s = r ){
		r = (Stmt*)s->rsib;
		if( !r || r->range.lo>pc ) break;
	}
	return s && s->range.lo<=pc ? s : 0;
}

TypMems::TypMems(UType *i)	{ ut = i; v = 0; }

Var *TypMems::gen(){
	trace( "%d.gen()", this ); OK(0);
	if( ut ){
		if( !ut->mem ) ut->gather();
		v = ut->mem;
		ut = 0;
	} else if( v )
		v = (Var*)v->rsib;
	trace( "%s", v->dump() );
	return v;
}

Block *Func::blk(){
	OK(0);
	if(!_blk) gather();
	return _blk;
}

Block *Func::blk(long pc){
	Stmt *s;

	trace( "%d.blk(%d)", this, pc ); OK(0);
	if( !pc || !(s = stmt(pc)) || !s->parent ) return blk();
	return (Block*) s->parent;
}

Var *Func::argument(int a){
	trace( "%d.argument(%d)", this, a ); OK(0);
	BlkVars bv(blk());
	Var *v;
	int i = 0;
	while( v = bv.gen() )
		if( (v->disc()==U_ARG || v->disc()==U_REG) && ++i==a )
			return v;
	return 0;
}

char *Func::text(long){
	return sf( "%s()", this ? _text : "?" );
}

char *Func::textwithargs(){
	return namewithargs ? namewithargs : text();
}

char *Stmt::text(long pc){		// pass in a Bls argument?
	char buf[256];					// use a Bls instead

	trace("%d.text(%d)", this, pc); OK("Stmt::text");
	Source *src = source();
	if( !src ) return sf( "pc=%d", range.lo );
	sprintf( buf, "%s:%d", src->text(), lineno );
	if( pc && range.lo < pc )
		strcatfmt( buf, "+%u", pc-range.lo );
	return sf("%s", buf);
}

char *Stmt::journal(Bls &b){
	trace("%d.journal(%d)", this, &b); OK("Stmt::journal");
	char s[81], *cp;
	
	sprintf(s, "%.80s", srcline());
	for(cp = s; *cp == '\t'; cp++)
		*cp = ' ';
	b.af("%s %s %s%s", text(), condtext->text, func()->text(), s);
	return b.text;
}

char *Stmt::srcline(){
	trace("%d.srcline()", this); OK( "Stmt::srcline");
	Source *s = source();
	if( !s || !s->srctext ) return "";
	return s->srctext->srcline(lineno);
}

void Stmt::select(long svp){
	trace( "%d.select(%d)", this, svp );	VOK;
	Source *src = source();
	if( src ) src->srctext->select(lineno, svp);
	else asmblr();
}

char *Stmt::contextsearch(char *pat, int dir){
	trace( "%d.conetxtsearch(%s,%d)", this, pat?pat:"", dir); OK("search");
	Source *src = source();
	if( src )
		return src->srctext->contextsearch(lineno, pat, dir);
	return "can't search";
}

Pad *Stmt::srcpad() { return source()->srctext->pad; }

void Stmt::error(const char *s){
	trace( "%d.error(%s)", this, s );		VOK;
	Line l;
	l.text = strdup(s);
	l.object = this;
	l.attributes |= SELECTLINE;
	l.key = lineno;
	srcpad()->insert( l );
}

char *Stmt::kbd(char *s){
	Parse y(G_EXPR, 0);
	trace( "%d.kbd(%s)", this, s );		OK("kbd");
	if( condition != Q_BPT ){
		switch( *s ){
			case '/': return contextsearch(s+1,  1);
			case '?': return contextsearch(s+1, -1);
		}
		process->openframe( range.lo, s );
		return 0;
	}
	Expr *newcond = (Expr*)y.parse(s);
	if( newcond ){
		conditional(newcond);
//		condition = newcond;			/* NOTE */
//		if( !condtext ) condtext = new Bls;
//		condtext->clear();
//		condtext->af("%s", newcond->text() );
		dobpt(1);
		select();
	} else {
		dobpt(0);
		error( sf("%s: %s", y.error, s) );
	}
	return 0;
}

char *Stmt::help(){
	trace( "%d.help()", this );
	return condition == Q_BPT
		? "<expr> {breakpoint condition}"
		: "<expr> {eval in frame} | [/?]<string> {search}";
}

const char *Stmt::enchiridion(long l){
	if (l == HELP_KEY)
		return "Source Text Line Keyboard";
	return 0;
}

void Stmt::conditional(Expr *e){
	trace( "%d.conditional()", this );	VOK;
	condition = e;
	if( !condtext ) condtext = new Bls;
	condtext->clear();
	condtext->af("%s", condition==Q_BPT ? "?" : condition->text() );
	select();
}

Func *Source::funcafter(int l, int count){
	if (!linefunc)
		return 0;
	while (l >= linefunc->lines.lo && linefunc->rsib)
		linefunc = (Func *)linefunc->rsib;
	while (linefunc->lsib &&  l < ((Func*)linefunc->lsib)->lines.lo)
		linefunc = (Func *)linefunc->lsib;
	Func *f = linefunc;
	do {
		if (l <= f->lines.hi && l >= f->lines.lo && count-- == 0)
			return f;
	} while (f = (Func *)f->lsib);
	return linefunc;
}

static const int large = 0x7FFFFFFF;

Stmt *Source::stmtafter(Func *f, int l){
	Stmt *s;
	Stmt *bests = 0;
	int diff;
	int bestdiff = large;
	
	if (!linefunc)
		return 0;
	for (s = f->blk()->stmt; s; s = (Stmt*)s->rsib) {
		diff = s->lineno - l;
		if (diff >= 0) {
			if (!diff)
				return s;
			if (diff < bestdiff) {
				bestdiff = diff;
				bests = s;
			}
		}
	}
	return bests;
}

Func *Stmt::func(){
	trace("%d.func()", this); OK(0);
	Source *src = source();
	if( !src ) return 0;
	return (Func*) src->symtab->loctosym(U_FUNC, range.lo);
}

void Stmt::asmblr(){
	trace( "%d.asmblr() %d", this, process );	VOK;
	if( process ) process->openasm(range.lo);
}

void Stmt::settrace(){
	trace("%d.settrace()", this); VOK;
	static Expr *zero = 0;
	// bpts()->pad->makecurrent();
	conditional(E_IConst(0));
	dobpt(1);
}

void Stmt::dobpt(int setorclr){
	trace( "%d.dobpt(%d) %d", this, setorclr, process ); VOK;
	if( condition == Q_BPT ){
		condtext = 0;
		condition = 0;
		if( setorclr )
			process->bpts()->set( this );
		else
			select();
	} else {
		if( setorclr )
			process->bpts()->set( this );
		else {
			if( condtext ) delete condtext;
			condtext = 0;
			condition = 0;
			process->bpts()->clr( this );
		}
	}
}

void Stmt::openframe(){
	trace( "%d.openframe()", this ); VOK;
	process->openframe( range.lo );
}

Index UType::carte(Op op){
	Menu m;
	Var *v;
	TypMems tm(this);
	long n = 0;
	char *on = (char*) OpName(op);
	Action a = op==O_ARROW ? (Action)&Phrase::applyarrow : (Action)&Phrase::applydot;		// C++ bug (Action)

	trace( "%d.carte(%s)", this, on ); OK(ZIndex);
	if( canspecial )
		m.last( canspecial, (Action)&Phrase::applyunary, O_SPECIAL );
	while( v = tm.gen() ){
		Bls field( "$%s%s\240", on, v->_text );
		m.sort( field.text, a, (long) v );
		++n;
	}
	a = op==O_ARROW ? (Action)&Phrase::allstar : (Action)&Phrase::alleval;
	m.first(sf("$%s*",on), a, (long)this);
	return m.index(n>2 ? sf("$%sid",on) : 0);
}

Source::~Source()	{ delete srctext; }
Stmt::~Stmt()		{}
UType::~UType()		{}
Var::~Var()		{ /* on the stack in Expr */ }
Func::~Func()		{ type.free(); }

Block::~Block(){
	while( var ){
		var->type.free();
		delete var;
		var = (Var*)var->rsib;
	}
}
