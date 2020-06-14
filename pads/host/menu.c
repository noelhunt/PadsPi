#include <pads.h>
SRCFILE("menu.c")

#define MAXMENUSIZE	0x7FFFFFFF

Index ZIndex;

Menu::Menu() { list = 0; size = 0; }

Menu::Menu(const char *t, Action a, long o){
	trace( "0x%08x.Menu(%s,%d,%d)", this, t, a, o );
	list = 0;
	size = 0;
	first(t, a, o);
}

Menu::~Menu(){
	IList *l, *lnext;

	for( l = list; l; l = lnext ){
		lnext = l->next;		// new malloc()
		delete l;
	}
}

void Menu::first(const char *t, Action a, long o){
	trace( "0x%08x.first(%s,%d,%d)", this, t, a, o );
	if (size == MAXMENUSIZE)
		return;
	first( ICache->place( Item(t,a,o) ) );
}

void Menu::last(const char *t, Action a, long o){
	trace( "0x%08x.last(%s,%d,%d)", this, t, a, o );
	if (size == MAXMENUSIZE)
		return;
	last( ICache->place( Item(t,a,o) ) );
}

void Menu::first(Index i){
	trace( "0x%08x.first(%u)", this, i.sht() );
	if( i.null() || size == MAXMENUSIZE )
		return;
	list = new IList(i,list);
	++size;
}

void Menu::last(Index i){
	IList *l;

	trace( "0x%08x.last(%u)", this, i.sht() );
	if( i.null() || size == MAXMENUSIZE )
		return;
	++size;
	if( !list ){
		list = new IList(i,0);
		return;
	}
	for( l = list; l->next; l = l->next ) {}
	l->next = new IList(i,0);
}

int IndexTextCmp( Index a, Index b ){
	trace( "IndexTextCmp(%u,%u)", a.sht(), b.sht() );
	return strcmp( ICache->take(a)->text, ICache->take(b)->text );
}

void Menu::sort(const char *t, Action a, long o ){
	int cmp;
	IList **p;

	trace( "0x%08x.sort(%s,%d,%d)", this, t, a, o );
	if (size == MAXMENUSIZE)
		return;
	Index i = ICache->place( Item(t,a,o) );
	if( !list ){
		first(i);
		return;
	}
	for( p = &list; *p; p = &((*p)->next) ){
		trace( "%u", (*p)->index.sht() );
		cmp = IndexTextCmp( i, (*p)->index );
		if( cmp == 0 ) return;
		if( cmp < 0 ) break;
	}
	++size;
	*p = new IList(i,*p);
}

Index Menu::index(const char *t, Action a, long o){
	IList *l;
	int i;
	Carte *c;
	
	if( size==0 ) return 0;
	trace( "0x%08x.index() %d t.%s", this, size, t? t: "<null>" );
#ifndef SAFE
	c = (Carte *) new char [CARTESIZE(size)];
#else
	c = new Carte(size);
#endif
	c->attrib = 0;
	c->size = size;
	for( l = list, i = 1; l; l = l->next, ++i )
		c->bin[i] = l->index;
	if( t )
		c->bin[0] = ICache->place(Item(t,a,o));
	Index ix = CCache->place(c);
	delete c;
	return ix;
}

void Menu::dump(){
	IList *l;

	trace( "0x%08x.dump()", this );
	for( l = list; l; l = l->next ){
		trace( "%u", l->index.sht() );
	}
}
