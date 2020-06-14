#include <pads.h>
#include <stdarg.h>
#include <ctype.h>
SRCFILE("pad.c")

Remote *R;

void Pad::makecurrent()	{ termop(P_MAKECURRENT); }
void Pad::clear()	{ termop(P_CLEAR); }

void Pad::alarm(short n){
	R->pktstart(P_ALARM);
	R->sendobj( _object );
	R->sendshort( n );
	R->pktend();
}

Pad::~Pad()		{ trace("0x%08x.~Pad()", this); VOK; termop(P_DELETE); }
int Pad::ok()		{ return this!=0; }
int Line::ok()		{ return this!=0; }

#ifdef TAC
Attrib Implicits(PadRcv *obj){
	Attrib accum = 0;
	PadRcv padrcv;

	if( !obj ) return 0;
	if( &obj->kbd	    != &padrcv.kbd	 ) accum |= ACCEPT_KBD; // warning
	if( &obj->userclose != &padrcv.userclose ) accum |= USERCLOSE;  // warning
	trace( "Implicits(%d,0x%X)", obj, accum );
	return accum;
}
#endif

Pad::Pad(PadRcv *o){
	trace( "0x%08x.Pad(%d)", this, o );		VOK;
	if( o && !o->isvalid() )
		PadsWarn( "Pad::Pad: object is not a PadRcv" );
	_name = "<name>";
	_banner = "<banner>";
	_object = o;
	_attributes = 0;
	R->pktstart( P_PADDEF );
	R->sendobj( _object );
	R->sendshort( _object ? _object->oid : 0 );
	R->pktend();
#ifdef TAC
	options( Implicits(_object) );
#else
	options(0);
#endif
#ifdef DAK
	helpmenu();
#endif
}

void Pad::nameorbanner(Protocol p, const char *fmt, va_list ap){
	const char *t, **_born = (p==P_BANNER ? &_banner : &_name);
	trace( "0x%08x.nameorbanner(0x%X,%s) %s", this, p, fmt, *_born );	VOK;
	t = vf( fmt, ap );
	va_end(ap);
	if( strcmp( t, *_born ) ){
		R->pktstart( p );
		R->sendobj( _object );
		R->sendstring( *_born = t );
		R->pktend();
	}
}
	
void Pad::banner(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	nameorbanner(P_BANNER, fmt, ap);
	va_end(ap);
}
	
void Pad::name(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	nameorbanner(P_NAME, fmt, ap);
	va_end(ap);
}

void Pad::tabs(short n){
	short lo = 1, hi = 127;

	trace( "0x%08x.tabs(%d)", this, n );	VOK;
	if( n<lo || n>hi )
		PadsWarn( "tabs(%d) should be >=%d and <=%d", n, lo, hi );
	else {
		R->pktstart( P_TABS );
		R->sendobj( _object );
		R->sendshort( n );
		R->pktend();
	}
}

void Pad::removeline(long k){
	trace( "0x%08x.removeline(%d)", this, k );		VOK;
	R->pktstart( P_REMOVELINE );
	R->sendobj( _object );
	R->sendlong( k );
	R->pktend();
}
	
void Pad::createline(long lo, long hi){
	trace( "0x%08x.createline(%d,%d)", this, lo, hi );		VOK;
	if( lo>hi ) return;
	R->pktstart( P_CREATELINE );
	R->sendobj( _object );
	R->sendlong( lo );
	R->sendlong( hi );
	R->pktend();
}
	
void Pad::createline(long k){
	createline(k, k);
}

#ifdef HELPMENU
void Pad::helpmenu(){
	Menu m;
	long i;
	static const char *helptags[] = {
		"overview", "menu bar", "keyboard",
		"line menus", "line keyboard" };

	IF_LIVE(!_object)
		return;
	int cnt = 0;
	for(i = 0; i < HELP_NTOPICS; i++)
		if (_object->help(i)) {
			m.last(helptags[i], (Action)&PadRcv::showhelp, i);
			cnt++;
		}
	if (cnt) {
		Index ix = m.index("help");
		R->pktstart( P_HELPCARTE );
		R->sendobj( _object );
		R->sendshort( ix.sht() );
		R->pktend();
	}
}
#endif
	
void Pad::menu(Index ix){
	trace( "0x%08x.menu(0x%X)", this, ix.sht() );	VOK;
	IF_LIVE( !_object ) return;
	R->pktstart( P_CARTE );
	R->sendobj( _object );
	R->sendshort( ix.sht() );
	R->pktend();
}

void Pad::menu(Menu &m){
	trace( "0x%08x.menu(0x%08x)", this, &m );	VOK;
	IF_LIVE( !&m ) return;
	menu(m.index());
}

void Pad::options(Attrib on, Attrib off){
	trace( "0x%08x.options(0x%X,0x%X)", this, on, off );VOK;
	_attributes |= on;
	_attributes &= ~off;
	R->pktstart( P_ATTRIBUTE );
	R->sendobj( _object );
	R->sendshort( _attributes );
	R->pktend();
}

void Pad::lines(long l){
	trace( "0x%08x.lines(%d)", this, l );		VOK;
	IF_LIVE( l<0 ) return;
	R->pktstart( P_LINES );
	R->sendobj( _object );
	R->sendlong( _lines = l );
	R->pktend();
}	

void Pad::termop(Protocol p){
	trace( "0x%08x.termop(%d)", this, int(p) ); VOK;
	R->pktstart( p );
	R->sendobj( _object );
	R->pktend();
}

void Pad::insert(long k, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vinsert(k, (Attrib)0, (PadRcv*)0, ZIndex, fmt, ap);
	va_end(ap);
}

void Pad::insert(long k, Attrib a, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vinsert(k, a, (PadRcv*)0, ZIndex, fmt, ap);
	va_end(ap);
}

void Pad::insert(long k, Attrib a, PadRcv *o, Menu &m, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vinsert(k, a, o, &m ? m.index() : ZIndex, fmt, ap);
	va_end(ap);
}

void Pad::vinsert(long k, Attrib a, PadRcv *o, Index ix, const char *fmt, va_list ap){
	Line l;
	char t[1024];

	trace( "0x%08x.vinsert(%d,0x%X,%d,%d,%s)", this, k, a, o, ix.sht(), fmt );	VOK;
	vsprintf( l.text = t, fmt, ap );
	l.key = k ? k : UniqueKey();
	if( !o ) a &= ~ACCEPT_KBD;
	l.attributes = a;
	l.object = o;
	l.carte = o ? ix : ZIndex;
	insert(l);
}

void Pad::insert(long k, Attrib a, PadRcv *o, Index ix, const char *fmt, ...){
	Line l;
	char t[1024];
	va_list ap;

	va_start(ap, fmt);
	trace( "%u.insert(%d,0x%X,%d,%d,%s)", this, k, a, o, ix.sht(), fmt );	VOK;
	vsprintf( l.text = t, fmt, ap );
	l.key = k ? k : UniqueKey();
	if( !o ) a &= ~ACCEPT_KBD;
	l.attributes = a;
	l.object = o;
	l.carte = o ? ix : ZIndex;
	insert(l);
	va_end(ap);
}

static Line prev; /* = { 0, 0, 0, 0, {0,0} } - cfront bug */
void Pad::insert(Line &l){
	char buf[256];
	register char *from;
	register int to;
	static Pad *prevpad;

	trace("0x%08x.insert(%d,%s,%d,%X)",this,l.key,l.text,l.object,l.attributes);VOK;
	if( l.object && !l.object->isvalid() )
		PadsWarn("Pad::insert: object is not a PadRcv");
	if( _lines>0 && (l.key<1 || l.key>_lines) ){
		PadsWarn("Pad::insert: key out of range: %d %s", l.key, l.text);
		return;
	}
	for( from = l.text, to = 0; *from && to<250; ++from )
		buf[to++] = isprint(*from) || *from=='\t'  ? *from : ' ';
	buf[to] = '\0';
	if(         this == prevpad
	&&         l.key == prev.key+1
	&& l.carte.sht() == prev.carte.sht()
	&&  l.attributes == prev.attributes ){
		trace( "P_NEXTLINE %d", l.key );
		R->pktstart( P_NEXTLINE );
		R->sendobj( l.object );
		R->sendshort( l.object ? l.object->oid : 0 );
	} else {
		R->pktstart( P_LINE );
		R->sendobj( _object );
		R->sendobj( l.object );
		R->sendshort( l.object ? l.object->oid : 0);
		R->sendlong( l.key );
		R->sendshort( l.carte.sht() );
		R->sendshort( l.attributes );
	}
	R->sendstring( buf );
	if( l.attributes&FLUSHLINE || l.key==prev.key )
		R->pktflush();
	else
		R->pktend();
	prev = l;
	prevpad = this;
}

Line::Line(){
	trace( "0x%08x.Line()", this ); VOK;
	object = 0;
	text = (char*)"";
	key = 0;
	attributes = 0;
	carte = 0;
}

long UniqueKey() { static long u; return u += 1024; }

void Pad::error( const char *fmt, ... ){
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	trace( "%d.error(%s)", this, fmt );	VOK;
	if( errorkey ) removeline( errorkey );
	errorkey = 0;
	if( !fmt || !*fmt ) return;
	vsprintf(buf, fmt, ap);
	insert(errorkey = UniqueKey(), SELECTLINE, buf);
	va_end(ap);
}

void Pad::makegap(long k, long gap){
	trace( "0x%08x.makegap(%d,%d)", this, k, gap ); VOK;
	R->pktstart( P_MAKEGAP );
	R->sendobj( _object );
	R->sendlong( k );
	R->sendlong( gap );
	R->pktend();
}
