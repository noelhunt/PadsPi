#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pads.h>
SRCFILE("misc.cc")

#define TBLS 250
class Bls {
	char	*p;
public:
	char	text[TBLS+1];
	int	clear() 	{ *(p=text) = 0; return 1;}	// cfront bug
		Bls()		{ clear(); }
		Bls(const char* ...);
		Bls(Bls&);
	char	*af(const char* ...);
	void	vf(const char*, va_list);
};

char *Bls::af(const char *fmt, ...){
	va_list v;
	trace( "%d.af(%s ...) %s", this, fmt, text );
	IF_LIVE(!this) return (char*)"Bls::af";
	va_start(v, fmt);
	vf(fmt, v);
	va_end(v);
	return text;
}

void Bls::vf(const char *fmt, va_list v){
	char x[1024], *q = x;

	trace( "%d.vf(%s ...) %s", this, fmt, text );
	if( p<text+TBLS && fmt ){
		vsprintf(x, fmt, v);
		while( *q && p<text+TBLS ) *p++ = *q++;
		*p = 0;
	}
}

Bls::Bls(const char *fmt, ...){
	va_list v;
	clear();
	va_start(v, fmt);
	vf(fmt, v);
	va_end(v);
}

Bls::Bls(Bls &b){
	clear();
	af("%s", b.text);
}

class Creator : public PadRcv {
	void	linereq(long,Attrib=0);
public:
	Pad	*pad;
		Creator();
	char	*kbd(char*);
};

class Journal : private PadRcv {
	int	length;
	long	key;
	long	lastreq;
	Bls	*bls;
	int	*ct;	
	char	*proc;
	Pad	*pad;
	char	buf[1024];
	void	banner();
	void	linereq(long,Attrib=0);
public:
	void	open();
	void	hostclose();
		Journal(char*);
	void	insert(const char* ...);
};

Journal::Journal(char *p){
	proc = p;
	key = 0;
	length = 50;
	lastreq = 0;
}

void Journal::open(){
	if( !pad ){
		pad = new Pad((PadRcv*) this);
		banner();
	}
	pad->makecurrent();
}

void Journal::hostclose(){
	if( pad ){
		delete pad;
		pad = 0;
	}
	if( bls ){
		delete bls;
		delete ct;
		bls = 0;
		ct = 0;
	}
	invalidate();
}

void Journal::banner(){
	if( pad ){
		pad->banner("Journal: %s", proc);
		pad->name("Journal %s", proc);
		pad->tabs(2);
		pad->options(TRUNCATE|NO_TILDE);
		if( !bls ){
			bls = new Bls[length];
			ct = new int[length];
		}
	}
}

void Journal::insert(PRINTF_ARGS){
	if( !this ) return;
	if( !this->pad || !bls ) return;			// can that be?
	va_list va;
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);
	Bls t(buf);
	if( !t.text[0] ) return;
	long i = key%length;
	if( key==0 || strcmp(bls[i].text, t.text) ){
		i = (++key)%length;
		ct[i] = 0;
		bls[i].clear();
		bls[i].af("%s", t.text);
	}
	if( lastreq == key )
		linereq(key);
	else
		pad->createline(key);
	++ct[i];
	if( key-length > 0 ) pad->removeline(key-length);
}

void Journal::linereq(long i, Attrib a){
	if( i <= key-length || !bls ) return;			// too late!
	int mod = i%length;
	if( ct[mod] > 1 )
		pad->insert(i, a, "%s (%d)", bls[mod].text, ct[mod]);
	else
		pad->insert(i, a, "%s", bls[mod].text);
	lastreq = i;
}


class Test : public PadRcv {
	void	linereq(long,Attrib=0);
public:
	Pad	*pad;
		Test();
	void	err(long);
	void	lines(long);
	void	exit();
	void	cycle();
	void	tabs(long t)	{ pad->tabs(t); }
	void	remove(long k)  { pad->removeline(k); }
	char	*kbd(char*);
	char	*help();
	void	usercut();
};

void Test::usercut() {}

int sscanf(const char*, const char* ...);

char *Test::kbd(char *s){
	int i;
	trace( "%d.kbd(%s)", this, s );
	if( !sscanf(s,"%d",&i) || i<=0 || i>=1000 )
		return (char *)"out of range";
	linereq(i,SELECTLINE);
	return 0;
}

char *Creator::kbd(char *s){
	int lo, hi;
	trace( "%d.kbd(%s)", this, s );
	if( 2 == sscanf(s, "%d %d", &lo, &hi) )
		pad->createline(lo, hi );
	return 0;
}

char *Test::help(){
	static int i;
	switch( i++%3 ){
	case 0:		return 0;
	case 1:		return (char *)"";
	case 2:		return (char *)"Test::help";
	default:	return (char *)"Test::help(default)";
	}
}

void Test::exit() { trace("%d.exit", this); PadsQuit(); }

void Test::cycle(){
	static int i;
	time_t t;

	pad->alarm(i++/10);
	time(&t);
	pad->insert( 1, "%s", ctime(&t) );
}

Test::Test(){
	Menu m( (char *)"first 1", (Action) &Test::linereq, 1 );
	Menu sub;
	Menu subsub;
	pad = new Pad( (PadRcv*) this );
	pad->lines(1000);
	pad->insert(1001, "one thousand and one");
	pad->banner( "%s=%s", "Banner", "Test" );
	pad->name( "%s=%s", "name", "test" );
	pad->options(USERCLOSE|ACCEPT_KBD|DONT_CLOSE|NO_TILDE);
	subsub.sort( "B", (Action) &Test::linereq, 101 );
	subsub.sort( "A", (Action) &Test::linereq, 102 );
	subsub.sort( "D", (Action) &Test::linereq, 103 );
	subsub.sort( "C", (Action) &Test::linereq, 104 );
	sub.first( subsub.index((char *)"subsub") );
	sub.last( "b", (Action) &Test::linereq, 110 );
	sub.last( "a", (Action) &Test::linereq, 120 );
	sub.last( "d", (Action) &Test::linereq, 130 );
	sub.last( "c", (Action) &Test::linereq, 140 );
	m.first( sub.index((char *)"sub") );
	m.first( "cycle", (Action) &Test::cycle );
	m.first( "tabs=1", (Action) &Test::tabs, 1 );
	m.first( "tabs=3", (Action) &Test::tabs, 3 );
	m.last( "tabs=5", (Action) &Test::tabs, 5 );
	m.last( "tabs=7", (Action) &Test::tabs, 7 );
	m.last( "tabs=0", (Action) &Test::tabs, 0 );
	m.last( "tabs=128", (Action) &Test::tabs, 128 );
	m.last( "remove 100", (Action) &Test::remove, 100 );
	m.last( NumericRange(1,10) );
	pad->menu(m.index());
	pad->alarm();
# ifdef INIT
	pad->makecurrent();
# endif
}

Creator::Creator(){
	pad = new Pad( (PadRcv*) this );
	pad->banner( "%s=%s", "Banner", "Creator" );
	pad->name( "%s=%s", "name", "creator" );
	pad->options(ACCEPT_KBD);
	for( long i = 1; i <= 100; i += 10 ){
		pad->createline(i);
		pad->createline(i+3, i+7);
	}
# ifdef INIT
	pad->makecurrent();
# endif
}

void Creator::linereq(long i, Attrib a){
	trace( "%d.linereq(%d,%d)", i, a );
	pad->insert(i, a, "line\t%d\t[]", i);
}

void Test::linereq(long i, Attrib a){
	Menu m;
	switch( i%5 ){
	case 0:
		pad->insert( i, a, "line\t%d\t[]", i  );
		pad->banner( "banner=%d", i );
		break;
	case 1:
		m.last( "150", (Action)&Test::linereq, 150 );
		m.last( "250", (Action)&Test::linereq, 250 );
		pad->insert( i, a|USERCUT, (PadRcv*) this, m, "line\t%d", i );
		pad->name( "name=%d", i );
		break;
	case 2:
		m.last( "250", (Action)&Test::linereq, 250 );
		m.last( "350", (Action)&Test::linereq, 350 );
		m.last( "450", (Action)&Test::linereq, 450 );
		pad->insert( i, a|USERCUT, (PadRcv*) this, m, "line\t%d", i );
		break;
	case 3:
		m.last("quit?", (Action)&Test::exit, 0 );
		pad->insert( i, a|USERCUT, (PadRcv*) this, m, "line\t%d", i );
		break;
	case 4:
		pad->insert( i, a|USERCUT, "" );
		break;
	}
	
}
enum UDisc {
	U_ERROR		= 0,

	U_FUNC		= 1,
	U_GLB		= 2,
	U_STA		= 3,
	U_STMT		= 4,
	U_UTYPE		= 5,
#                               define TOSYM 0x7        /* 2^n-1 */
	U_ARG		= 10,
	U_AUT		= 11,
	U_BLOCK		= 12,
	U_FST		= 13,
	U_MOT		= 14,
	U_SOURCE	= 15,

	U_ASM		= 20,
	U_AUDIT		= 21,
	U_BLKVARS	= 22,
	U_BPTS		= 23,
	U_CELL		= 24,
	U_CORE		= 25,
	U_DTYPE		= 26,
	U_EXPR		= 27,
	U_FRAME		= 27,
	U_MASTER	= 28,
	U_GLOBALS	= 29,
	U_INSTR		= 30,
	U_MEMORY	= 32,
	U_PHRASE	= 33,
	U_PROCESS	= 34,
	U_REG		= 35,
	U_REGCELL	= 36,
	U_REGFILE	= 37,
	U_SRCFUNCS	= 38,
	U_SRCTEXT	= 39,
	U_STABSRCS	= 40,
	U_SYMTAB	= 41,
	U_TRAP		= 42,
	U_TYPMEMS	= 43,
	U_WD		= 44,
        U_SIGMASK       = 45,
        U_HELP          = 46,
        U_SRCDIR        = 47,
        U_CONTEXT       = 48,
        U_HOSTMASTER    = 49,
        U_TERMMASTER    = 49,
        U_SIGBIT        = 50
};

#define PUBLIC(c,d)\
	int	disc()	{ return d; }\
public:\
	int	ok()	{ return this && disc() == d; }

class Wd : public PadRcv {
	long	key;
	Pad	*pad;
const	char	*awd;
	Index	carte();
	void	pwd(Attrib=0);
PUBLIC(Wd,U_WD)
		Wd();
	char	*kbd(char *);
	char	*help();
};

void NewWd() { new Wd; }

Wd::Wd(){
	pad = new Pad((PadRcv*) this);
	pad->name("pwd/cd");
	pad->banner("Pwd:");
	pad->options(ACCEPT_KBD);
	pwd();
}

Index Wd::carte(){
	Menu m;
	struct dirent *dp;
	struct stat stbuf;

	DIR *dirp = ::opendir(".");
	if( dirp ) {
		while( (dp = readdir( dirp )) ){
			int len = strlen(dp->d_name);
# ifdef TAC
			if( len>=3 && dp->d_name[len-2]=='.' ) continue;
# endif
			if( ::stat(dp->d_name, &stbuf)== -1
			 || (stbuf.st_mode&S_IFMT)!=S_IFDIR )
                                continue;
			long opand = (long) sf("%s/%s", awd, dp->d_name);
			m.sort(sf("%s\240", dp->d_name), (Action)&Wd::kbd, opand);
		}
		::closedir(dirp);
	}
	return m.index();
}

const char *Getwd(){
	char name[MAXPATHLEN+1];

	if( !getcwd(name, MAXPATHLEN) ) return (char *)"can't get working directory";
	if( chdir(name) < 0 ) return strerror(errno);
	return sf("%s", name);
}

char *Wd::help(){
	return (char *)"[cd] <path> {change working directory}";
}

char *Wd::kbd(char *s){
	if( s[0]=='c' && s[1]=='d' && s[2]==' ' ) s += 3;
	while( *s == ' ' ) ++s;
	if( chdir(s) == -1 ) pad->insert(++key, "cannot cd %s", s);
	pwd(SELECTLINE);
	return 0;
}

void Wd::pwd(Attrib a){
	static Index zix;
	awd = Getwd();
	Index ix = strncmp(awd, "/n/", 3) ? carte() : zix;
	pad->menu(ix);
	pad->insert(++key, a, (PadRcv*)this, ix, "%s", awd);
}

#include <ctype.h>
#include <stdlib.h>

int alldigits(const char *p){
	if( !p ) return 0;
	while( isdigit(*p) ) ++p;
	return !*p;
}
	
const char *SysErr(const char *s){
	return sf( "%s %s", s, strerror(errno) );
}

int StrCmp(const void *s1, const void *s2){
	char *end;
	long i, j;

	i = strtol((const char *)s1, &end, 10);
	if(end==(char *)s1) SysErr("ps output: no pid");
	j = strtol((const char *)s2, &end, 10);
	if(end==(char *)s2) SysErr("ps output: no pid");
	if(i==j) return 0;
	return ((i<j)? 1: -1);
}

class HostMaster : public PadRcv {
	long	key;
	Pad	*pad;
	Journal	*_journal;
const	char	*dopscmd(char *);
	void	makeproc(const char*, const char* =0, const char* =0);
	char	*kbd(char*);
	char	*help();
	void	refresh(char*);
	void	exit();
public:
		HostMaster();
	void	openjournal();
	Journal	*journal(){ return _journal; }
};

void NewHostMaster() { new HostMaster; }

HostMaster::HostMaster(){
	static const char *ps[] = {
		"/bin/ps  ",
		"/bin/ps a",
		"/bin/ps x",
		"/bin/ps auxgww",
		0
	};
	Menu m;
	int i;

	pad = new Pad( (PadRcv*) this );		/* this code cannot */
	pad->options(ACCEPT_KBD|TRUNCATE|SORTED);	/* be in base ctor */
	pad->name( "pi" );
	pad->banner( "pi = 3.141592653" ); // 3.141592653 589793
	for( i = 0; ps[i]; ++i )
		m.last( ps[i], (Action)&HostMaster::refresh, (long)ps[i] );
	m.last("Journal", (Action)&HostMaster::openjournal);
	m.last("quit?", (Action)&HostMaster::exit, 0);
	pad->menu(m);
	refresh(0);
	pad->makecurrent();
}

void HostMaster::openjournal(){
	if( !_journal)
		_journal = new Journal("/proc/17398");
	journal()->open();
}

void HostMaster::makeproc(const char *s, const char *t, const char *u){
	char c = *s;
}

void HostMaster::exit() { PadsQuit(); }

#define PSOUT 256
#define PROCS 256
const char *HostMaster::dopscmd(char *ps){
	char psout[PROCS][PSOUT];
	FILE *f;
	int Pclose(FILE *);
	int pid, i, j, e;

	if( !ps ) return 0;
	if( !(f = Popen( ps, "r")) )
		return SysErr( "cannot read from popen(): " );
	for( i = 0; i<PROCS && fgets(psout[i],PSOUT,f); ++i ) {}
	if( e = Pclose(f) )
		return SysErr( ps );
	qsort( &psout[0][0], i, PSOUT, StrCmp );
	for( j = 0; j <= i; ++j )
#ifdef notdef
		if( 2 == sscanf( psout[j], " %d %[^\n]", &pid, psout[0] ) )
			pad->insert( ++key, "%d %s", pid, psout[0] );
#endif
			pad->insert( ++key, "%s", psout[j] );
	return 0;
}

void HostMaster::refresh(char *ps){
	const char *error;

	pad->clear();
	if( error = dopscmd(ps) ) pad->error(error);
}

void StartAudit() {
	// new Audit; NewPadStats();
}

char buf[128];
char *pihelp = (char*) "[*] <corepath | pid> <tables> {[open] process/dump} | !<cmd> {program}";

char *HostMaster::kbd(char *s){
	char *corep;
	char core[64], syms[64], star = 0;
	void StartAudit();

	journal()->insert("kbd: %s", s);
	while( *s == ' ' ) ++s;
	if( !strcmp(s, "new Audit") ){
		StartAudit();
		return 0;
	}
	while( *s==' ' ) ++s;
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
		case 2:	if( alldigits(corep) )
				corep = sf( "%s", core );
			break;
		default:
			return help();
		}
	}
	return 0;
}

char *HostMaster::help(){
	return pihelp;
}

void PadsRemInit();

int main(int argc, char **argv){
	if (argc == 2 && !strcmp(argv[1],"-R"))
		PadsRemInit();
	else
		PadsInit( argc, argv );
	extern char *TapTo;
	TapTo = (char *)".tapto";
	new Test;
	new Creator;
	NewWd();
	NewPadStats();
	NewHelp();
	NewHostMaster();
	PadsServe();
}
