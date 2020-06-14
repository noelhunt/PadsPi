#include <stdio.h>
#include <stdlib.h>

#define log(e, u, d)	event[e][u] += (long) d;

#define A_LARGE 256
#define A_USER  0x55000000

#define NOTOOBIG 16383	/* was  32767 */

#define POOL	0
#define ALLOC	1
#define FREE	2
#define NREVENT	3

union M {
	long size;
	union M *link;
};

union M *freelist[A_LARGE];
long	 req[A_LARGE];
long	 event[NREVENT][A_LARGE];

void	panic(char*s);

void *talloc(long u){ 
	union M *m;
	register int r;

	u = ((u-1)/sizeof(union M) + 2);

	if( u >= A_LARGE ){	
		log(ALLOC, 0, (long) 1);
		m = (union M *) malloc( u * sizeof(union M) );
		if (m == NULL)
			panic("talloc: malloc fault");
	} else {	
		if( freelist[u] == NULL ){	
			r = req[u] += req[u] ? req[u] : 1;
			if (r > NOTOOBIG)
				r = req[u] = NOTOOBIG+1;
			log(POOL, (int) u, (long) r);
			freelist[u] = (union M *) malloc( r*u*sizeof(union M) );
			if (freelist[u] == NULL)
				panic("talloc: malloc fault");

			(freelist[u] + u*(r-1))->link = 0;
			for (m = freelist[u] + u*(r-2); m >= freelist[u]; m -= u)
				m->link = m+u;
		}
		log(ALLOC, (int) u, (long) 1);

		m = freelist[u];
		freelist[u] = m->link;
	}
	m->size = u | A_USER;
	/*
	for (r = 1; r < u; )(&m->size)[r++] = 0;
	*/
	return (char *) (m+1);
}

void tfree(void *v){ 
	register union M *m = (union M *) v;
	register long u;

	--m;
	if( (m->size&0xFF000000) != A_USER)
		panic("tfree: releasing a free block");

	u = (m->size &= 0xFFFFFF);
	if( u >= A_LARGE ){	
		log(FREE, (int) 0, (long) 1);
		free(m);
	} else {	
		log(FREE, (int) u, (long) 1);
		m->link = freelist[u];
		freelist[u] = m;
	}
}

double tstats(){ 
	register int i;
	long p, a, f, j;
	long sum = 0;

	fprintf(stderr, "chunk\t  pool\tallocs\t frees\t spill\n");
	for(i = 0; i < A_LARGE; i++){
		p = event[POOL][i];
		a = event[ALLOC][i];
		f = event[FREE][i];
		if( !(p|a|f) ) continue;

		j = (long) (i * sizeof(union M));
		fprintf(stderr, "%5d\t%6ld\t%6ld\t%6ld\t%6ld\n", j, p, a, f, a-f);

		sum += p*j;
	}
	fprintf(stderr, "total pools  %7u\n", sum);

	return (double) sum;
}
