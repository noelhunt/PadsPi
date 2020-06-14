#include "univ.h"

#define STACK	32768

int ALARM_CYCLE = 15;		/* 1/4 second for internal refresh */
int HOST_CYCLE = 4;		/*  1  second for host objects */

static Channel*	ctimer;
static Timer *timer;
enum{ FALSE, TRUE };

uint msec(void){
	return nsec()/1000000;
}

void timerstop(Timer *t){
	t->next = timer;
	timer = t;
}

void timercancel(Timer *t){
	t->cancel = TRUE;
}

static void timerproc(void *v){
	int i, nt, na, dt, del;
	Timer **t, *x;
	uint old, new;

	USED(v);
	threadsetname("timerproc");
# ifdef RFORK
	rfork(RFFDG);
# endif
	t = nil;
	na = 0;
	nt = 0;
	old = msec();
	for(;;){
		sleep(1);	/* will sleep minimum incr */
		new = msec();
		dt = new-old;
		old = new;
		if(dt < 0)	/* timer wrapped; go around, losing a tick */
			continue;
		for(i=0; i<nt; i++){
			x = t[i];
			x->dt -= dt;
			del = FALSE;
			if(x->cancel){
				timerstop(x);
				del = TRUE;
			}else if(x->dt <= 0){
				/*
				 * avoid possible deadlock if client is
				 * now sending on ctimer
				 */
				if(nbsendul(x->c, 0) > 0)
					del = TRUE;
			}
			if(del){
				memmove(&t[i], &t[i+1], (nt-i-1)*sizeof t[0]);
				--nt;
				--i;
			}
		}
		if(nt == 0){
			x = recvp(ctimer);
	gotit:
			if(nt == na){
				na += 10;
				if((t = realloc(t, na*sizeof(Timer*))) == nil)
					panic("timer realloc failed");
			}
			t[nt++] = x;
			old = msec();
		}
		if(nbrecv(ctimer, &x) > 0)
			goto gotit;
	}
}

void timerinit(void){
	ctimer = chancreate(sizeof(Timer*), 100);
	chansetname(ctimer, "ctimer");
	proccreate(timerproc, nil, STACK);
}

Timer *timerstart(int dt){
	Timer *t;

	t = timer;
	if(t)
		timer = timer->next;
	else{
		t = (Timer*) Alloc(sizeof(Timer));
		t->c = chancreate(sizeof(int), 0);
		chansetname(t->c, "tc%p", t->c);
	}
	t->next = nil;
	t->dt = dt;
	t->cancel = FALSE;
	sendp(ctimer, t);
	return t;
}
