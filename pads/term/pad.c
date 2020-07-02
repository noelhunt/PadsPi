#include "univ.h"
Selection Selected;
Pad *Current;
Pad *HostObject;
Pad *HostParent;
short Scrolly;
Point Zpoint;
Image *tagcols[NCOL];
Image *padcols[NCOL];
Image *kbdcols[NCOL];
Rectangle ZRectangle;
Pad *DirtyPad;

Pad Sentinel = { 0, 0, 0, 0, 0, 0, &Sentinel, &Sentinel };
Rectangle PadSpace;
Rectangle KBDrect;
Attrib NewFold;

#define ISPAD(p)	((p) != &Sentinel)
#define ISLINE(l,p)	((l) != &(p)->sentinel)	/* used how much? */

void Proto(int);

#define DMauve		0xFFAAFFFF
#define DOrange         0xFFA500FF

void PadStart(Rectangle r){
	/* Tag text is pinkish */
	tagcols[BACK] = allocimagemix(display, DMauve, DWhite);
	tagcols[TEXT] = display->black;

	/* Pad text is yellowish */
	padcols[BACK] = allocimagemix(display, DPaleyellow, DWhite);
	padcols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DDarkyellow);
	padcols[BORD] = allocimage(display, Rect(0,0,2,2), screen->chan, 1, DYellowgreen);
	padcols[TEXT] = display->black;
	padcols[HTEXT] = display->black;

	/* Keyboard text is blueish */
	kbdcols[BACK] = allocimagemix(display, DPalebluegreen, DWhite);
	kbdcols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DPurpleblue);
	kbdcols[BORD] = allocimage(display, Rect(0,0,2,2), screen->chan, 1, DPurpleblue);
	kbdcols[TEXT] = display->black;
	kbdcols[HTEXT] = display->white;
}

Point PickPoint( char *s ){
	char *pick = "pick window: ";
	Point p;
	cursswitch(&Bullseye);
	if( s ) InvertKBDrect( pick, s );
	while( !button123() ) waitMOUSE();
	p =  (butts==BUTT2 || butts==BUTT3) ? mouse->xy: Zpoint;
	cursswitch( Jcursor );
	PaintKBD();
	return p;
}

#define MINWD 50
#define MINHT 50
int PadSized( Rectangle r ){
	return r.max.x-r.min.x > MINWD && r.max.y-r.min.y > MINHT;
}

Rectangle getrectmask(int n){
	Rectangle r, rc;
	Mousectl *mc = mousectl;

	cursswitch(&Sweep);
	while(mc->m.buttons)  readmouse(mc);
	while(!mc->m.buttons) readmouse(mc);
	if(!(mc->m.buttons & n)){
		rc.min.x = rc.max.x = 0;
		rc.min.y = rc.max.y = 0;
		while(mc->m.buttons)
			readmouse(mc);
		goto Return;
	}
	r.min = mc->m.xy;
	r.max = mc->m.xy;
	do{
		rc = canonrect(r);
		drawgetrect(rc, 1);
		readmouse(mc);
		drawgetrect(rc, 0);
		r.max = mc->m.xy;
	}while(mc->m.buttons & n);
	cursswitch(nil);
Return:
	return rc;
}

#define SWEEP 125
int SWEEPTIMEOUT = 1000*15;
Rectangle clipgetrect( char *s ) {
	Rectangle r, r1;
	long start;

	if( s ) InvertKBDrect( "sweep ", s );
	for( start = msec(); ; Jwait() ){
		if( ptinrect(mouse->xy, insetrect(PadSpace,-SWEEP)) )
			break;
		if( msec()>start+SWEEPTIMEOUT ){
			r = screen->r;
			while( PadSized(r1 = insetrect(r,1)) )
				r = r1;
			goto TimedOut;
		}
	}
	r = getrectmask(Button2|Button3);
	if( r.max.x && r.max.x-r.min.x<=5 && r.max.y-r.min.y<=5 )
		r = screen->r;	/* from jim */
TimedOut:
	PaintKBD();
	if( !rectclip( &r, PadSpace ) ) return ZRectangle;
	return r;
}

void DelLine( Line *l ){
	assert( l && l->down && l->up, "DelLine" );

	if( Selected.line == l ) Selected.line = 0;
	l->down->up = l->up;
	l->up->down = l->down;
	free( (uchar*)l->text );
	free( l );
}

Line *InsAbove(Line *l, Line *t){
	Line *n;

	assert( l && l->down && l->up, "InsAbove"  );
	if( !t->text ) return 0;		/* || !t->text[0] allow */
	n = salloc(Line);
	*n = *t;
	n->text  = strdup(t->text);
	n->down = l;
	n->up = l->up;
	l->up->down = n;
	l->up = n;
	return n;
}

Line *InsPos(Pad *p, Line *tk){
	Line *l = &p->sentinel;	

	assert( p && tk, "InsPos" );
	if( (p->attributes&SORTED) && tk->text ){
		while( ISLINE(l->up,p) && !dictorder(l->up->text,tk->text) )
			l = l->up;
	} else {
		while( ISLINE(l->up,p) && l->up->key > tk->key )
			l = l->up;
	}
	return l;
}

void CreateLine(Pad *p){
	long lo, hi, k;
	Line *inspos, *l;
	Line fake;
	char tilde;

	lo = RcvLong();
	hi = RcvLong();
	fake.ptop = fake.phit = 255;
	if( p->sentinel.key || p->attributes&SORTED ) return;
	tilde = p->attributes&NO_TILDE ? 0 : '~';
	for( k = lo; k <= hi; ++k ) if( k ){
		fake.key = k;
		fake.carte = 0;
		fake.attributes = FAKELINE;
		fake.text = itoa(k);
		*fake.text = tilde;
		inspos = InsPos(p, &fake);
		if( inspos->up->key == k )
			DelLine( inspos->up );
		InsAbove(inspos, &fake);
		p->attributes |= FAKELINE;
		Dirty(p);
	}
}	

void PutLine( Pad *p, Protocol op ){
	static Line prevrcvd = { 0 };
	Line *l, *inspos;
	Line rcvd;
	char text[1024];			/* 256 ? */

	rcvd = prevrcvd;
	rcvd.object = RcvLong();
        rcvd.oid = RcvShort();
	if( op == P_NEXTLINE )
		rcvd.key = ++prevrcvd.key;
	else {
		rcvd.key = RcvLong();
		rcvd.carte = RcvLong();
		rcvd.attributes = RcvShort();
		prevrcvd = rcvd;
	}
	RcvString(rcvd.text = text);
	if( !p ) return;
	if( p->sentinel.key && rcvd.key>p->sentinel.key ) return;
	inspos = InsPos(p, &rcvd);
	{
		extern Script ObjScript;
		rcvd.ptop = ObjScript.prevtop;
		rcvd.phit = ObjScript.prevhit;
	}
	if( rcvd.attributes&SELECTLINE ) MakeCurrent(p);
	for( l = p->sentinel.up; ISLINE(l,p); l = l->up ){
		if( l->key == rcvd.key ){
			if( l == Selected.line )
				rcvd.attributes |= SELECTLINE;
			if( !strcmp(rcvd.text, l->text)
			 && rcvd.object == l->object
			 && rcvd.carte == l->carte
			 && rcvd.attributes == l->attributes
			 && !(rcvd.attributes&SELECTLINE) )
				return;
			inspos = l->down;
			if( NewFold = l->attributes & (FOLD|TRUNCATE) )
				FoldToggle( &rcvd.attributes );
			rcvd.ptop = l->ptop;
			rcvd.phit = l->phit;
			DelLine( l );
			break;
		}
	}
	l = InsAbove(inspos, &rcvd);
	if( !(rcvd.attributes&DONT_DIRTY) ) Dirty(p);
	if( l && rcvd.attributes&SELECTLINE ){	/* selected <= "" !! */
		Selected.line = l;
		Selected.pad = p;
		Paint(p);
	}
}

void SetCurrent( Pad *p ){
	if( Current ) HeavyBorder(Current);
	if( Current = p ) HeavyBorder( p );
}

void PaintCurrent(){
	if( Current ) Paint(Current);
}

Cover Covered( Pad *p ){	/* should be smarter */
#ifdef TAC
	Pad *f;
	for( f = p->front; ISPAD(f); f = f->front )
		if( rectinrect( p->rect, f->rect ) ) return COMPLETE;
	for( f = p->front; ISPAD(f); f = f->front )
		if( rectXrect( p->rect, f->rect ) ) return PARTIAL;
	return CLEAR;
#else
	extern int SomeVis, SomeInvis;
	if( !PadSized(p->rect) ) return COMPLETE;
	SomeVis = SomeInvis = 0;
	PadBlt( (Image *)0, p->rect, p->front );
	if(SomeVis==0)   return COMPLETE;
	if(SomeInvis==0) return CLEAR;
	return PARTIAL;
#endif
}

void PaintForward(Rectangle, Pad*);

void Dirty( Pad *p ){
	if( p == DirtyPad ) return;
	if( DirtyPad && Covered(DirtyPad)!=COMPLETE )
		if( !ClipPaint(DirtyPad->rect, DirtyPad) )
			PaintForward( DirtyPad->rect, DirtyPad->front );
	DirtyPad = p;
}

void Linkin( Pad *p ){
	SetCurrent( (Pad *) 0 );
	p->back = Sentinel.back;
	p->back->front = p;
	p->front = &Sentinel;
	Sentinel.back = p;
	if( PadSized(p->rect) ) SetCurrent(p);
}

void Unlink( Pad *p ){
	if( p == Current ) SetCurrent( (Pad *) 0 );
	p->back->front = p->front;
	p->front->back = p->back;
	p->front = p->back = 0;		/* redundant - but caught an mcc bug! */
}

void FrontLink( Pad *p ){
	if( !p ) return;
	Unlink(p);
	Linkin(p);
}

void PaintForward( Rectangle r, Pad *p ){
	int busy = 0;
	for( ; p != &Sentinel; p = p->front ){
		if( rectXrect(r, p->rect) && Covered(p)!=COMPLETE ){
			if( !ClipPaint( r, p ) )
				r = boundrect(r,p->rect);
			if( ++busy%4 == 0 ) flushimage(display, 1);
		}
	}
}

void Refresh( Rectangle r ){
	draw(screen, r, display->white, nil, ZPoint);
	PaintForward( r, Sentinel.front );
}

char NewString[] = "<new>";

void P_Define( Pad *p, long o ){
	short oid = RcvShort();
	if ( !p || oid != p->oid ) {
		p = salloc(Pad);		/* zeros */
		p->sentinel.up = p->sentinel.down = &p->sentinel;
		p->sentinel.ptop = 255;
		Linkin( p );
		p->object = o;
		p->oid = oid;
		p->name = NewString;
		p->sentinel.text = NewString;
		p->tabs = 8;
		p->selkey = 0;
	}
	Dirty( p );
}

void P_Carte( Pad *p ){
	Index i = RcvLong();
	if( p && p->object ) p->carte = i;
}

void P_Lines( Pad *p ){
	long k = RcvLong();
	if( p ){
		if( k < p->sentinel.key ) DelLines( p );
		p->sentinel.key = k;
		Dirty( p );
	}
}

void P_Banner( Pad *p ){
	char b[256];
	RcvString(b);
	if( p ){
		if( p->sentinel.text != NewString ) free( (uchar*)p->sentinel.text );
		p->sentinel.text = strdup( b );
		Dirty( p );
	}
}

void P_Name( Pad *p ){
	char n[256];
	RcvString(n);
	if( p ){
		if( p->name != NewString ) free( (uchar*)p->name );
		p->name = strdup( n );
	}
}

void P_Attributes( Pad *p ){
	Attrib a = RcvShort();
	if( p ) p->attributes = a;
}

void P_Tabs( Pad *p ){
	register short t = RcvShort();
	if( p && t>0 && t<128 ) p->tabs = t;
	Dirty( p );
}

void P_RemoveLine( Pad *p ){
	long k = RcvLong();
	Line *l;

	if( !p ) return;
	for( l = p->sentinel.up; ISLINE(l,p); l = l->up ){
		if( l->key == k ){
			DelLine( l );
			Dirty(p);
			return;
		}
	}
	
}

Pad *ObjToPad(long o){
	Pad *p;

	for( p = Sentinel.back; ISPAD(p); p = p->back )
		if( p->object == o ) return p;
	return 0;
}

void Cycle(){
	Pad *p;

	for( p = Sentinel.back; ISPAD(p); p = p->back ){
		if( p->ticks>0 ){
			if( --p->ticks == 0 ){
				HostParent = HostObject = p;
				ToHost( P_CYCLE, /* garbage */ 0 );
			}
		}
	}
}

void MakeGap(Pad *p){
	Line *l, *lsent = &p->sentinel;
	long k = RcvLong();
	long gap = RcvLong();

	for( l = lsent->down; l!=lsent; l = l->down )
		if( l->key >= k ) l->key += gap;
}

# define PROTODEBUG

# ifdef PROTODEBUG
char *OpToStr(Protocol op){
	char *s;
	static char buf[32];
	switch ( op&0xFF ) {
	default: sprint(buf, "P_NONE.%x", op&0xFF); s = buf; break;
	case P_UCHAR: s = "P_UCHAR"; break;
	case P_SHORT: s = "P_SHORT"; break;
	case P_LONG: s = "P_LONG"; break;
	case P_CACHEOP: s = "P_CACHEOP"; break;
	case P_I_DEFINE: s = "P_I_DEFINE"; break;
	case P_I_CACHE: s = "P_I_CACHE"; break;
	case P_C_DEFINE: s = "P_C_DEFINE"; break;
	case P_C_CACHE: s = "P_C_CACHE"; break;
	case P_STRING: s = "P_STRING"; break;
	case P_INDEX: s = "P_INDEX"; break;
	case P_PADDEF: s = "P_PADDEF"; break;
	case P_ATTRIBUTE: s = "P_ATTRIBUTE"; break;
	case P_BANNER: s = "P_BANNER"; break;
	case P_CARTE: s = "P_CARTE"; break;
	case P_LINES: s = "P_LINES"; break;
	case P_NAME: s = "P_NAME"; break;
	case P_TABS: s = "P_TABS"; break;
	case P_HELPCARTE: s = "P_HELPCARTE"; break;
	case P_PADOP: s = "P_PADOP"; break;
	case P_ACTION: s = "P_ACTION"; break;
	case P_ALARM: s = "P_ALARM"; break;
	case P_CYCLE: s = "P_CYCLE"; break;
	case P_DELETE: s = "P_DELETE"; break;
	case P_KBDSTR: s = "P_KBDSTR"; break;
	case P_LINE: s = "P_LINE"; break;
	case P_LINEREQ: s = "P_LINEREQ"; break;
	case P_CLEAR: s = "P_CLEAR"; break;
	case P_MAKECURRENT: s = "P_MAKECURRENT"; break;
	case P_MAKEGAP: s = "P_MAKEGAP"; break;
	case P_NEXTLINE: s = "P_NEXTLINE"; break;
	case P_NUMERIC: s = "P_NUMERIC"; break;
	case P_USERCLOSE: s = "P_USERCLOSE"; break;
	case P_CREATELINE: s = "P_CREATELINE"; break;
	case P_REMOVELINE: s = "P_REMOVELINE"; break;
	case P_HOSTSTATE: s = "P_HOSTSTATE"; break;
	case P_BUSY: s = "P_BUSY"; break;
	case P_IDLE: s = "P_IDLE"; break;
	case P_USERCUT: s = "P_USERCUT"; break;
	case P_PICK: s = "P_PICK"; break;
	case P_HELPSTR: s = "P_HELPSTR"; break;
	case P_SHELL: s = "P_SHELL"; break;
	case P_VERSION: s = "P_VERSION"; break;
	}
	return s;
}
#endif

void PadOp(Protocol op){
	static long LINEobj;
	register long obj;
	register Pad *p;
	register short t;

# ifdef PROTODEBUG
	dprint(1, "PadOp(%s)", OpToStr(op) );
# endif
	obj = op == P_NEXTLINE ? LINEobj : RcvLong();
	p = ObjToPad( obj );
	switch( (int) op ){
	case P_PADDEF:
		P_Define(p,obj);
		break;
	case P_ATTRIBUTE:
		P_Attributes(p);
		break;
	case P_REMOVELINE:
		P_RemoveLine(p);
		break;
	case P_TABS:
		P_Tabs(p);
		break;
	case P_BANNER:
		P_Banner(p);
		break;
	case P_CARTE:
		P_Carte(p);
		break;
	case P_LINES:
		P_Lines(p);
		break;
	case P_NAME:
		P_Name(p);
		break;
	case P_CLEAR:
		DelLines(p);
		Dirty(p);
		break;
	case P_MAKECURRENT:
		MakeCurrent(p);
		break;
	case P_LINE:
		LINEobj = obj;
	case P_NEXTLINE:
		PutLine(p,op);
		break;
	case P_CREATELINE:
		CreateLine(p);
		break;
	case P_DELETE:
		if( p ) p->attributes |= USERCLOSE;
		DeletePad(p);
		break;
	case P_MAKEGAP:
		MakeGap(p);
		break;
	case P_ALARM:
		t = RcvShort();
		if(p){
			if( !(p->ticks = t) ){
				HostParent = HostObject = p;
				ToHost( P_CYCLE, /* garbage */ 0 );
			}
		}
		break;
	default:
		ProtoErr( "PadOp(): " );
	}
# ifdef PROTODEBUG
	dprint(0, "PadOp(%s)", OpToStr(op) );
# endif
}

void PickOp(){
	register Pad *p;
	Index i;

	i = RcvShort();
	p = PickPad(PickPoint(IndexToStr(i)));
	MakeCurrent(p);
	while( butts ) waitMOUSE();
	if( p && (HostParent = HostObject = p) ){
		FlashBorder(p);
		Proto(P_PICK);
		HostAction( &i );
	}
}

Pad *PickPad(pt)
Point pt;
{
	register Pad *p;

	for( p = Sentinel.back; ISPAD(p); p = p->back )
		if( PadSized(p->rect) && ptinrect(pt,p->rect) )
			return p;
	return 0;
}

void DeletePick(){
	DeletePad(PickPad(PickPoint(0L)));
}

void DelLines( Pad *p ){
	if( !p ) return;
	while( ISLINE(p->sentinel.up,p) )
		DelLine( p->sentinel.up );
}

void DeletePad( Pad *p ){
	Rectangle r;
	register Line *l, *lu;

	if( !p ) return;
	if( DirtyPad == p ) Dirty((Pad*) 0);
	if( p->attributes&USERCLOSE ){
		HostParent = HostObject = p;
		ToHost( P_USERCLOSE, /* garbage */ 0 );
		if( p->attributes&DONT_CLOSE ) return;
		DelLines( p );
		Unlink( p );
		if( p->sentinel.text != NewString ) free( (uchar*)p->sentinel.text );
		if( p->name != NewString ) free( (uchar*)p->name );
		free( p );
	} else {
		if( p->attributes&DONT_CLOSE ) return;
		for( l = p->sentinel.up; ISLINE(l,p); l = lu ){
			lu = l->up;
			if( !(l->attributes&DONT_CUT) ) DelLine(l);
		}
	}
	if( Current == p ) SetCurrent((Pad *) 0);
	if( Selected.line && Selected.pad == p ) Selected.line = 0;
	r = p->rect;
	p->rect = ZRectangle;
	Refresh( r );
}

void Select( Line *l, Pad *p ){
	if( Selected.line == l ) return;
	if( Selected.line ){
		if( Selected.pad != p )
			Selected.pad->selkey = Selected.line->key;
		switch( (int) Covered(Selected.pad) ){
		case COMPLETE:
			break;
		case CLEAR:
			LineXOR(Selected.line, UNSELECT);
			break;
		case PARTIAL:
			Selected.line = 0;
			Refresh(Selected.pad->rect);
		}
	}
	if( Selected.pad = p )  p->selkey = 0;
	if( Selected.line = l ) LineXOR(l, SELECT);
}

void MakeCurrent( Pad *p ){
	int paint;
	Line *l;	

	if( !p ) return;
	if( Selected.line && Selected.pad!=p ) Select((Line*)0, (Pad*)0);
	paint = Covered(p) != CLEAR;
	if( !PadSized(p->rect) ){
		if( Selected.line ) Select((Line*)0, (Pad*)0);
		if( !PadSized(p->rect = clipgetrect(p->sentinel.text))) return;
		paint = 1;
	}
	if( p == Current ) return;
	FrontLink(p);
	if( paint ) Paint(p);
	l = &p->sentinel;
	if( p->selkey )	
		while( ISLINE(l->up, p) ){
			l = l->up;
			if( l->key == p->selkey ){
				Select(l, p);
				break;
			}
		}
}
	
void Relocate( Pad *p, Rectangle r ){
	Rectangle bounding;

	if( !PadSized(r) ) return;
	MakeCurrent(p);			/* bug - used to be FrontLink(p); */
	bounding = boundrect( r, p->rect );
	p->rect = r;
	Refresh( bounding );
}

void Move(){
	Pad *p = PickPad(PickPoint(0L));

	if( !p ) return;
	Relocate( p, moverect(p->rect, PadSpace) );
}

void Reshape(){
	register Pad *p = PickPad(PickPoint(0L));

	if( !p ) return;
	while( button123() ) waitMOUSE();
	Relocate( p, clipgetrect(0L) );
}
	
Rectangle NewSpace;
Point Scale( Point p ){
#define		fo PadSpace.min
#define		fc PadSpace.max
#define		to NewSpace.min
#define		tc NewSpace.max
#define SCALE(xy) p.xy = to.xy + muldiv( p.xy-fo.xy, tc.xy-to.xy, fc.xy-fo.xy );

	SCALE(x);
	SCALE(y);
	return p;
}

#define KBDLEN 90
char KBDStr[KBDLEN]=  "";

void PadClip(){
	Pad *p;

	KBDrect = NewSpace = screen->r;
	KBDrect.min.y = (NewSpace.max.y -= font->height+PADBORDER*2+2);
	KBDrect.max.y -= 1;
	KBDrect.min.x += 1;
	KBDrect.max.x -= 1;
	for( p = Sentinel.back; ISPAD(p); p = p->back ){
		p->rect.min = Scale( p->rect.min );
		p->rect.max = Scale( p->rect.max );
		if( !PadSized(p->rect) ){
			p->rect = ZRectangle;
			if( p == Selected.pad ){
				Selected.pad = 0;
				Selected.line = 0;
			}
		}
	}
	Refresh( PadSpace = NewSpace );
	PaintKBD();
}

#define TICKW	3

int	Tscale;
Image	*Tick;

void InitTick(){
	Image *b;
	int ht = font->height;
	Tscale = scalesize(display, 1);
	b = display->screenimage;
	Tick = allocimage(display, Rect(0,0,Tscale*TICKW,ht), b->chan, 0, DWhite); 
	/* background color */
	draw(Tick, Tick->r, kbdcols[BACK], nil, ZP);
	/* vertical line */
	draw(Tick, Rect(Tscale*(TICKW/2),0,Tscale*(TICKW/2+1),ht), display->black, nil, ZP);
	/* box on each end */
	draw(Tick, Rect(0,0,Tscale*TICKW,Tscale*TICKW), kbdcols[TEXT], nil, ZP);
	draw(Tick, Rect(0,ht-Tscale*TICKW,Tscale*TICKW,ht), kbdcols[TEXT], nil, ZP);
}

void DrawTick(Point pt){
	Rectangle r;

	if( Tick==0 ) InitTick();
	pt.x -= Tscale;
	r = Rect(pt.x, pt.y, pt.x+TICKW*Tscale, pt.y+font->height);
	if( r.max.x > r.max.x )
		r.max.x = r.max.x;
	draw(screen, r, Tick, nil, ZP);
}

Image	*Block = 0;

void InitBlock(){
	Image *b;
	int ht = font->height;
	b = display->screenimage;
	Block = allocimage(display, Rect(0,0,CHARWIDTH,ht), b->chan, 0, DWhite); 
	/* background color */
	draw(Block, Block->r, display->black, nil, ZP);
}

void DrawBlock(Point pt){
	Rectangle r;

	if( Block==0 ) InitBlock();
	r = Rect(pt.x, pt.y, pt.x+CHARWIDTH, pt.y+font->height);
	draw(screen, r, Block, nil, ZP);
}
		
void DrawKBD(char *s, int invert){
	int d = 0, borderwidth;
	Point pt;
	Image *back, *text, *bord;
	if( invert ){
		back = kbdcols[HIGH];
		text = kbdcols[HTEXT];
		bord = kbdcols[HIGH];
		borderwidth = PADBORDER;
	}else{
		back = kbdcols[BACK];
		text = kbdcols[TEXT];
		bord = kbdcols[BORD];
		borderwidth = PADBORDER-1;
	}
	draw(screen, KBDrect, back, nil, ZP);
	border(screen, KBDrect, borderwidth, bord, ZP);
	pt = addpt(KBDrect.min, Pt(PADBORDER+1,PADBORDER+1));
	if( *s ){
		string(screen, pt, text, pt, font, s);
		d = stringwidth(font, s);
	}
	if( !invert )
#ifdef TICK
		DrawTick(Pt(pt.x+d, pt.y));
#else
		DrawBlock(Pt(pt.x+d, pt.y));
#endif
}

void InvertKBDrect(char *s1, char *s2){
	char buf[257];

	strncpy( buf, s1, 128 );
	strncat( buf, s2, 128 );
	DrawKBD( buf, 1 );
}

void PaintKBD()	{ DrawKBD( KBDStr, 0 ); }	
	
#define PAD_TO_SH	(1L)
#define LINE_TO_SH	(2L)
#define LINE_TO_SEL	(3L)
#define LINE_TO_PAD	(4L)

void CarriageReturn(long obj){
	char *kbds = KBDStr;
	Line *l, *lsent;
	long ct;

	if( obj == LINE_TO_SH ){
		Proto(P_SHELL);
		SendLong(0L);			/* common protocol */
		SendShort(0);
		SendLong(0L);
		SendShort(0);
		SendString(kbds+1);
		SendLong(1L);
		SendString(Selected.line->text);
	} else if( obj == PAD_TO_SH ){
		Proto(P_SHELL);
		SendLong(0L);			/* common protocol */
		SendShort(0);
		SendLong(0L);
		SendShort(0);
		SendString(kbds+1);
		lsent = &Current->sentinel;
		ct = 0;
		for(l = lsent->down; l != lsent; l = l->down)
			++ct;
		SendLong(ct);
		for(l = lsent->down; l != lsent; l = l->down)
			SendString(l->text);
	} else {
		Proto(P_KBDSTR);
		SendLong(Current->object);
		SendShort(Current->oid);
		switch( obj ){
		case LINE_TO_SEL:
			SendLong(Selected.line->object);
			SendShort(Selected.line->oid);
			break;
		case LINE_TO_PAD:
			SendLong(Current->object);
			SendShort(Current->oid);
			break;
		}
		SendString(kbds);
	}
	kbds[0] = 000;
}

Rectangle bscreenrect(Rectangle *clipr){
	if(clipr) *clipr = screen->clipr;
	return screen->r;
}

int LayerReshaped(){
	extern int resized;
	if(resized){
		if(getwindow(display, Refnone) < 0){
			panic("pads: can't reattach to window");
			threadexits("resized");
		}
		screen->r = bscreenrect(&screen->clipr);
		PadClip();
		resized = 0;
		return 1;
	}
	return 0;
}

long LiveKBD(){
	Line *sel = Selected.line;
	Pad *cur = Current;

	if( KBDStr[0] == '>' && cur ){
		if( sel ){
			DoubleOutline(screen, sel->rect);
			return LINE_TO_SH;
		} else {
			HeavyBorder(cur);
			return PAD_TO_SH;
		}
	}
	if( sel && (sel->attributes&ACCEPT_KBD) ){
		DoubleOutline(screen, sel->rect);
		return LINE_TO_SEL;
	}
	if( cur && (cur->attributes&ACCEPT_KBD) ){
		HeavyBorder(cur);
		return LINE_TO_PAD;
	}
	return 0L;
}

#define CNTRL_U 025
void KBDAppend(int c){
	char *t;
	int len = strlen(t = KBDStr);

	if( c < 040 || (c&0200) || len >= KBDLEN ){
		if( c == CNTRL_U ) t[0] = 000;
		if( c != '\t' ) return;	/* bug: \t when len >= KBDLEN !! */
	}
	t[len] = c;
	t[len+1] = 000;
}

void MuxSnarf(){
	String hold;
	int i, c;

	hold.s = 0;
	hold.n = hold.size = 0;
	padsnarf(&hold);
	for( i = 0; i < hold.n; ++i ){
		c = hold.s[i];
		if( c == '\n' ) break;
		KBDAppend(c);
	}
}

#define KBD_PAUSE 4
#define ESCAPE 033
void KBDServe(){
	int c;
	char *t;
	int live, len;

	if( !(Pstate & (1<<RKeyboard)) ) return;
	live = LiveKBD();
	while( (c = kbdchar()) >= 0 ){
		if( c == ESCAPE ){
			MuxSnarf();
			break;
		}
		if( (c == '\r' || c == '\n') && live ){
			CarriageReturn(live);
			break;
		}
		len = strlen(t = KBDStr);
		if( c == '\b' ){
			t[len-1] = 000;
			continue;
		}
		KBDAppend(c);
	}
	PaintKBD();
	sleep( KBD_PAUSE );
	LiveKBD();
}

void FoldToggle(Attrib *a){
	*a &= ~(TRUNCATE|FOLD);
	*a |= NewFold;
	Paint(Current);
}

Entry *FoldEntry(Attrib *a){
	static Entry e = { 0, FoldToggle, 0 };

	assert(Current, "FoldEntry");
	if( ( (*a&(TRUNCATE|FOLD)) ? *a : Current->attributes )&TRUNCATE )
		e.text = "fold",	  NewFold = FOLD;
	else
		e.text = "truncate", NewFold = TRUNCATE;
	e.opand = (long) a;
	return &e;
}
