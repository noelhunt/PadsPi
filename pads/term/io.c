#include "univ.h"

int	protodebug = 0;
int	cursorfd;
int	alarmfd = -1;
int	input;
int	got;
int	block;
uint	kbdc;
int	resized;
uchar	*hostp;
uchar	*hoststop;
uchar	*alarmbase;
uchar	*alarmp;
uchar	*alarmstop;
Channel	*alarmc;
Channel	*hostc;
Mouse	*mouse;
Mousectl *mousectl;
Keyboardctl *kbdctl;

void Cycle(void);
void hoststart(void);
void timerinit(void);
void timerstop(Timer*);

void initio(void){
	threadsetname("main");
	if(protodebug) print("mouse\n");
	mousectl = initmouse(nil, display->image);
	if(mousectl == nil){
		fprint(2, "pads: mouse init failed: %r\n");
		threadexitsall("mouse");
	}
	mouse = &mousectl->m;
	if(protodebug) print("kbd\n");
	kbdctl = initkeyboard(nil);
	if(kbdctl == nil){
		fprint(2, "pads: keyboard init failed: %r\n");
		threadexitsall("kbd");
	}
	if(protodebug) print("hoststart\n");
	hoststart();
	if(protodebug) print("timerinit\n");
	timerinit();
	if(protodebug) print("initio done\n");
}

void waitMOUSE(void){
	if(readmouse(mousectl) < 0)
		panic("mouse");
}

void mouseunblock(void){
	got &= ~(1<<RMouse);
}

void kbdblock(void){		/* ca suffit */
	block = (1<<RKeyboard)|(1<<RAlarm);
}

# ifdef BUTTONPROC
int button(int but){
	waitMOUSE();
	return mouse->buttons&(1<<(but-1));
}
# endif

int Jwait(void) {
	Alt alts[NRes+1];
	int i;
	Rune r;
	ulong type;
	char s[UTFmax+1];

again:
	alts[RAlarm].c = Alarm->c;
	alts[RAlarm].v = nil;
	alts[RAlarm].op = CHANRCV;
	if(block & (1<<RAlarm))
		alts[RAlarm].op = CHANNOP;

	alts[RHost].c = hostc;
	alts[RHost].v = &i;
	alts[RHost].op = CHANRCV;
	if(block & (1<<RHost))
		alts[RHost].op = CHANNOP;

	alts[RKeyboard].c = kbdctl->c;
	alts[RKeyboard].v = &r;
	alts[RKeyboard].op = CHANRCV;
	if(block & (1<<RKeyboard))
		alts[RKeyboard].op = CHANNOP;

	alts[RMouse].c = mousectl->c;
	alts[RMouse].v = &mousectl->m;
	alts[RMouse].op = CHANRCV;
	if(block & (1<<RMouse))
		alts[RMouse].op = CHANNOP;

	alts[RResize].c = mousectl->resizec;
	alts[RResize].v = nil;
	alts[RResize].op = CHANRCV;
	if(block & (1<<RResize))
		alts[RResize].op = CHANNOP;

	if(protodebug) print("waitforio %c%c%c%c%c\n",
		"h-"[alts[RHost].op == CHANNOP],
		"k-"[alts[RKeyboard].op == CHANNOP],
		"m-"[alts[RMouse].op == CHANNOP],
		"a-"[alts[RAlarm].op == CHANNOP],
		"R-"[alts[RResize].op == CHANNOP]);

	alts[NRes].op = CHANEND;

	if(got & ~block) return got & ~block;

	flushimage(display, 1);
	type = alt(alts);
	switch(type){
	case RHost:
		hostp = hostbuf[i].data;
		hoststop = hostbuf[i].data + hostbuf[i].n;
		block = 0;
		break;
	case RAlarm:
		timerstop(Alarm);
		break;
	case RKeyboard:
		kbdc = r;
		(void) runetochar(s, &r);
		break;
	case RMouse:
		break;
	case RResize:
		resized = 1;
		/* do the resize in line if we're not in a blocking state */
		if( block==0 )
			LayerReshaped();
		goto again;
	}
	got |= 1<<type;
	return got; 
}

int kpeekc = -1;
int ecankbd(void){
	Rune r;

	if(kpeekc >= 0)
		return 1;
	if(nbrecv(kbdctl->c, &r) > 0){
		kpeekc = r;
		return 1;
	}
	return 0;
}

int ekbd(void){
	int c;
	Rune r;
	char s[UTFmax+1];

	if(kpeekc >= 0){
		c = kpeekc;
		kpeekc = -1;
		(void) runetochar(s, &r);
		return s[0];
	}
	if(recv(kbdctl->c, &r) < 0){
		fprint(2, "pads: keybard recv error: %r\n");
		panic("kbd");
	}
	(void) runetochar(s, &r);
	return s[0];
}

int kbdchar(void){
	int c;
	char s[UTFmax+1];
	if(got & (1<<RKeyboard)){
		(void) runetochar(s, &kbdc);
		c = s[0];
		kbdc = -1;
		got &= ~(1<<RKeyboard);
		return s[0];
	}
	if(!ecankbd())
		return -1;
	return ekbd();
}

int qpeekc(void){ return kbdc; }

int rcvchar(void){
        int c;

        if(!(got & (1<<RHost)))
                return -1;
        c = *hostp++;
        if(hostp == hoststop)
                got &= ~(1<<RHost);
        return c;
}

void ALARMServe() {
	if( !(Pstate & (1<<RAlarm)) ) return;
	Cycle();
	Alarm = timerstart(1 SECOND);
	got &= ~(1<<RAlarm);
}
