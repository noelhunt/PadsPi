#include <pads.h>
SRCFILE("cache.c")

static int CartesMade, CarteBytes, ItemsMade, ItemBytes;

char *CacheStats(){
	static char report[128];

	sprintf( report, "pads: %d Cartes in %d bytes; %d Items in %d bytes",
		CartesMade, CarteBytes, ItemsMade, ItemBytes );
	return report;
}

short Index::sht()			  { return (major<<8) | minor; }

Item::Item(const char* t,Action a,long o) { text = t; action = a; opand = o;	}
Item::Item()				  { text = "error"; action = 0; opand = 0;}

int Cache::ok() { return this!=0; }	

Cache::Cache( unsigned char maj, unsigned char min ){
	trace( "0x%08x.Cache(%d,%d)", this, maj, min );	VOK;
	root = 0;
	SIZE = Index(maj,min);
	current = Index(0,1);
}

ItemCache::ItemCache():Cache(100,200){		/* (250,250) for megabyte term */
	trace( "0x%08x.ItemCache()", this );	VOK;
	cache = new Item** [SIZE.major];
	ItemBytes += SIZE.major*4;		/* term */
	R->pktstart( P_I_DEFINE );
	R->sendshort(SIZE.sht());
	R->pktend();
};

CarteCache::CarteCache():Cache(50,100){		/* (250,250) for megabyte term */
	trace( "0x%08x.CarteCache()", this );	VOK;
	cache = new Carte** [SIZE.major];
	CarteBytes += SIZE.major*4;		/* term */
	R->pktstart( P_C_DEFINE );
	R->sendshort(SIZE.sht());
	R->pktend();
};

int ItemCache::compare(Item *a, Item *b){
	int cmp = strcmp(a->text,b->text);
	if (cmp) return cmp;
	if (a->action == b->action) return a->opand - b->opand;
#ifdef ANACHRONISM
	/*
	 * Pointers to member functions are no longer
	 * treated as pointers and hence no arithmetic
	 * can be done on them.
	 */
	return (long)a->action - (long)b->action;
#else
	return a - b;
#endif
}

Index ItemCache::place(Item i){
	Binary	*b, *above;
	int	cmp, size;
	Item	copy;

	trace("0x%08x.place(%s,%d,%d)", this, i.text, i.action, i.opand);OK(0);
	for (b = root; b; above = b, b = cmp < 0 ? b->left : b->right) {
		cmp = compare( &i, cache[b->index.major][b->index.minor] );
		if (!cmp) {
			trace( "0x%08x:%d", b->index.major, b->index.minor );
			return b->index;
		}
	}
	b = new Binary;
	if (!root) root = b;
	else if (cmp < 0) above->left = b;
	else above->right = b;
	if( (size = strlen(i.text)+1) >= SIZE.minor ) abort(/* "cache" */);
	trace( "%d %d %d", size, current.minor, size+current.minor );
	if( size+current.minor >= SIZE.minor ){
		if( ++current.major >= SIZE.major )
			abort(/* "ItemCache overflow" */);
		current.minor = 0;
	}
	if( !cache[current.major] ){
		trace( "Item alloc %d", current.major );
		cache[current.major] = new Item* [SIZE.minor];
		ItemBytes += SIZE.minor;		// term
	}
	b->index = current;
	copy = i;
	copy.text = sf("%0.64s", i.text);
	cache[current.major][current.minor] = new Item;
	*(cache[current.major][current.minor]) = copy;
	trace("%s:%d:%d", copy.text, copy.action, copy.opand);
	R->pktstart(P_I_CACHE);
	R->sendshort(current.sht());
	R->sendstring(sf("%0.64s", copy.text));
	R->pktend();
	++ItemsMade;
	current.minor += size;
	trace("%s", CacheStats());
	trace( "%d:%d", b->index.major, b->index.minor );
	return b->index;
}

Item *ItemCache::take(Index i){
	trace( "0x%08x.take(%d:%d)", this, i.major, i.minor ); OK(0);
	if( (i.major&CARTE) || i.major>=SIZE.major
	 || !cache[i.major] || !cache[i.major][i.minor] ) abort(/* "cache" */);
	return cache[i.major][i.minor];
}

int CarteCache::compare(Carte *a, Carte *b){
	int i;
	IF_LIVE(!a || !b || a->size <= 0 || b->size <= 0)
		return 0;
	if (a->size != b->size)
		return a->size - b->size;
	for(i = 0; i <= a->size; ++i )
		if( a->bin[i].sht() != b->bin[i].sht() )
			return a->bin[i].sht() - b->bin[i].sht();
	return 0;
}

Index CarteCache::place(Carte *c){
	Binary	*b, *above;
	int	cmp, i;
	Carte	*copy;

	trace( "0x%08x.place(%d)", this, c );	OK(0);
	for (b = root; b; above = b, b = cmp < 0 ? b->left : b->right) {
		cmp = compare( c, cache[b->index.major][b->index.minor] );
		if (!cmp) {
			trace( "0x%08x:%d", b->index.major, b->index.minor );
			{ Index ix = b->index; ix.major|=CARTE; return ix; }
		}
	}
	b = new Binary;
	if (!root)
		root = b;
	else if (cmp < 0) above->left = b;
	else above->right = b;
	if( current.minor >= SIZE.minor ){
		if( ++current.major >= SIZE.major )
			abort(/* "CarteCache overflow" */);
		current.minor = 0;
	}
	if( !cache[current.major] ){
		trace( "Carte alloc %d", current.major );
		cache[current.major] = new Carte* [SIZE.minor];
		CarteBytes += SIZE.minor*4;		/* term */
	}
	b->index = current;
#ifndef SAFE
	cache[current.major][current.minor] = copy =
		(Carte *) new char [CARTESIZE(c->size)];
#else
	cache[current.major][current.minor] = copy = new Carte(c->size);
#endif
	CarteBytes += c->size*4+8;			/* term */
	*copy = *c;
	for (i = 0; i <= copy->size; ++i) copy->bin[i] = c->bin[i];
	R->pktstart(P_C_CACHE);
	R->sendshort(current.sht());
	if ( copy->attrib&NUMERIC ) {
		R->senduchar( 1 );
		R->senduchar( copy->attrib );
		R->sendshort( copy->bin[0].sht() );
		R->sendshort( copy->bin[1].sht() );
	} else {
		R->senduchar( copy->size );
		R->senduchar( copy->attrib );
		for (i = 0; i <= copy->size; ++i)
			R->sendshort( copy->bin[i].sht() );
	}
	cartelimits(copy);
	R->senduchar(copy->items);
	R->senduchar(copy->width);
	R->pktend();
	++CartesMade;
	++current.minor;
	trace("%s", CacheStats());
	trace( "%d:%d", b->index.major, b->index.minor );
	{ Index ix = b->index; ix.major|=CARTE; return ix; }
}

Carte *CarteCache::take(Index i){
	trace( "0x%08x.take(0x%X)", i.sht() ); OK(0);
	IF_LIVE( !(i.major&CARTE) ) return 0;
	i.major &= ~CARTE;
	IF_LIVE(i.major>=SIZE.major || !cache[i.major] || !cache[i.major][i.minor])
		return 0;
	return cache[i.major][i.minor];
}

Index CarteCache::numeric(int lo, int hi){
	Index ix;
	Carte *c;

	trace( "0x%08x.Carte(%d,%d)", this, lo, hi );	OK(0);
	IF_LIVE( lo > hi ) return 0;
#ifndef SAFE
	c = (Carte *) new char [CARTESIZE(2)];
#else
	c = new Carte(2);
#endif
	if (hi > lo + 255) hi = lo + 255;
	c->size = 2;
	c->attrib = NUMERIC;
	c->bin[1] = Index(lo);
	c->bin[2] = Index(hi);
	ix = place(c);
	trace( "%u:%u", ix.major, ix.minor );
	return ix;
}

Index NumericRange(short lo, short hi) { return CCache->numeric((int)lo, (int)hi); }

void CarteCache::cartelimits(Carte *c){
	trace( "0x%08x.ItemCount(%d)", this, c ); VOK;
	c->items = c->width = 0;
	if(c->attrib & NUMERIC) {
		c->items = c->bin[2].sht() - c->bin[1].sht() + 1;
		c->width = 5;			/* max log10 d ? */
		return;
	}
	for (int j = 1; j <= c->size; ++j) {
		if( c->bin[j].major&CARTE ){
			Carte *t = take(c->bin[j]);
			if (t->bin[0].null()){
				c->items += t->items;
				if (t->width > c->width) c->width = t->width;
			} else {
				++c->items;
				int l = strlen(ICache->take(t->bin[0])->text);
				l += 3;	// room for cascade arrow in menu
				if (l > (int)c->width) c->width = l;
			}
		} else {
			++c->items;
			int l = strlen(ICache->take(c->bin[j])->text);
			if (l > (int)c->width) c->width = l;
		}
	}
}
