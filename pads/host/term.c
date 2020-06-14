#include <ctype.h>
#include <pads.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <new.h>
SRCFILE("term.c")

ItemCache  *ICache;
CarteCache *CCache;

static void mallocerr(){
	PadsError("Pads library: malloc failed");
	abort();
}

void Pick( const char *s, Action a, long o ){
	Index ix;

	trace("Pick(%d,%d)", a, o);
	ix = ICache->place(Item(s, a, o));
	R->pktstart( P_PICK );
	R->sendshort( ix.sht() );
	R->pktend();
}

const char *padsterm = "padsterm";

const char *loadterm(int argc, char **argv, const char *cmd){
	int targc = 0;
	char *targv[20];
	int ph2t[2], pt2h[2];
	char err[128];
	char *StrDup(const char*);
	targv[targc++] = StrDup(cmd);
	while( argc > 1 ){
		targv[targc++] = argv[argc];
               --argc, argv++;
        }
	targv[targc] = 0;
	if(pipe(ph2t)==-1 || pipe(pt2h)==-1){
		sprintf(err, "loadterm: pipe: %s", strerror(errno));
		return strdup(err);
	}
	switch(fork()){
	case 0:
		dup2(ph2t[0], 0);
		dup2(pt2h[1], 1);
		close(ph2t[0]);
		close(ph2t[1]);
		close(pt2h[0]);
		close(pt2h[1]);
		execvp(padsterm, targv);
		fprintf(stderr, "can't exec: ");
		perror(padsterm);
		exit(127);
	case -1:
		sprintf(err, "can't fork padsterm: %s", strerror(errno));
		return strdup(err);
	}
	dup2(pt2h[0], 0);
	dup2(ph2t[1], 1);
	close(ph2t[0]);
	close(ph2t[1]);
	close(pt2h[0]);
	close(pt2h[1]);

	return 0;
}

const char *PadsInit(int argc, char **argv){
	const char *c = loadterm(argc, argv, padsterm);
	if (c)
		return c;
	std::set_new_handler(mallocerr);
	R = new Remote(0, 1);
	trace(">> PadsInit()");
	R->pktstart(P_VERSION); R->sendlong(PADS_VERSION); R->pktend();
	R->pktstart(P_BUSY); R->pktend();
	ICache = new ItemCache;
	CCache = new CarteCache;
	trace("<< PadsInit()");

	return 0;
}

const char *PadsTermInit(int argc, char **argv, char *machine){
	return "not implemented.";
}

void PadsRemInit(){
	R = new Remote(0);
	set_new_handler(mallocerr);
	R->pktstart(P_VERSION); R->sendlong(PADS_VERSION); R->pktend();
	R->pktstart(P_BUSY); R->pktend();
	ICache = new ItemCache;
	CCache = new CarteCache;
}

char *TapTo;
void WireTap(PRINTF_ARGS){
	static int fd = -1;
	static long t0;
	struct stat s;
	long t;
	va_list ap;
	va_start(ap, fmt);

	if( !TapTo ) return;
	t = time(0);
	if( ::stat("/usr/tmp/.logpads", &s) || ctime(&t)[23]!='6' )
		goto BailOut;
	if( t0 )
		t -= t0;
	else
		t0 = t;
	char buf[256];
	sprintf(buf, "%x:", t);
	vsprintf(buf+strlen(buf), fmt, ap);
	va_end(ap);
	if( fd < 0 ){
		if( ::stat(TapTo, &s) )
			creat(TapTo, 0777); 
		fd = open(TapTo, 1);
	}
#define PILOGSIZE 32000
	if( fd<0
	 || fstat(fd, &s)
	 || s.st_size > PILOGSIZE )
			goto BailOut;
	lseek(fd, s.st_size, 0);
	write(fd, buf, strlen(buf));
	return;
BailOut:
	TapTo = 0;
}

int BothValid(PadRcv *p, PadRcv *o){
	return p && o;
}

void TermAction(PadRcv *parent, PadRcv *obj, int pick){
	Item *item;
	Index ix((int)R->rcvlong());

	trace( "TermAction(%d,%d,%d)", parent, obj, pick );
	if( ix.null() ) return;
	item = ICache->take(ix);
	if( !BothValid(parent,obj)
	 || (pick && !obj->accept(item->action)) )
		return;
	if( item->action ) (obj->*item->action)(item->opand, 0, 0);
}

const char *DoKbd(PadRcv *obj, char *buf){
//	WireTap("%x->%x(%x) %s\n", obj, &obj->kbd, strlen(buf), buf);
	const char *e = obj->kbd(buf);
	if( e ) PadsWarn("%s", e);
	return e;
}

void Shell(){
	char cmd[256];
	R->rcvstring(cmd);
	FILE *fp = Popen(cmd, "w");
	for( long lines = R->rcvlong(); lines>0; --lines ){
		char data[256];
		if( fp ) fprintf(fp, "%s\n", R->rcvstring(data));
	}
	if( !fp ){
		PadsWarn("cannot write to pipe");
		return;
	}
	int x = Pclose(fp);
	if( x ) PadsWarn( "exit(%d): %s", x, cmd );
}

void ShKbd(PadRcv *obj, char *cmd){
	trace( "ShKbd(%d,%s)", obj, cmd );
	FILE *fp = Popen(cmd, "r");
	if( !fp ){
		PadsWarn("cannot read from pipe");
		return;
	}
	char buf[256];
	while( fgets(buf, sizeof buf, fp) ){
		buf[strlen(buf)-1] = 0;
		if( DoKbd(obj, buf) ) break;
	}
	int x = Pclose(fp);
	if( x ) PadsWarn( "exit(%d): %s", x, cmd );
}

void Kbd(PadRcv *parent, PadRcv *obj){
	char buf[256];
	R->rcvstring(buf);
	trace( "Kbd %d %s", obj, buf );
	if( !BothValid(parent,obj) ) return;
	if( !strcmp( buf, "?" ) ){
		const char *h = obj->help();
		PadsWarn( "%s", (h && *h) ? h : "error: null help string" );
	} else if( buf[0] == '<' ){
		ShKbd(obj, buf+1);
	} else
		DoKbd(obj, buf);
}

void TermServe(){
	Protocol p;
	long n, to, pick = 0;

	R->pktstart(P_IDLE); R->pktflush();
	p = (Protocol) R->get();
	if( p == P_PICK ) {
		pick = 1;
		p = (Protocol) R->get();
	}
	PadRcv *par = R->rcvobj();
	PadRcv *obj = R->rcvobj();
	if( p != P_CYCLE ){ R->pktstart(P_BUSY); R->pktflush(); }
	trace("TermServe() p.%X", p&0xFF);
	switch( (int) p ){
		case P_ACTION:
			TermAction(par, obj, (int)pick);
			break;
		case P_KBDSTR:
			Kbd(par, obj);
			break;
		case P_SHELL:
			Shell();
			break;
		case P_NUMERIC:
		case P_CYCLE:
		case P_USERCLOSE:
		case P_USERCUT:
			n = R->rcvlong();
			if( !BothValid(par,obj) ) return;
			switch( (int) p ){
			case P_NUMERIC:
				obj->numeric(n);	break;
			case P_CYCLE:
				obj->cycle();		break;
			case P_USERCLOSE:
				obj->userclose();	break;
			case P_USERCUT	:
				obj->usercut();		break;
			default: R->err();
			}
			break;
		case P_LINEREQ:
			n = R->rcvlong();
			to = R->rcvlong();
			trace("P_LINEREQ %d %d %d", p, n, to);
			if( !BothValid(par,obj) ) return;
			while( n <= to )
				obj->linereq((long) n++, 0);
			break;
		case P_QUIT:
			exit(0);
			break;
		default:
			R->err();
	}
}

void PadsServe(long n){
	trace("PadsServe( n.%d )", n);
	if( n ){
		while( n-->0 ) TermServe();
	} else {
		for( ;; )  TermServe();
	}
}

void PadsWarn(const char *fmt, ...){
	va_list av;
	char t[256];
	va_start(av, fmt);
	vsprintf(t, fmt, av);
	va_end(av);
	R->pktstart( P_HELPSTR );
	R->sendstring( t );
	R->pktend();
}

void PadsError(const char *fmt, ...){
	va_list av;
	char t[256];
	va_start(av, fmt);
	vsprintf(t, fmt, av);
	va_end(av);
	R->pktstart( P_ERROR );
	R->sendstring( t );
	R->pktflush();
	exit(1);
}

void PadsQuit(){
	R->pktstart( P_QUIT );
	R->pktflush();
	exit(0);
}
