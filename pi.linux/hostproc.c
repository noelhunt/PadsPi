#include <sys/utsname.h>
#include <sys/procfs.h>
#include "core.h"
#include "process.h"
#include "master.h"
#include "sigmask.h"
#include "frame.h"
#include "memory.h"
#include "symtab.h"
#include "symbol.h"
#include "srcdir.h"
#include "asm.h"
#include "bpts.h"
#include "expr.h"
#include "hostcore.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <utmp.h>
SRCFILE("hostproc.c")

static char sccsid[ ] = "%W%\t%G%";

HostProcess::HostProcess(Process *sib, const char *p, const char *s, const char *c)
 : Process(sib,p,s,c) { }

void HostProcess::batch(){
	core = newCore(master);
	const char *error = core->open();
	if( error ){
		fprintf(stderr, "%s\n", error);
		exit(1);
	}
	CallStk *c = core->callstack();
	if (!c) {
		printf("cannot obtain callstack\n");
		long pc = core->pc();
		if (pc) {
			const char *s = symtab()->symaddr(pc);
			if (*s)
				printf("pc=%s\n", s);
			else
				printf("pc=0x%X\n", pc);
		} else
			printf("Cannot obtain pc\n");
		return;
	}
	for( long l = c->size-1; l>=0; --l )
		if( l<32 || l>c->size-32 ){
		Frame f = c->frame(l);
		Bls t;
		f.addsymbols();
		printf("%s\n", f.text(t));
		if( f.func ){
			BlkVars bv(f.func->blk(f.pc));
			Var *v;
			while( v = bv.gen() ){
				Bls tt;
				if( v->disc() == U_ARG ) continue;
				Expr *e = E_Sym(v);
				e->evaltext(&f, tt);
				printf("\t%s\n", tt.text);
			}
		}
	}
}

void HostProcess::takeover(){
	if (pad) {
		open(0);
		insert(ERRORKEY, "take over: already open");
		return;
	}
	if (fixsymtab())
		return;
	Pick("take over", (Action)&HostProcess::substitute, (long)this);
}

#define PROCFS

int HostProcess::fixsymtab(){
	const char *nstab;
	char file[80];

#ifndef PROCFS
	if (stabpath || hfn->stabfdsupported())
		return 0;
	sscanf(&comment[hfn->getpsfield()], "%s", file);
#else
	sprintf(file, "/proc/%s/exe", procpath);
	nstab = file;
#endif
	stabpath = sf("%s", nstab);
	comment = 0;
	master->insert(this);
	return 0;
}

int HostProcess::accept( Action a ){
	return a == (Action)&HostProcess::substitute;
}

void HostProcess::substitute(HostProcess *t){
	const char *error, *oldprocpath, *oldstabpath, *oldcomment;

	insert(ERRORKEY, 0);
	if( !core ){
		insert(ERRORKEY, "that ought to work - but it doesn't");
		return;
	}
	if( !core->online() ){
		insert(ERRORKEY, "cannot take over a coredump");
		return;
	}
	_bpts->lift();
	if( error = core->reopen(t->procpath, t->stabpath) ){
		_bpts->lay();
		insert(ERRORKEY, error);
		return;
	}
	oldprocpath = procpath;
	oldstabpath = stabpath;
	oldcomment = comment;
	procpath = t->procpath;
	stabpath = t->stabpath;
	comment = t->comment;
	master->makeproc( oldprocpath, oldstabpath, oldcomment );
	t->isdead = 1;
	master->insert(t);
	master->insert(this);
	banner();
	if( _asm ) _asm->banner();
	if( _bpts ) _bpts->banner();
	if( memory ) memory->banner();
	if( globals ) globals->banner();
	if( sigmsk ){
		sigmsk->banner();
		sigmsk->updatecore();
	}
	if( srcdir ) srcdir->banner();
	core->symtab()->banner();
	pad->clear();
	_bpts->lay();
	docycle();
}

void HostProcess::imprint(){
	int pid, ppid;

	sscanf(procpath, "%d", &pid);
	ppid = hfn->getppid(hfn->pidtoproc(pid));
	const char *parentpath = sf("%05d", ppid);
	insert(ERRORKEY, "parent=%s", parentpath);
	Process *p = master->search(parentpath);
	if (!p) {
		insert(ERRORKEY, "parent (%d) not opened", ppid);
		return;
	}
	_bpts->liftparents(p->_bpts);
}

void HostProcess::userclose(){
	if (sigmsk) {
		sigmsk->hostclose();
		delete sigmsk;
		sigmsk = 0;
	}
	Process::userclose();
}

void HostProcess::open(long ischild){
	Menu m, k, s;
	const char *error;

	Process::openpad();
	if( core ) return;
	if (fixsymtab())
		return;
	core = newCore(master);
	if (!core) {
		insert(ERRORKEY, "Processor type not supported");
		return;
	}
	insert(ERRORKEY, "Checking process and symbol table...");
	if (error = core->open()) {
		delete core;
		core = 0;
		if (ischild)
			m.last("open child", (Action)&HostProcess::open, 1);
		else
			m.last("open process", (Action)&HostProcess::open, 0);
		pad->menu(m);
		insert(ERRORKEY, error);
		return;
	}
	insert(ERRORKEY, core->symtab()->warn());
	globals = new Globals(core);
	_asm = core->newAsm();
	m.last( "Source",  (Action)&Process::srcfiles    );
	m.last( "Globals",   (Action)&Process::openglobals );
	m.last( "Memory", (Action)&Process::openmemory  );
	m.last( "Assembler", (Action)&Process::openasm     );
	m.last( "User Types",(Action)&Process::opentypes   );
	if( core->online() ){
		m.last("Journal", (Action)&Process::openjournal);
		m.last("Signals", (Action)&HostProcess::opensigmask);
		m.last("Bpt List", (Action)&HostProcess::openbpts);
		_bpts = new Bpts(core);
		_bpts->lay();
		if( ischild ) imprint();
		sigmsk = new SigMask(core);
	}
	if( core->online() ){
		s.last("run",		(Action)&Process::go);
		s.last("stop",		(Action)&Process::stop);
		s.last("current",	(Action)&Process::currentstmt);
		s.last("return",	(Action)&Process::pop);
		s.last("step into",	(Action)&Process::stepinto);
		s.last("step   1",	(Action)&Process::stmtstep, 1);
		s.last("step   5",	(Action)&Process::stmtstep, 5);
		s.last("step  25",	(Action)&Process::stmtstep, 25);
		s.last("step 100",	(Action)&Process::stmtstep, 100);
		s.last("step 500",	(Action)&Process::stmtstep, 500);
	} else
		s.first("stmt",		(Action)&Process::currentstmt);
	m.last(s.index("stmt"));
	if( core->online() ){
		k.last("kill?",		(Action)&HostProcess::destroy);
		m.last(k.index("kill"));
	}
	pad->menu(m.index("views"));
	pad->makecurrent();
	docycle();
	return;
}

void HostProcess::opensigmask()		{ if( sigmsk ) sigmsk->open(); }

void HostProcess::destroy(){
	IF_LIVE( !core->online() ) return;
	insert(ERRORKEY, core->destroy());
	docycle();
}

void HostProcess::stop(){
	IF_LIVE( !core->online() ) return;
#ifdef SIGSTOP
	if( !(sigmsk->mask&sigmsk->bit(SIGSTOP)) ) sigmsk->setsig(SIGSTOP);
#endif
	Process::stop();
}

Index HostProcess::carte(){
	Menu m;
	if( !strcmp(procpath,"!") ){
		m.last( "hang & open proc", (Action)&HostProcess::hangopen );
		m.last( "hang & take over", (Action)&HostProcess::hangtakeover );
	} else if( alldigits(procpath) ) {
		m.last( "open process",  (Action)&HostProcess::open );
		m.last( "take over",    (Action)&HostProcess::takeover );
		m.last( "open child", (Action)&HostProcess::open, 1 );
	} else
		m.last( "open coredump",(Action)&HostProcess::open );
	return m.index();
}


void HostProcess::hang(){
	int pid;
	char program[128];

	if (!(pid = hfn->hang(stabpath))) {
		PadsWarn("%s", "hang failed");	//insert(ERRORKEY, "Hang Failed");
		return;
	}
	procpath = sf("%d", pid);
	const char *ssave = stabpath;
	if (hfn->stabfdsupported())
		stabpath = 0;
	else {
		sscanf(stabpath, "%s", program);
		stabpath = sf("%s", program);
	}
	master->makeproc("!", ssave);
	master->insert(this);
}

void HostProcess::hangopen(){
	hang();
	open(0);
}

void HostProcess::hangtakeover(){
	hang();
	takeover();
}
