#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef unsigned char uchar;
typedef unsigned long ulong;

/*
 * Garbage-compacting allocator
 *
 * called as:	gcalloc(nbytes, where)
 *		gcrealloc(p, nbytes)
 *		gcfree(p)
 *	nbytes is unsigned long
 *
 * where is a pointer to the location which will point to
 * the area (e.g. base in a Bitmap), to be  updated  when
 * necessary.   The return values  for the allocators are
 * the new address, but since  they  update  *where,  the
 * value isn't too useful.
 * 
 * They panic if they fail.
 */

/*
 * The  arena  is allocated in longs, to speed up compaction. Garbage
 * is compacted towards the bottom (low address end) of the arena.  A
 * block  has a struct hdr as its top HEADERSIZE longs, and a pointer
 * to the struct header as  its  bottom  long,  for  use  by  gcfree.
 * hdr.pval  points  to  the  USER  location where the pointer to the
 * block is stored; this cell must be  updated  after  a  compaction.
 * Deallocated blocks have hdr.pval odd.
 */

#define	BRKINCR		(16*1024)	/* bytes to add to arena each time */
#define LBRKINCR	(BRKINCR/sizeof(long))
#define	FREE		1
#define	HDRSIZE		sizeof(struct hdr)/sizeof(long)

static long *nextlong;
static long *startarena;
static long *endarena;
static void compact();
int	dontcompact;
void	gcfree(void*);
void	panic(char*s);

struct hdr{
	union uuu{
		long	Uival;		/* integer value of where */
		long	**Upval;	/* where */
	}u;
	ulong nlongs;			/* # of longs in the user's data */
};
#define	ival	u.Uival
#define	pval	u.Upval
#define	hp	((struct hdr*)p)
#define DEBUG
# ifdef DEBUG
static	struct hdr *Dhp;
# endif

#define MALLOC

#ifndef MALLOC
uchar *gcalloc(ulong nbytes, long **where){
#else
uchar *gcmalloc(ulong nbytes){
#endif
	long *p, Nlongs = nbytes;
	ulong nl;
#ifdef MALLOC
	long *x = 0;
	long **where = &x;
#endif
	if(nextlong>endarena)
		panic("nextlong>endarena");
#ifndef MALLOC
	if((long)where&FREE)	/* head off a possible disaster */
		panic("where&FREE");
#endif
	if(startarena==0){
		if((int)(startarena=(long *)sbrk(BRKINCR))==-1)
			panic("sbrk");
		endarena=startarena+(BRKINCR/sizeof(long));
		nextlong=startarena;
	}
	Nlongs+=sizeof(long)-1;
	Nlongs/=sizeof(long);	/* convert bytes to longs */
	Nlongs+=HDRSIZE;
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
	return (uchar *)(*(hp->pval)=p+HDRSIZE);
}

char *gcrealloc(uchar *cp, ulong nbytes){
	long *p=(long *)cp, *q, Nlongs = nbytes;
	long *newp, **ptrold;
	int n;
	long *x;
	p-=HDRSIZE;		/* the hdr */
	n=hp->nlongs;
	Nlongs+=sizeof(long)-1;
	Nlongs/=sizeof(long);	/* convert bytes to longs; nbytes is now Nlongs */
	ptrold=hp->pval;	/* location that will be updated if compaction occurs */
	/*
	 * we give where==x to gcalloc to avoid collision with old hdr
	 */
#ifndef MALLOC
	newp=(long *)gcalloc((ulong)(Nlongs*sizeof(long)), &x);
#else
	newp=(long *)gcmalloc((ulong)(Nlongs*sizeof(long)));
#endif
	/* now it's safe to have both hdrs point to the same place */
	((struct hdr *)(newp-HDRSIZE))->pval=ptrold;
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

void gcfree(void *cp){
	long *p=(long *)cp;
	if(p==0)
		return;
	p-=HDRSIZE;
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
		*(hp->pval)=w+HDRSIZE; /* update *where */
		*w++=hp->ival;
		*w++=n=hp->nlongs;
		p+=HDRSIZE;
		if((n-=HDRSIZE)>0) do
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

void panic(char *s){
	fprintf(stderr, "malloc: %s\n", s);
	exit(1);
}

#define NCHUNK 27

int chunk[NCHUNK] = {
	7,    13,   17,   23,   29,   31,    32,    33,     47,
	59,   101,  107,  121,  153,  161,   164,   189,    197,
	203,  233,  333,  457,  603,  1011,  4063,  16343,  65521
};
long *pp[NCHUNK*10];

void main(int argc, char *argv[]){
	int i, j;
	uchar *cp;
	for(i=0; i<10; i++){
		for(j=0; j<NCHUNK; j++){
#ifndef MALLOC
			cp = gcalloc(chunk[j]*(i+1), &pp[j+i*NCHUNK]);
			printf("cp.0x%08x pp[%d].0x%08x\n", cp, j+i*NCHUNK, pp[j+i*NCHUNK]);
#else
			pp[j+i*NCHUNK] = (long *) gcmalloc(chunk[j]*(i+1));
			printf("pp[%d].0x%08x\n", j+i*NCHUNK, pp[j+i*NCHUNK]);
#endif
			if( j%3 ) gcchk();
		}
	}
	for(i=0; i<NCHUNK*10; i++)
		gcfree( pp[i] );
}
