#include "master.h"
#include "process.h"
SRCFILE("master.c")

static char sccsid[ ] = "%W%\t%G%";

int Master::disc()		{ return U_MASTER; }
void Master::open()		{ }

Process	*Master::newProcess(Process*,const char*,const char*,const char*) {
	return 0;
}

char *Master::kbd(char*s)	{ return PadRcv::kbd(s); }
char *Master::help()		{ return PadRcv::help(); }

Master::Master(){
	trace( "%d.Master()", this );
	child = 0;
	core = 0;
}

Process *Master::makeproc(const char* proc, const char *stab, const char *comment){
	Process *p;

	for( p = child; p; p = p->sibling )
		if( !p->isdead && eqstr(proc,p->procpath) ) {
			if ( p->core )
				return p;
		 	if ( eqstr(stab,p->stabpath) ){
				p->comment = comment?sf("%s",comment):(char*)0;
				insert(p);
				return p;
			}
		}
	child = newProcess(child, proc, stab, comment);
	child->master = this;
	insert(child);
	return child;
}

void Master::insert(Process *p){
	trace( "%d.insert(%d)", this, p ); VOK;
	IF_LIVE( !p || p->disc()!=U_PROCESS ) return;
	if( p->isdead ){
		pad->removeline( (long)p );
		return;
	}
	const char *pp = p->procpath;
	const char *sp = p->stabpath;
	const char *ct = p->comment;
	pad->insert( (long)p, SELECTLINE, (PadRcv*)p, p->carte(),
		"%s %s %s", pp?pp:"", sp?sp:"", ct?ct:"");
}

Process *Master::search(const char *path){
	Process *p;
	OK(0);
	for( p = child; p; p = p->sibling )
		if( eqstr(path, p->procpath) && p->core ) return p;
	return 0;
}
