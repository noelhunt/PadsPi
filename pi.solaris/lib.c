#include <pads.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"
SRCFILE("lib.c")

static char sccsid[ ] = "%W%\t%G%";

char *basename(char *path){
	char *lsi = strrchr(path, '/');
	return lsi ? lsi+1 : path; 
}

char *slashname(char *path){
	return sf("%0.*s", strlen(path)-strlen(basename(path)),  path);
}

int alldigits(const char *p){
	if( !p ) return 0;
	while( isdigit(*p) ) ++p;
	return !*p;
}

char *Name(char *s, int i){
	static char buf[8][32];
	static int cycle;
	char *t = buf[(cycle++,cycle%=8)];
	sprintf( t, s, i );
	return t;
}
	
char *SysErr(char *s){
#ifndef errno
	extern int errno;
#endif
	return sf( "%s %s", s, strerror(errno) );
}

char *strcatfmt(char *s, const char* fmt, ... ){
	va_list va;
	va_start(va, fmt);
	vsprintf( &s[strlen(s)], fmt, va);
	va_end(va);
	return s;
}

int ReadOK( int fd, char *addr, int req ){
	int got;

	got = read(fd,addr,req);
	trace( "ReadOK(%d,%d,%d) %d", fd, addr, req, got );
	return got == req;
}

int WriteOK( int fd, char *addr, int req ){
	int got;

	got = write(fd,addr,req);
	trace( "WriteOK(%d,%d,%d) %d", fd, addr, req, got );
	return got == req;
}

long readn(int f, uchar *av, long n){
	char *a;
	long m, t;
	a = (char*)av;
	t = 0;
	while(t < n){
		m = read(f, a+t, n-t);
		if(m <= 0){
			if(t == 0)
				return m;
			break;
		}
		t += m;
	}
	return t;
}

long modified( int fd ){
	struct stat buf;

	fstat( fd, &buf );
	trace( "fd=%d buf.st_mtime=%d", fd, buf.st_mtime );
	return buf.st_mtime;
}

char *pathexpand(char *f, char *path, int a){
	static char file[128];
	char *p;

	if (!strncmp(f, "./", 2) || !strncmp(f, "../", 3)) {
		if (access(f, a) != -1)
			return f;
	}
	if (*f != '/' && path) {
		while(*path){
			for(p=file; *path && *path!=':';)
				*p++ = *path++;
			if(p!=file)
				*p++='/';
			if(*path)
				path++;
			(void)strcpy(p, f);
			if (access(file, a) != -1)
				return file;
		}
	}
	if (access(f, a) != -1)
		return f;
	return 0;
}

static char issep[256], isfield[256];
static int init = 0;

void setfields(char *arg){
	uchar *s = (uchar *)arg;

	memset(issep, 0, sizeof issep);
	memset(isfield, 1, sizeof isfield);
	while(*s){
		issep[*s] = 1;
		isfield[*s++] = 0;
	}
	isfield[0] = 0;
	init = 1;
}

int getfields(char *ss, char **sp, int nptrs){
	uchar *s = (uchar *)ss;
	uchar **p = (uchar **)sp;
	unsigned int c;

	if( init == 0 ) setfields(" \t");
	for(;;){
		if(*s == 0) break;
		if(--nptrs < 0) break;
		*p++ = s;
		while(isfield[c = *s++]);
		if(c == 0) break;
		s[-1] = 0;
	}
	if(nptrs > 0)
		*p = 0;
	else if(--s >= (uchar *)ss)
		*s = c;
	return(p - (uchar **)sp);
}

int getmfields(char *ss, char **sp, int nptrs){
	uchar *s = (uchar *)ss;
	uchar **p = (uchar **)sp;
	uint c;
	uchar *goo;

	if( init == 0 ) setfields(" \t");
	if(*s){
		while(nptrs-- > 0){
			*p++ = s;
			while(isfield[*s++]);
			goo = s-1;
			if((c = *goo) == 0)
				break;
			*goo = 0;
			while(issep[*s]) s++;
			if(*s == 0) break;
		}
	}
	if(nptrs > 0)	/* plenty of room */
		*p = 0;
	else if(*s)	/* no room and we found a trailing non-sep */
		*goo = c;
	return(p - (uchar **)sp);
}
