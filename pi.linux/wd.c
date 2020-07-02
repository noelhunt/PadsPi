#include "univ.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include "wd.h"
SRCFILE("wd.c")

static char sccsid[ ] = "%W%\t%G%";

void NewWd() { new Wd; }

Wd::Wd(){
	pad = new Pad((PadRcv*) this);
	pad->options(ACCEPT_KBD);
	pad->name("pwd/cd");
	pad->banner("Working Directory:");
	prevwd = 0;
	pwd();
}

Index Wd::carte(){
	struct stat stbuf;
	Menu m;
	struct dirent *dp;
	DIR *dirp;

	dirp = ::opendir(".");
	if (dirp == NULL)
		return m.index();
	for ( dp = ::readdir(dirp); dp!=NULL; dp=::readdir(dirp) ) {
		char dn[26];
		sprintf(dn, "%0.25s", dp->d_name);
		if( dp->d_ino == 0 
		 || ::stat(dn, &stbuf)== -1
		 || (stbuf.st_mode&S_IFMT)!=S_IFDIR )
			continue;
		long opand = (long) sf("%s/%s", getwd, dn);
		m.sort(sf("%s\240", dn), (Action)&Wd::kbd, opand);
	}
	::closedir(dirp);
	return m.index("chdir");
}

const char *Getwd(){
	char pathname[1024];
	const char *e = 0;
	SIG_TYP save = signal(SIGCHLD, SIG_DFL);
	if (!getcwd(pathname, sizeof(pathname)))
		e = "getwd error";
	
	signal(SIGCHLD, save);
	return e ? e : sf("%s",pathname);
}

char *Wd::help(){
	trace( "%d.help()", this );
	return "[cd] <path> {change working directory}";
}

const char *Wd::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Directory Window";
		case HELP_MENU:		return "Directory Menu Bar";
		case HELP_KEY:		return "Directory Keyboard";
		case HELP_LMENU:	return "Directory Line Menus";
		default:		return 0;
	}
}

char *Wd::kbd(char *s){
	if( s[0]=='c' && s[1]=='d' && (s[2]==' '||s[2]==0) ) s += 2;
	while( *s == ' ' ) ++s;
	if( *s==0 ){
		char *e = getenv("HOME");
		if( e ) s = e;
	}
	if( chdir(s) == -1 ){
		pad->insert(key++, "cannot cd %s", s);
		prevwd = 0;
	}
	pwd(SELECTLINE);
	return 0;
}

void Wd::pwd(Attrib a){
	if( prevwd )
		pad->insert(key, a, (PadRcv*)this, ix, "%s", getwd);
	getwd = Getwd();
	ix = carte();
	pad->menu(ix);
	pad->insert(++key, a|DONT_CUT, (PadRcv*)this, ix, "wd=%s", getwd);
	prevwd = getwd;
}
