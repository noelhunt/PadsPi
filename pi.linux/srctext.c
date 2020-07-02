#include "lib.h"
#include "symbol.h"
#include "srctext.h"
#include "core.h"
#include "symtab.h"
#include "process.h"
#include "bpts.h"
#include "expr.h"
#include "format.h"
#include "frame.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
SRCFILE("srctext.c")

static char sccsid[] = "%W%\t%G%";

int SrcText::disc()	{ return U_SRCTEXT; }

SrcText::SrcText(Source *s, long c){
	trace( "%d.SrcText(%d,%d)", this, s, c ); VOK;
	source = s;
	compiletime = c;
	prevpat = "<no pattern>";
}

Core *SrcText::core() { return source->symtab->core(); }

Process *SrcText::process() { return core()->process(); }

void SrcText::DoNothing(){}

void SrcText::promote(){
	Menu m;
	trace( "%d.promote()", this ); VOK;
	if( promoted ){
		process()->openglobals();
		return;
	}
	Var *v;
	BlkVars bv(source->blk);
	int limit = 25;
	while( limit-->0 && (v = bv.gen()) )
		m.sort(sf("%s\240Sta",v->_text), (Action)&Frame::pickvar, (long)v);
	char *t = sf("\276 %s", basename(source->text()));
	m.first( t, (Action)&SrcText::DoNothing );
	process()->openglobals(&m);
	promoted = 1;
}
	
char *SrcText::read(Menu &m){
	Func *fun;
	long l;
	struct stat fdstat;
	char *p, *plast;
	Menu sub;

	if( edge ) return 0;
	if( fstat(fd, &fdstat) )
		return SysErr("cannot stat");
	if( fdstat.st_size == 0 )
		return "null file";
	if( !compiletime ) compiletime = source->symtab->modtime();
	if( modified(fd) > compiletime && !warned ){
		++warned;
		trace( "%d>%d", modified(fd), compiletime );
		return sf("modified since compilation at %24s",ctime(&compiletime));
	}
	warned = 0;
	fun = (Func*)source->child;
	long n = 0;
	while( fun  ){
		Bls toc;
		if (lastline < fun->lines.hi)
			lastline = (int)fun->lines.hi;
		if (fun->lines.lo > 0){
			toc.af( "%.55s\256%d", fun->textwithargs(),
				fun->lines.lo );
			sub.sort( toc.text, (Action) &SrcText::select, fun->lines.lo );
			++n;
		}
		fun = (Func*)fun->rsib;
	}
	if (lastline <= 0)
		return "not referenced by symbol table (cc -g?)";
	m.last(sub.index("func index"));
	edge = new char*[lastline+1];
	body = new char[fdstat.st_size+1];
	if( !ReadOK( fd, body, (int)fdstat.st_size ) )
		return SysErr( "read error" );
	funcnt = new int[lastline+1];
	stmts = new void*[lastline+1];
	for(fun = (Func*)source->child; fun; fun = (Func*)fun->rsib) {
		for (l = fun->lines.lo; l <= fun->lines.hi; l++)
			funcnt[l]++;
	}
	p = body;
	plast = body + fdstat.st_size;
	for(l = 1; l <= lastline && p < plast; l++ ){
		edge[l] = p;
		do {
			if (*p++ == '\n') {
				*(p-1) = 0;
				break;
			}
		} while( p < plast );
		trace( "%d %s", l, edge[l] );
	}
	if (l < lastline)
		lastline = (int)l - 1;
	pad->lines(lastline);
	return 0;
}

void SrcText::open(){
	Menu m;
	char *error = 0;
	struct stat stbuf;

	if( pad && edge ){
		pad->makecurrent();
		return;
	}
	if( !pad ) pad = new Pad( (PadRcv*) this );
	pad->options( ACCEPT_KBD|USERCLOSE|TRUNCATE);
	char *fname = path ? path : source->filename();
	char *expname = pathexpand(fname, process()->srcpath, 0);
	pad->banner( "Source Text: %s", expname ? expname : fname );
	pad->name( basename(fname) );
	pad->tabs(4);
	pad->error(0);
	if( !edge ){
		pad->menu( ZIndex );
		pad->lines(0);
		if( !expname || (fd = ::open(expname,0)) < 0 ) {
			error=  SysErr( "cannot open:" );
			fd = -1;
		}
		if( !error ){
			if( ::fstat( fd, &stbuf ) )
				error = "cannot stat"; 
			else if( (stbuf.st_mode&S_IFMT) == S_IFDIR )
				error = "is a directory";
		}
		if( !error ) error = read(m);
		if( fd >= 0 ) close(fd);
	}
	m.first("reopen" ,(Action)&SrcText::reopen );
	pad->menu(m);
	pad->error( error );
	pad->makecurrent();
}

void SrcText::reopen(){
	trace( "%d.reopen()", this );	VOK;
	free();
	open();
}

void SrcText::free(){
	trace( "%d.free()", this );	VOK;
	if( edge ) { delete edge; edge = 0; }
	if( body ) { delete body; body = 0; }
	if( stmts ) {
		for(long l = 1; l <= lastline; l++)
			if (funcnt[l] > 1)
				delete (MultiStmt *)stmts[l];
		delete stmts;
		stmts = 0;
	}
	if( funcnt ) { delete funcnt; funcnt = 0; }
}

void SrcText::userclose(){
	trace( "%d.userclose()", this );	VOK;
	free();
	if( pad ){
		delete pad;
		pad = 0;
	}
}

#define SETBPT 0
#define SETCLR 1
#define CLRBPT 2
#define NONBPT 3

void SrcText::linereq(int i, Attrib a){	/* profile hot spot ... */
	Stmt *stmt = 0;
	Bls t;
	static Index *ix;
	Index x;
	int bpty;
	Func *f;
	int j;

	trace( "%d.linereq(%d,0x%X)", this, i, a ); VOK;
	if (!ix) {		/* ... that's better */
		ix = new Index[4];
		Menu m[4];
		for( bpty = SETBPT; bpty <= NONBPT; ++bpty ){
		      m[bpty].last( "assembler",   (Action) &Stmt::asmblr    );
		      m[bpty].last( "open frame",  (Action) &Stmt::openframe );
		}
		m[SETBPT].first( "cond bpt",	(Action) &Stmt::conditional, (long)Q_BPT);
		m[SETBPT].first( "trace on",	(Action) &Stmt::settrace );
		m[SETBPT].first( "set  bpt",	(Action) &Stmt::dobpt, 1 );

		m[SETCLR].first( "trace on",	(Action) &Stmt::settrace );
		m[SETCLR].first( "set  bpt",	(Action) &Stmt::dobpt, 1 );
		m[SETCLR].first( "clear bpt",	(Action) &Stmt::dobpt, 0 );

		m[CLRBPT].first( "clear bpt",	(Action) &Stmt::dobpt, 0 );
		for (bpty = SETBPT; bpty <= NONBPT; ++bpty)
			ix[bpty] = m[bpty].index();
	}
	if( !edge ) open();
	if( !edge ) return;
	if( i<1 || i>lastline ) return;
	j = (int)i;
	while (!funcnt[j] && j < lastline)
		j++;
	if (funcnt[j] == 1) {
		bpty = NONBPT;
		if (!stmts[j] && (f = source->funcafter(j,0)))
			stmts[j] = source->stmtafter(f, j);
		stmt = (Stmt *)stmts[j];
		if (stmt && core()->online()) {
		    	if (!process()->bpts()->isbpt(stmt)) {
				bpty = SETBPT;
				if (stmt->lineno==i && stmt->condition==Q_BPT) {
					bpty = SETCLR;
					t.af("if( ? ) >>>");
				}
			} else {
				bpty = CLRBPT;
				if (stmt->lineno == i) {
					if (stmt->condition)
						t.af("if(%s)",
							stmt->condtext->text);
					t.af(">>>");
				}
			}
		}
		x = ix[bpty];
	} else if (funcnt[j] > 1) {
		if (!stmts[j])
			addmulti(j);
		x = multiindx(j, i, t);
	}
	t.af("%s", edge[i] ? edge[i] : "");
	pad->insert(i, (Attrib)a|ACCEPT_KBD, (PadRcv*)stmts[j], x, "%s", t.text);
}

char *SrcText::srcline(long i){
	trace( "%d.srcline(%d)", this, i ); OK("SrcText::srcline");
	if( !edge || i<1 || i>lastline ) return "";
	return edge[i];
}

void SrcText::select(long i, long svp){
	trace( "%d.select(%d,%d)", this, i, svp ); VOK;
	if( svp!=SVP || edge ) linereq(i, SELECTLINE);
}

void SrcText::go(){
	trace( "%d.go()", this ); VOK;
	process()->go();
}

void SrcText::currentstmt(){
	trace( "%d.currentstmt()", this ); VOK;
	core()->process()->currentstmt();
}

void SrcText::stmtstep(long i){
	trace( "%d.stmtstep()", this ); VOK;
	process()->stmtstep(i);
}

void SrcText::stepinto(){
	trace( "%d.stepinto()", this ); VOK;
	process()->stepinto();
}

char *SrcText::kbd(char *s){
	trace( "%d.kbd(%s)", this, s );		OK("kbd");
	if( edge ){
		if( *s == '/' )
			return contextsearch( lastline, s+1, 1 );
		else if( *s == '?' )
			return contextsearch( 1, s+1, -1 );
		else if( alldigits(s) )
			select( atoi(s) );
		else
			return help();
	} else {
		path = sf("%s",s);
		reopen();
	}
	return 0;
}

char *SrcText::help(){
	trace( "%d.help()", this );		OK("SrcText::help");
	return edge ? "<line number> {display line} | [/?]<string> {search}"
		    : "<path> {change source file name}";
}

const char *SrcText::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:
			return "Source Text Window";
		case HELP_MENU:
			return edge? "Source Text Menu Bar": "Source Text Opening Errors";
		case HELP_KEY:
			return edge? "Source Text Keyboard": "Source Text Opening Errors";
		case HELP_LMENU:
			return "Source Text Line Menus";
		case HELP_LKEY:
			return "Source Text Line Keyboard";
		default:
			return 0;
	}
}

char *SrcText::contextsearch(int from, const char *pat, int dir){
	trace( "%d.contextsearch(%d,%s,%d)", this, from, pat?pat:"", dir );
	OK("contextsearch");
	if( !pat || !*pat || !strcmp(pat,"?") || !strcmp(pat,"/") )
		pat = prevpat;
	prevpat = sf("%s", pat);
	int patlen = strlen(pat);
	char pat0 = *pat;
	for( int probe = from+dir; probe != from; probe += dir ){
		if( probe == 0 ) probe = lastline;
		if( probe == lastline+1 ) probe = 1;
		char *p;
		for( p = edge[probe]; *p; ++p ){
			if( *p == pat0 && !strncmp(p, pat, patlen) ){
				linereq(probe, SELECTLINE);
				return 0;
			}
		}
	}
	return sf("%s: not found", pat);
}

void SrcText::addmulti(long l){
	int cnt = funcnt[l];
	MultiStmt *ms = new MultiStmt(cnt);
	stmts[l] = ms;
	for(int i = 0; i < ms->nstmt; i++) {
		Func *f = source->funcafter((int)l, i);
		if (!f)
			break;
		ms->stmt[i] = source->stmtafter(f, (int)l);
	}
}

#define pk(x,y)	(((x) << 16)| (y))

Index SrcText::multiindx(long l, long li, Bls &t){
	Menu top;
	Stmt *s;
	Func *f;
	MultiStmt *ms = (MultiStmt *)stmts[l];
	static Index *ix;
	int bpty;

	if (!ix) {
		ix = new Index[3];
		Menu am[3];
		am[SETBPT].first("cond bpt", (Action)&MultiStmt::req, pk(0,2));
		am[SETBPT].first("trace on", (Action)&MultiStmt::req, pk(0,3));
		am[SETBPT].first("set  bpt", (Action)&MultiStmt::req, pk(0,4));

		am[SETCLR].first("trace on",  (Action)&MultiStmt::req, pk(0,3));
		am[SETCLR].first("set   bpt", (Action)&MultiStmt::req, pk(0,4));
		am[SETCLR].first("clear bpt", (Action)&MultiStmt::req, pk(0,5));

		am[CLRBPT].first("clear bpt", (Action)&MultiStmt::req, pk(0,5));
		for (bpty = SETBPT; bpty <= CLRBPT; ++bpty)
			ix[bpty] = am[bpty].index("all");
	}
	bpty = NONBPT;
	for (int i = 1; i <= ms->nstmt; i++) {
		Menu m;
		s = ms->stmt[i-1];
		if (!s)
			break;
		f = s->func();
		m.last("assembler",  (Action)&MultiStmt::req, pk(i,0));
		m.last("open frame", (Action)&MultiStmt::req, pk(i,1));
		if (core()->online()){
		    	if (!process()->bpts()->isbpt(s)) {
				if (s->lineno == li && s->condition == Q_BPT) {
					if (bpty != CLRBPT)
						bpty = SETCLR;
					m.first("trace on",  (Action)&MultiStmt::req, pk(i,3));
					m.first("set   bpt", (Action)&MultiStmt::req, pk(i,4));
					m.first("clear bpt", (Action)&MultiStmt::req, pk(i,5));
					t.af("if( ? ) >%d>", i);
				} else {
					if (bpty == NONBPT)
						bpty = SETBPT;
					m.first("cond bpt", (Action)&MultiStmt::req, pk(i,2));
					m.first("trace on", (Action)&MultiStmt::req, pk(i,3));
					m.first("set  bpt", (Action)&MultiStmt::req, pk(i,4));
				}
			} else {
				bpty = CLRBPT;
				m.first( "clear bpt", (Action)&MultiStmt::req, pk(i,5));
				if (s->lineno == li) {
					if (s->condition)
						t.af("if(%s)", s->condtext->text);
					t.af(">%d>", i);
				}
			}
		}
		top.last(m.index(f->textwithargs()));
	}
	if (bpty != NONBPT)
		top.last(ix[bpty]);
	return top.index();
}

void MultiStmt::req(long r){
	long stmtno = r >> 16;
	long op = r & 0xFFFF;
	Stmt *s;
	long i, last;
	
	if (stmtno) {
		i = stmtno - 1;
		last = i + 1;
	} else {
		i = 0;
		last = nstmt;
	}
	for ( ; i < last && (s = stmt[i]); i++)
		switch(op) {
		case 0:	s->asmblr(); break;
		case 1:	s->openframe(); break;
		case 2:	s->conditional(Q_BPT); break;
		case 3:	s->settrace(); break;
		case 4:	s->dobpt(1); break;
		case 5:	s->dobpt(0); break;
		}
}

char *MultiStmt::kbd(char *c){
	int i;
	char *err = 0;
	Stmt *s;

	for (i = 0; i < nstmt; i++) {
		if ((s = stmt[i]) && s->condition == Q_BPT) {
			for( ; i < nstmt; i++) {
				if ((s = stmt[i]) &&
				    s->condition == Q_BPT &&
				    (err = stmt[i]->kbd(c)))
					break;
			}
			return err;
		}
	}
	switch(*c) {
		case '/':
		case '?':
			return stmt[0]->kbd(c);
	}
	return "Incorrect input: type ? for help";
}

const char *MultiStmt::enchiridion(long l){
	if (l == HELP_KEY)
		return "Source Text Line Keyboard";
	return 0;
}

MultiStmt::MultiStmt(int i){
	if (i > 12)
		i = 12;
	nstmt = i;
	stmt = new Stmt*[i];
}

MultiStmt::~MultiStmt(){
	nstmt = 0;
	delete stmt;
}
