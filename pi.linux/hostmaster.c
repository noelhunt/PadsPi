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
SRCFILE("hostcore.c")

static char sccsid[ ] = "%W%\t%G%";

#define	K	DB_KERNELID

extern char *PATH;
extern Hostfunc *hfn;

HostMaster::HostMaster()	{ open(); }

Process	*HostMaster::newProcess(Process *pr, const char *p, const char *s, const char* c){
	return new HostProcess(pr, p, s, c);
}

void HostMaster::open(){
	Menu m, q, h;

	pad = new Pad( (PadRcv*) this );
	pad->options( TRUNCATE|ACCEPT_KBD );
	pad->name( "pi" );
	pad->banner( "pi = 3.1415926 (%s)", hfn->osname() ); // 3.141592653 589793
	pscmds = hfn->getpscmds();
	for(long i = 1; pscmds[i-1]; i++ )
		m.last( pscmds[i-1], (Action)&HostMaster::refresh, i );
	h.last("help", (Action)&HostMaster::openhelp);
	m.last(h.index("help"));
	m.last("quit?", (Action)&HostMaster::exit, 0);
	pad->menu(m.index("ps"));
	pad->makecurrent();
	refresh(0);
}

void HostMaster::openhelp(){
	extern int helptopic(const char*);
	helptopic("Introduction");
}

void HostMaster::exit() { PadsQuit(); }

#define PSOUT 128
#define PROCS 100
char *HostMaster::dopscmd(int cmd){
	char psout[PROCS][PSOUT];
	FILE *f, *Popen(const char*,const char*);
	int Pclose(FILE *);
	int pid, i, j, e;
	char *err = 0;
	SIG_TYP save;

	if (!cmd) return 0;
	cmd--;
	if (!pscmds[cmd]) return 0;
	save = signal(SIGCHLD, SIG_DFL);
	if (!(f = Popen(pscmds[cmd], "r"))) {
		err = SysErr( "cannot read from popen(): ");
		goto out;
	} 
	for (i = 0; i < PROCS && fgets(psout[i],PSOUT,f); i++){}
	if (e = Pclose(f)) {
		err = sf( "exit(%d): %s", e, pscmds[cmd] );
		goto out;
	}
	for (j = 0; j <= i; ++j)
		if (2 == sscanf(psout[j], " %d %[^\n]", &pid, psout[0]))
			makeproc(sf("%d",pid), 0, psout[0]);
out:
	signal(SIGCHLD, save);
	return err;
}

void HostMaster::refresh(int cmd){
	const char *error;
	Process *p;

	pad->clear();
	makeproc( "!", "a.out", "" );
	makeproc( "core", "a.out", "" );
	if( error = dopscmd(cmd) ) pad->error(error);
	for( p = child; p; p = p->sibling )
		if( p->core || (p->procpath && eqstr(p->procpath,"!")))
			insert(p);
}

char *HostMaster::kbd(char *s){
	char core[64], syms[64];	// Warning: core hides Master::core.
	const char *corep;
	int star = 0;
	HostProcess *p;
	static char helpstring[] = "Incorrect input: type ? for help";

	while( *s == ' ' ) ++s;
	switch( *s ){
	case '!':
		for( ++s; *s==' '; ++s ) {}
		makeproc("!", s, "");
		break;
	case '*':
		star = 1;
		for( ++s; *s==' '; ++s ) {}
	default:
		switch( sscanf(s, "%s %s \n", corep = core, syms) ){
		case 1:	if (!hfn->stabfdsupported() || !alldigits(corep))
				return helpstring;
			syms[0] = '\0';
		case 2:	if( alldigits(corep) ) {
				int id;
				sscanf(corep, "%d", &id);
				corep = sf("%05d", id);
			}
			p = (HostProcess*) makeproc(corep, syms[0]?syms:0, 0);
			if( star && p ) p->open(0);
			break;
		default:
			return helpstring;
		}
	}
	return 0;
}

char *HostMaster::help(){
	return "[*] <corepath | pid> <tables> {[open] process/dump} | !<cmd> {program}";
}

char *HostMaster::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Unix Pi Window";
		case HELP_MENU:		return "Unix Pi Menu Bar";
		case HELP_KEY:		return "Unix Pi Keyboard";
		case HELP_LMENU:	return "Unix Pi Line Menus";
		default:		return 0;
	}
}
