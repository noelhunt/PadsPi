#include "univ.h"

char **ICache;
Carte ***CCache;
Index I_SIZE, C_SIZE;

void CacheOp(Protocol p){
	Index i;
	int size;
	Carte *c;
	Index *rcv;

	i = RcvShort();
	switch( (int) p ){
	case P_I_DEFINE:
		assert(!ICache);
		I_SIZE = i;
		ICache = (char **) Alloc(MJR(I_SIZE) * sizeof(char*) );
		break;
	case P_C_DEFINE:
		assert(!CCache);
		C_SIZE = i;
		CCache = (Carte***) Alloc(MJR(C_SIZE) * sizeof(Carte*) );
		break;
	case P_I_CACHE:
#ifdef GCALLOC
		if( !ICache[MJR(i)] ) GCAlloc(MNR(I_SIZE), &ICache[MJR(i)]);
#else
		if( !ICache[MJR(i)] ) ICache[MJR(i)] =
			(char*) Alloc(MNR(I_SIZE) * sizeof(Carte*));
#endif
		RcvString( &ICache[MJR(i)][MNR(i)] );
		break;
	case P_C_CACHE:
		size = RcvUChar();
		if( !CCache[MJR(i)] ) CCache[MJR(i)] =
			(Carte**) Alloc(MNR(C_SIZE) * sizeof(Carte*));
		assert( !CCache[MJR(i)][MNR(i)] );
#ifdef GCALLOC
		c = (Carte*) GCAlloc(CARTESIZE(size), (char **)&CCache[MJR(i)][MNR(i)]);
#else
		c = CCache[MJR(i)][MNR(i)] = (Carte*) Alloc(CARTESIZE(size));
#endif
		c->attrib = RcvUChar();
		for( rcv = c->bin; size-- >= 0; *rcv++ = RcvShort()) {}
		c->size = RcvUChar();			/* recursive size   */
		c->width = RcvUChar();			/* recursive widest */
		break;
	default:
		ProtoErr( "CacheOp(): " );

	}
}

char *IndexToStr(Index i){
	assert( !(MJR(i)&CARTE) && ICache[MJR(i)] && MJR(i)<MJR(I_SIZE) );
	return &ICache[MJR(i)][MNR(i)];
}

Carte *IndexToCarte(Index i){
	assert( MJR(i)&CARTE );
	Index j = i;
	i &= ~(CARTE<<8);
	assert(MJR(i)<MJR(C_SIZE) && CCache[MJR(i)] && CCache[MJR(i)][MNR(i)]);
	return CCache[MJR(i)][MNR(i)];
}
