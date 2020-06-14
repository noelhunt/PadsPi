#include "univ.h"
#undef ctime
#undef gmtime
#undef localtime
#include <signal.h>

void h_sig(int);
void initio(void);
void PadUpdate(void);

Mousectl	*mousectl;
Rectangle	PadSpace;
extern int	protodebug;

Timer *Alarm;
Cursor *Jcursor = 0;
int hostfd[2];
int Pstate;
int maxtab = 8;
int quitok;
Point ZPoint;
Rectangle ZRectangle;

void threadmain(int argc, char *argv[]){
	Rectangle r;

	ZPoint = Pt(0,0);
	ZRectangle = Rect(0,0,0,0);

# ifndef BLIT
	signal(SIGALRM, h_sig);
# else
	signal(SIGHUP, h_sig);
	signal(SIGINT, h_sig);
	signal(SIGPIPE, h_sig);
	signal(SIGTERM, h_sig);
# endif
	dup(0, 3);
	dup(1, 4);
	hostfd[0] = 3;
	hostfd[1] = 4;
	close(0);
	open("/dev/null", OREAD);
	close(1);
	if(open("/dev/tty", OWRITE) < 0)
		open("/dev/null", OWRITE);

	Pstate = 0;
	quitok = 0;
#ifdef ALLOC
	allocinit();
#endif
	if(initdraw(0, nil, "padsterm") < 0){
		fprint(2, "padsterm: initdraw: %r\n");
		threadexitsall("initdraw");
	}
	draw(screen, screen->clipr, display->white, nil, ZP);
	initio();
	Alarm = timerstart(1 SECOND);
	display->locking = 1;
	PadStart(screen->clipr);
	PadClip();

	cursswitch(&Coffee);
	Configuration |= NOVICEUSER;
	Configuration |= BIGMEMORY;

	for( ; ; Pstate = Jwait() ){
		LayerReshaped();
		MOUSEServe();
		KBDServe();
		RCVServe();
		ALARMServe();
		PadUpdate();
	}
}

void h_sig(int s){ abort(); }

void PadUpdate(){
	extern int got;
	static int last = 0;
	if( (last==(1<<RHost)) && (got==0) ) Dirty((Pad*)0);
	last = got;
}
