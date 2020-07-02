#include <pads.h>
SRCFILE("cache.c")

static int CartesMade, CarteBytes, ItemsMade, ItemBytes;

char *CacheStats(){
	static char report[128];

	sprintf( report, "pads: %d Cartes in %d bytes; %d Items in %d bytes",
		CartesMade, CarteBytes, ItemsMade, ItemBytes );
	return report;
}

Item::Item(char* t,Action a,long o)	{ text = t; action = a; opand = o; }
Item::Item()				{ text = "error"; action = 0; opand = 0; }

int Cache::ok() { return this!=0; }	

Cache::Cache(int size){
	trace( "%d.Cache(%d)", this, size );	VOK;
	SIZE.indx = size;
	current.indx = 1;
}

ItemCache::ItemCache():Cache(2048){
	trace( "%d.ItemCache()", this );	VOK;
	cache = new Item* [SIZE.indx];
	ItemBytes += SIZE.indx*4;				/* pads */
	R->pktstart(P_I_DEFINE);
	R->sendlong(SIZE.indx);
	R->pktend();
};

CarteCache::CarteCache():Cache(2048){
	trace( "%d.CarteCache()", this );	VOK;
	cache = new Carte* [SIZE.indx];
	CarteBytes += SIZE.indx*4;				/* pads */
	R->pktstart(P_C_DEFINE);
	R->sendlong(SIZE.indx);
	R->pktend();
};

int ItemCache::compare(Item *a, Item *b){
	int cmp = strcmp(a->text,b->text);
	if (cmp)
		return cmp;
	if (a->action == b->action)
		return a->opand - b->opand;
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

	trace("%d.index(%s,%d,%d)", this, i.text, i.action, i.opand);OK(0);
	for (b = root; b; above = b, b = cmp < 0 ? b->left : b->right) {
		cmp = compare(&i, cache[b->index.indx]);
		if (!cmp) {
			trace("%d", b->index.indx);
			return b->index;
		}
	}
	b = new Binary;
	if (!root)
		root = b;
	else if (cmp < 0)
		above->left = b;
	else
		above->right = b;
	size = strlen(i.text) + 1;
	trace("%d %d", size, current.indx);
	if (current.indx >= SIZE.indx) {
		trace("Item alloc %d", current.indx);
		Item **old = cache;
		cache = new Item* [SIZE.indx*2];
		for(int i = 0; i < SIZE.indx; i++)
			cache[i] = old[i];
		delete old;
		ItemBytes += SIZE.indx*4;		// pads
		SIZE.indx *= 2;
		R->pktstart(P_I_DEFINE);
		R->sendlong(SIZE.indx);
		R->pktend();
	}
	b->index = current;
	copy = i;
	copy.text = sf("%0.64s", i.text);
	cache[current.indx] = new Item;
	*(cache[current.indx]) = copy;
	trace("%s:%d:%d", copy.text, copy.action, copy.opand);
	R->pktstart(P_I_CACHE);
	R->sendlong(current.indx);
	R->sendstring(sf("%0.64s", copy.text));
	R->pktend();
	++ItemsMade;
	current.indx++;
	trace("%s", CacheStats());
	trace("%d", b->index.indx);
	return b->index;
}

Item *ItemCache::take(Index i){
	trace("%d.take(%d)", this, i.indx); OK(0);
	if ((i.indx & CARTE) || i.indx >= SIZE.indx || !cache[i.indx])
		abort();
	return cache[i.indx];
}

int CarteCache::compare(Carte *a, Carte *b){
	int i;
	IF_LIVE(!a || !b || a->size <= 0 || b->size <= 0)
		return 0;
	if (a->size != b->size)
		return a->size - b->size;
	for(i = 0; i <= a->size; ++i )
		if( a->bin[i].indx != b->bin[i].indx )
			return a->bin[i].indx - b->bin[i].indx;
	return 0;
}

Index CarteCache::place(Carte *c){
	Binary	*b, *above;
	int	cmp, i;
	Carte	*copy;
	Index	ix;

	trace( "%d.place(%d)", this, c );	OK(0);
	for (b = root; b; above = b, b = cmp < 0 ? b->left : b->right) {
		cmp = compare(c, cache[b->index.indx]);
		if (!cmp) {
			trace("%d", b->index.indx);
			ix.indx = b->index.indx | CARTE;
			return ix;
		}
	}
	b = new Binary;
	if (!root)
		root = b;
	else if (cmp < 0)
		above->left = b;
	else
		above->right = b;

	if (current.indx >= SIZE.indx) {
		trace("Carte alloc %d", current.indx);
		Carte **old = cache;
		cache = new Carte* [SIZE.indx*2];
		for(int i = 0; i < SIZE.indx; i++)
			cache[i] = old[i];
		delete old;
		CarteBytes += SIZE.indx*4;			// pads
		SIZE.indx *= 2;
		R->pktstart(P_C_DEFINE);
		R->sendlong(SIZE.indx);
		R->pktend();
	}
	b->index = current;
	cache[current.indx] = copy = (Carte *)new char[CARTESIZE(c->size)];
	CarteBytes += c->size*4+8;				// pads
	*copy = *c;
	for (i = 0; i <= copy->size; ++i)
		copy->bin[i] = c->bin[i];
	R->pktstart(P_C_CACHE);
	R->sendlong(current.indx);
	if (copy->attrib & NUMERIC) {
		R->sendlong(1);
		R->senduchar(copy->attrib);
		R->sendlong(copy->bin[0].indx);
		R->sendlong(copy->bin[1].indx);
	} else {
		R->sendlong(copy->size);
		R->senduchar(copy->attrib);
		for (i = 0; i <= copy->size; ++i)
			R->sendlong(copy->bin[i].indx);
	}
	cartelimits(copy);
	R->sendlong(copy->items);
	R->senduchar(copy->width);
	R->pktend();
	++CartesMade;
	++current.indx;
	trace("%s", CacheStats());
	trace("%d", b->index.indx);
	ix.indx = b->index.indx | CARTE;
	return ix;
}

Carte *CarteCache::take(Index i){
	trace( "%d.take(%d)", i.indx); OK(0);
	IF_LIVE(!(i.indx&CARTE))
		return 0;
	i.indx &= ~CARTE;
	IF_LIVE(i.indx >=SIZE.indx || !cache[i.indx])
		return 0;
	return cache[i.indx];
}

Index CarteCache::numeric(int lo, int hi){
	Index ix;
	Carte *c;

	trace( "%d.Carte(%d,%d)", this, lo, hi );	OK(0);
	IF_LIVE(lo > hi)
		return 0;
	c = (Carte *) new char [CARTESIZE(2)];
	if (hi > lo + 255)
		hi = lo + 255;
	c->size = 2;
	c->attrib = NUMERIC;
	c->bin[1] = Index(lo);
	c->bin[2] = Index(hi);
	ix = place(c);
	trace( "%u", ix.indx );
	return ix;
}

Index NumericRange(short lo, short hi){
	return CCache->numeric((int)lo, (int)hi);
}

void CarteCache::cartelimits(Carte *c){
	trace( "%d.ItemCount(%d)", this, c ); VOK;
	c->items = c->width = 0;
	if(c->attrib & NUMERIC) {
		c->items = c->bin[2].indx - c->bin[1].indx + 1;
		c->width = 5;			/* max log10 d ? */
		return;
	}
	for (int j = 1; j <= c->size; ++j) {
		if (c->bin[j].indx & CARTE) {
			Carte *t = take(c->bin[j]);
			if (t->bin[0].null()){
				c->items += t->items;
				if (t->width > c->width)
					c->width = t->width;
			} else {
				++c->items;
				int l = strlen(ICache->take(t->bin[0])->text);
				l += 3;	// room for cascade arrow in menu
				if (l > (int)c->width)
					c->width = l;
			}
		} else {
			++c->items;
			int l = strlen(ICache->take(c->bin[j])->text);
			if (l > (int)c->width)
				c->width = l;
		}
	}
}
