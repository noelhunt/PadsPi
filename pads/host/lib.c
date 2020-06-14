#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

void PadsError(const char* ...);

#define NEWDEL
#define	log(e, u, d) { event[u][e] += d; }

char literal[] = "";

Allocator NewDel;

Allocator::Allocator(){}

char *Allocator::profile(long u){
	char *s = literal;
	static char report[64];
	long p = event[u][POOL], a = event[u][ALLOC], f = event[u][FREE];

	if( u >= A_LARGE ) return 0;
	if( !(p|a|f) ) return s;
	u = u ? (u-1)*sizeof(M) : 0;
	sprintf(report, "size=%5d: pool=%6d new=%8d del=%8d tot=%8d",
		u, p, a, f, (u+4)*p);
	return report;
}

void *Allocator::alloc(size_t u){
	M *m;
	int r;

	if( u <= 0 ) PadsError( "Allocator.alloc(%d)", u );
	u = ((u-1)/sizeof(M)) + 2;
	if( u >= A_LARGE ){
		log(ALLOC,0,u*sizeof(M));
		m = (M *)malloc(u*sizeof(M));
		if( !m ) PadsError("host out of memory");
	} else {
		if( !freelist[u] ){
			if( req[u] == 0 ) req[u] = 1;
			r = req[u];
			if( req[u] < 256 ) req[u] *= 2;
			log(POOL,u,r);
			freelist[u] = (M *)malloc(r*u*sizeof(M));
			if( !freelist[u] ) PadsError("host out of memory");
			for( m = freelist[u]; --r > 0; m = m->link = m+u );
			m->link = 0;
		}
		log(ALLOC,u,1);
		m = freelist[u];
		freelist[u] = m->link;
	}
	m->size = u|A_USER;
	for( r = 1; r < u; )
		(&m->size)[r++] = 0;
	return m+1;
}

void Allocator::free(void *v){
	void free(M*);
	M* m = (M*) v;
	long u;

	--m;
	if( (m->size&0xFF000000) != A_USER ) PadsError( "delete error" );
	u = (m->size &= 0xFFFFFF);
	if( u >= A_LARGE ){
		log(FREE,0,u*sizeof(M));
		::free(m);
	}else{
		log(FREE,u,1);
		m->link = freelist[u];
		freelist[u] = m;
	}
}

void *zalloc(size_t size){
	void *p = malloc(size);
	memset(p, 0, size);
        return p;
}

#include <new>

typedef void *PV;
typedef void (*PF)(PV);

void *operator new( std::size_t size ) throw(std::bad_alloc) {
	if( size<= 0 ) PadsError("new(%d)", size);
#ifdef NEWDEL
	return NewDel.alloc(size);
#else
	void *p = malloc(size);
	memset(p, 0, size);
        return p;
#endif
}

void *operator new[]( std::size_t size ) throw(std::bad_alloc) {
	if( size<= 0 ) PadsError("new[](%d)", size);
#ifdef NEWDEL
	return NewDel.alloc(size);
#else
	void *p = malloc(size);
	memset(p, 0, size);
        return p;
#endif
}

void operator delete(PV p) throw() {
	void free(void*);
#ifdef NEWDEL
	if(p) NewDel.free(p);
#else
	::free(p);
#endif
}

void operator delete[](PV p) throw() {
	void free(void*);
#ifdef NEWDEL
	if(p) NewDel.free(p);
#else
	::free(p);
#endif
}

char *StrDup(const char *s1){
	size_t nbytes;
	char *s2;

	nbytes = strlen(s1) + 1;
	s2 = new char[nbytes];
	if (s2 == NULL) return NULL;
	memcpy(s2, s1, nbytes);
	return s2;
}

PV _vec_new(PV op, int n, int sz, PV f){
	int i;
	char* p;
	if (op == 0) op = PV( new char[n*sz] );
	p = (char*) op;
	for (i=0; i<n; i++) ( *PF(f) )( PV(p+i*sz) );
	return PV(p);
}

void _vec_delete(PV op, int n, int sz, PV f, int){	// what is that last int?
	int i;
	char* p = (char*) op;
	for (i=0; i<n; i++) ( *(PF)f )( (PV)(p+i*sz) );
}
