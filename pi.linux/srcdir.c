#include "srcdir.h"
#include "process.h"
#include "symbol.h"
#include "srctext.h"
#include "symtab.h"
SRCFILE("srcdir.c")

static char sccsid[ ] = "%W%\t%G%";

int SrcDir::disc()	{ return U_SRCDIR; }

SrcDir::SrcDir(Process *p){
	trace( "%d.SrcDir(%d)", this, p );	VOK;
	process = p;
}

void SrcDir::banner(){
	trace( "%d.banner()", this );	VOK;
	if( pad ){
		pad->name( "Src Files" );
		pad->banner( "Source Files: %s", process->procpath );
	}
}

void SrcDir::hostclose(){
	trace( "%d.hostclose()", this );	VOK;
	if( pad ) delete pad;
	pad = 0;
}

char *SrcDir::help(){
	trace( "%d.help()", this );	OK("SrcDir::help");
	return "<path> {set source path prefix}";
}

const char *SrcDir::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Source Files Window";
		case HELP_MENU:		return "Source Files Menu Bar";
		case HELP_KEY:		return "Source Files Keyboard";
		case HELP_LMENU:	return "Source Files Line Menus";
		default:		return 0;
	}
}

char *SrcDir::kbd(char *s){
	trace( "%d.kbd(%s)", this, s );	OK("kbd");
	process->srcpath = sf("%s",s);
	pad->insert( 1, SELECTLINE|DONT_CUT, " Source Path: %s", s );
	return 0;
}

void SrcDir::fileoption(long l){
	process->fnametype = (FName)l;
	update();
}

void SrcDir::open(){
	Source	*r;

	trace( "%d.open()", this );	VOK;
	if( !pad ){
		r = process->symtab()->root();
		if( !r ) return;
		if( !r->rsib ){
			r->srctext->open();
			return;
		}
		pad = new Pad( this );
		banner();
		pad->options(ACCEPT_KBD|SORTED);
		Menu m;
		m.first("full path", (Action)&SrcDir::fileoption,(long)FN_FULL);
		m.first("basename", (Action)&SrcDir::fileoption, (long)FN_BASE);
		m.first("default", (Action)&SrcDir::fileoption, (long)FN_ENTRY);
		pad->menu(m.index("src"));
		update();
	}
	pad->makecurrent();
}

void SrcDir::update(){
	Source	*s;
	long	k = 2;
	Menu	m;

	m.last( "open source file", (Action)&SrcText::open);
	m.last( "file statics", (Action)&SrcText::promote);
	pad->clear();
	if (process->srcpath)
		pad->insert(1,DONT_CUT," Source Path: %.256s",process->srcpath);
	for(s = process->symtab()->root(); s; s = (Source*)s->rsib ) {
		if (!s->child)
			continue;
		pad->insert(k++,DONT_CUT,(PadRcv*)s->srctext,m,s->filename());
	}
}
