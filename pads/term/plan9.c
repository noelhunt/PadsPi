#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "univ.h"

#define STACK 16384

void usage(void){
	fprint(2, "usage: padsterm\n");
	threadexitsall("usage");
}

int snarfswap(char *frompads, int nc, char **topads){
	char *s;
	s = getsnarf();
	putsnarf(frompads);
	*topads = s;
	return s ? strlen(s) : 0;
}

Channel *hostc;
Readbuf	hostbuf[2];

void hostproc(void *arg){
	Channel *c;
	int i=0, n, which;

	c = arg;
	for(;;){
		i = 1-i;	/* toggle */
		n = read(hostfd[0], hostbuf[i].data, sizeof hostbuf[i].data);
		if(n <= 0){
			if(n == 0){
				if(!quitok)
					werrstr("unexpected eof");
			}
			fprint(2, "padsterm: host read error: %r\n");
			threadexitsall("host");
		}
		hostbuf[i].n = n;
		which = i;
		send(c, &which);
	}
}

void hoststart(void){
	hostc = chancreate(sizeof(int), 0);
	chansetname(hostc, "hostc");
	proccreate(hostproc, hostc, STACK);
}

void padsnarf(String *pmb) {
	int n;
	char *p;

	/* get current X selection */
	p = getsnarf();
	if ((n = strlen(p)) == 0)
		return;
	if (n > pmb->size) {
		if (!pmb->size)
			pmb->s = Alloc(n);
		else
			pmb->s = realloc((uchar *)pmb->s, n);
		pmb->size = n;
	}
	pmb->n = n;
        strncpy(pmb->s, p, n);
}

#define	UP	0
#define	DOWN	1

/*
 * buttons(UP)	 - waits until ALL buttons are up
 * buttons(DOWN) - waits until at least ONE button is down
 */

void buttons(int updown){
	while(((mouse->buttons&7)!=0) != updown)
		waitMOUSE();
}
