#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;

/*
 * Garbage-compacting allocator
 *
 * called as:	gcalloc(nbytes, where)
 *		gcrealloc(p, nbytes)
 *		gcfree(p)
 *	nbytes is unsigned long
 *	where is a pointer to the location which will point to the area
 *	(e.g. base in a Bitmap), to be updated when necessary.
 *	The return values  for the allocators are the new address,
 *	but since they update *where, the value isn't too useful.
 *	They panic if they fail.
 */

/*
 * The arena is allocated in longs, to speed up compaction
 * Garbage is compacted towards the bottom (low address end) of the arena.
 * A block has a struct Ghdr as its bottom GHDRSIZE longs for use by gcfree.
 * Ghdr.pval points to the USER location where the pointer to the block
 * is stored; this cell must be updated after a compaction.
 * Deallocated blocks have Ghdr.pval odd.
 */

#define	BRKINCR		(16*1024)	/* bytes to add to arena each time */
#define LBRKINCR	(BRKINCR/sizeof(long))
#define	FREE		1
#define	GHDRSIZE	sizeof(struct Ghdr)/sizeof(long)

static long *nextlong;
static long *startarena;
static long *endarena;
static void compact();
int	dontcompact;
void	gcfree(uchar*);
void	panic(char*s);

struct Ghdr{
	union uuu{
		long	Uival;		/* integer value of where */
		long	**Upval;	/* where */
	}u;
	ulong nlongs;			/* # of longs in the user's data */
};
#define	ival	u.Uival
#define	pval	u.Upval
#define	hp	((struct Ghdr *)p)
#define DEBUG
# ifdef DEBUG
static	struct Ghdr *Dhp;
# endif

uchar *gcalloc(ulong nbytes, long **where){
	long *p, Nlongs = nbytes;
	ulong nl;
	if(nextlong>endarena)
		panic("nextlong>endarena");
	if((long)where&FREE)	/* head off a possible disaster */
		panic("where&FREE");
	if(startarena==0){
		if((int)(startarena=(long *)sbrk(BRKINCR))==-1)
			panic("sbrk");
		endarena=startarena+(BRKINCR/sizeof(long));
		nextlong=startarena;
	}
	Nlongs+=sizeof(long)-1;
	Nlongs/=sizeof(long);	/* convert bytes to longs */
	Nlongs+=GHDRSIZE;
	if(endarena-nextlong < Nlongs){
		if(!dontcompact)
			compact();
		if(endarena-nextlong < Nlongs){
			nl=(nextlong+Nlongs)-startarena;
			/* if !compacting, avoid greed */
			if(!dontcompact)
				nl=((nl+LBRKINCR-1)/LBRKINCR)*LBRKINCR;
			if(brk((char *)(startarena+nl))!=0)
				panic("brk");
			endarena=startarena+nl;
		}
	}			
	p=nextlong;
	hp->pval=where;
	hp->nlongs=Nlongs;
	nextlong+=Nlongs;
# ifdef DEBUG
	Dhp = hp;
# endif
	return (uchar *)(*(hp->pval)=p+GHDRSIZE);
}

char *gcrealloc(uchar *cp, ulong nbytes){
	long *p=(long *)cp, *q, Nlongs = nbytes;
	long *newp, **ptrold;
	int n;
	long *x;
	p-=GHDRSIZE;		/* the Ghdr */
	n=hp->nlongs;
	Nlongs+=sizeof(long)-1;
	Nlongs/=sizeof(long);	/* convert bytes to longs; nbytes is now Nlongs */
	ptrold=hp->pval;	/* location that will be updated if compaction occurs */
	/*
	 * we give where==x to gcalloc to avoid collision with old Ghdr
	 */
	newp=(long *)gcalloc((ulong)(Nlongs*sizeof(long)), &x);
	/* now it's safe to have both Ghdrs point to the same place */
	((struct Ghdr *)(newp-GHDRSIZE))->pval=ptrold;
	p= *ptrold;
	q=newp;
	if(n>Nlongs) n=Nlongs;
	if(n>0) do
		*q++= *p++;
	while(--n);
	(void)gcfree((uchar *)*ptrold);
	return (char *)(*ptrold=newp);
}

void dcopy(uchar *s1, uchar *s2, uchar *d, int dir) {
        long n=s2-s1;

        if(dir<0){
                if(n>0){
			long m=(n+7)/8;
			d+=n;
			switch((int)(n&7)){
			case 0: do{	*--d = *--s2;
			case 7:		*--d = *--s2;
			case 6:		*--d = *--s2;
			case 5:		*--d = *--s2;
			case 4:		*--d = *--s2;
			case 3:		*--d = *--s2;
			case 2:		*--d = *--s2;
			case 1:		*--d = *--s2;
				}while(--m>0);
			}
		}
	}else
		memcpy((char *)d, (char *)s1, (int)(s2-s1));
}

void shiftgcarena(ulong nl){
	long *p;
	if(startarena==0 || dontcompact)
		return;
	if(nl<0)
		panic("shiftgcarena");
	dcopy((uchar *)startarena, (uchar *)nextlong, (uchar *)(startarena+nl), -1);
	nextlong+=nl;
	startarena+=nl;
	endarena+=nl;
	for(p=startarena; p<nextlong; p+=hp->nlongs){
		if((hp->ival&FREE)==0)
			*(hp->pval)+=nl;
	}
}

void gcfree(uchar *cp){
	long *p=(long *)cp;
	if(p==0)
		return;
	p-=GHDRSIZE;
	if(p<startarena || nextlong<=p)
		panic("gcfree");
	hp->ival|=FREE;
}

static void compact(){
	long *w, *p;
	register ulong n;

	w=p=startarena;
	while(p<nextlong){
		if(hp->ival&FREE){
			p+=hp->nlongs;
			continue;
		}
		if(w==p){
			w+=hp->nlongs;
			p+=hp->nlongs;
			continue;
		}
		*(hp->pval)=w+GHDRSIZE; /* update *where */
		*w++=hp->ival;
		*w++=n=hp->nlongs;
		p+=GHDRSIZE;
		if((n-=GHDRSIZE)>0) do
			*w++ = *p++;
		while(--n);
	}
	nextlong=w;
}

void gcchk(){
	register long *p;
	if(startarena==0)
		return;
	for(p=startarena; p<nextlong; p+=hp->nlongs)
		if((hp->ival&FREE)==0){
			if(hp->pval==0
			 || ((long *)hp->pval>=startarena
			 && ((int)(hp->pval)&0x70000000L)==0))
				panic("gcchk 1");
			if(p+hp->nlongs>nextlong)
				panic("gcchk 2");
		}
}

/*
 *	Allocator. Manages arena between initial bss and garbage-compacted
 *	arena.  Uses shiftgcarena() when it needs to expand its dominion.
 */

typedef struct Ahdr Ahdr;

struct Ahdr {
	union{
		long *Unext;	/* pointer to next object */
		long Uinext;	/* integer next; odd if deallocated */
	}uu;
	long	nlongs;
};
#define	inext	uu.Uinext
#define	next	uu.Unext
//#define	hp	((struct Ahdr*)p)

#define	AHDRSIZE	(sizeof(struct Ahdr)/sizeof(long))

static long *basep;	/* beginning of arena; only set once	*/
static long *endp;	/* end of arena				*/
static long *nextp;	/* next one to be used			*/

void panic(char*s);

void allocinit(){
	basep=(long *)sbrk(0);
	if(brk((char *)(basep+LBRKINCR))!=0)
		panic("brk");
	nextp=basep;
	endp=basep+LBRKINCR;
}

void *alloc(ulong nbytes){
	long *p, *q, Nlongs = nbytes;
	ulong nl;
	Nlongs += sizeof(long)-1;
	Nlongs /= sizeof(long);	/* convert bytes to longs */
	Nlongs += AHDRSIZE;
	/* look for exact match */
	for(p = basep; p<nextp; p = (long *)(((struct Ahdr*)p)->inext&~FREE))
		if((((struct Ahdr*)p)->inext&FREE) && hp->nlongs == Nlongs){
			((struct Ahdr*)p)->inext &= ~FREE;
			goto Return;
		}
	/* try off the end */
	if(endp-nextp < Nlongs){
		nl = (nextp+Nlongs)-basep;			/* number we need */
		nl = ((nl+LBRKINCR-1)/LBRKINCR)*LBRKINCR;	/* rounded up */
		nl -= (endp-basep);				/* minus number we have */
		if((int)sbrk((int)(nl*sizeof(long))) == -1)
			panic("sbrk");
		shiftgcarena(nl);
		endp += nl;
	}
	p = nextp;
	nextp += Nlongs;
	((struct Ahdr*)p)->nlongs = Nlongs;
	((struct Ahdr*)p)->next = nextp;
    Return:
	for(q = p+AHDRSIZE, Nlongs -= AHDRSIZE; Nlongs-->0; )
		*q++ = 0;
	return (uchar *)(p+AHDRSIZE);
}

void allocfree(void *cp){
	long *p=(long *)cp;
	if(p<=basep || nextp<=p)
		panic("allocfree");
	p-=AHDRSIZE;
	((struct Ahdr*)p)->inext|=FREE;
}

void panic(char *s){
	fprintf(stderr, "malloc: fatal: %s\n", s);
	abort();
}

#ifndef SALLOC
#undef FREE
#include "talloc.c"
#endif

#define NCHUNK 27

int chunk[NCHUNK] = {
	7,    13,   17,   23,   29,   31,    32,    33,     47,
	59,   101,  107,  121,  153,  161,   164,   189,    197,
	203,  233,  333,  457,  603,  1011,  4063,  16343,  65521
};
void **pp[NCHUNK*10];

void main(int argc, char *argv[]){
	int i, j;
#ifdef SALLOC
	allocinit();
#endif
	for(i=0; i<10; i++){
		for(j=0; j<NCHUNK; j++){
#ifdef SALLOC
			pp[j+i*NCHUNK] = alloc(chunk[j]*(i+1));
#else
			pp[j+i*NCHUNK] = talloc(chunk[j]*(i+1));
#endif
		}
	}
	for(i=0; i<NCHUNK*10; i++)
#ifdef SALLOC
		allocfree( pp[i] );
#else
		tfree( pp[i] );
#endif
#ifndef SALLOC
	tstats();
#endif
}
