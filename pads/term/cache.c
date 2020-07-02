#include "univ.h"

char **ICache;
Carte **CCache;
Index I_SIZE, C_SIZE;

void CacheOp(Protocol p){
	Index i;
	int j, size;
	Carte *c;
	Index *rcv;
	char **PICache;
	Carte **PCCache;

	i = RcvLong();
	switch( (int) p ){
	case P_I_DEFINE:
		if ( !ICache )
			ICache = (char **)Alloc(i * sizeof(char*));
		else{
			PICache = ICache;
			ICache = (char **)Alloc(i * sizeof(char*));
			for(j = 0; j < I_SIZE; j++)
				ICache[j] = PICache[j];
			free(PICache);
		}
		I_SIZE = i;
		break;
	case P_C_DEFINE:
		if ( !CCache )
			CCache = (Carte**) Alloc(i * sizeof(Carte*));
		else{
			PCCache = CCache;
			CCache = (Carte**) Alloc(i * sizeof(Carte*));
			for(j = 0; j < C_SIZE; j++)
				CCache[j] = PCCache[j];
			free(PCCache);
		}
		C_SIZE = i;
		break;
	case P_I_CACHE:
		assert(!ICache[i], "P_I_CACHE");
		RcvAllocString( &ICache[i] );
		break;
	case P_C_CACHE:
		assert(!CCache[i], "P_C_CACHE");
		size = RcvLong();
		c = CCache[i] = (Carte*) Alloc(CARTESIZE(size));
		c->attrib = RcvUChar();
		for( rcv = c->bin; size-- >= 0; *rcv++ = RcvLong()) {}
		c->size = RcvLong();			/* recursive size   */
		c->width = RcvUChar();			/* recursive widest */
		break;
	default:
		ProtoErr( "CacheOp(): " );

	}
}

char *IndexToStr(Index i){
	assert(!(i & CARTE) && i < I_SIZE, "IndexToStr");
	return ICache[i];
}

Carte *IndexToCarte(Index i){
	assert(i & CARTE, "IndexToCarte");
	i &= ~CARTE;
	assert(i < C_SIZE && CCache[i], "IndexToCarte C_SIZE");
	return CCache[i];
}
