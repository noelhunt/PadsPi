#include "univ.h"

Line *Lsent;
Rectangle Trect;
Attrib Attributes;
int CharsWide;
Line FakeLine;
Image *Osbm;
char Tilde;
#ifdef SCROLL
static Image *Bkup;
#endif

void DoubleOutline( Image *b, Rectangle r ){
	extern Image *padcols[];
	border(b, r, Outlinewidth, padcols[BORD], ZPoint);
}

void HeavyBorder( Pad *p ){
	static int Bflag = 1;
	extern Image *padcols[];
	Image *mask = ( Bflag )? padcols[BORD]: padcols[BACK];
	border(screen, insetrect(p->rect,PADBORDER+1), Outlinewidth, mask, ZPoint);
	Bflag = 1 - Bflag;
}

Line *Linei(int i){
	Line *lsent = Lsent, *l;
	long ct = 1, k = lsent->key;
	Line fake;

	if( !i ) return lsent;
	for( l = lsent->down; l != lsent; l = l->down, ++ct )
		if( k ? (l->key==i) : (ct==i) )
			return l;
	if( k && i<=k ){
		FakeLine.key = i;
		*(FakeLine.text = itoa((int)i)) = Tilde;
		return &FakeLine;
	}
	return 0;
}

int Trunc(Line *l){
	int a;
	return (a=l->attributes)&(FOLD|TRUNCATE) ? a&TRUNCATE : Attributes&TRUNCATE;
}

int Expanded;
short Tabs = 8;
void Expand( char *blanks, char *tabs ){
	register int i, c;

	for( i = 0; (c = *tabs++) && i<250; )
		if( c == '\t' )
			do
				blanks[i++] = ' ';
			while( i % Tabs );
		else
			blanks[i++] = c;
	if( i==0 ) blanks[i++] = ' ';
	blanks[i] = '\0';
	Expanded = i;
}

int Needs(int i){
	Line *l = Linei(i);
	char blanks[1024];
	int j, k, Width = CharsWide;

	if( Trunc(l) ) return 1;
	Expand( blanks, l->text );
#ifndef TAC
	for( j = 0, k = Expanded-1; k>=0 ; j++ ){
		if( j == 1 ) --Width;
		k -= Width;
	}
	return j;
#else
	return 1 + (Expanded-1)/CharsWide;	/* doesn't take account of indent */
#endif
}

Point PaintLine(Image *b, Line *l, int code){
	int i, k, z;
	int restore = 0;
	Point pt;
	Image *text;
	Rectangle r;
	int TagWidth;
	int Width = CharsWide;
	char save, *terminate, blanks[512];

	pt = l->rect.min;
	z = pt.x;
	Expand( blanks, l->text );
	for( i = 0, k = 0; i < Expanded; i += Width ){
		if( i ){
			if( Trunc(l) ) break;
			pt.x += CHARWIDTH;
			if( !k ){ k++; Width--; }
		}
		r.min = pt;
		r.max.x = l->rect.max.x;
		r.max.y = pt.y + font->height;
		save = '\0';
		if( i+Width < Expanded ){
			save = *(terminate=blanks+i+Width);
			*terminate = '\0';
		}
		if( code == SELECT ){
			text = padcols[HTEXT];
			draw( b, r, padcols[HIGH], nil, r.min );
		}else{
			text = padcols[TEXT];
			if( code == BANNER )
				draw( b, r, tagcols[BACK], nil, r.min );
			if( code == UNSELECT )
				draw( b, r, padcols[BACK], nil, r.min );
		}
		string( b, pt, text, ZPoint, font, blanks+i );
		pt.x = z;
		l->rect.max.y = (pt.y += font->height);
		if( save ) *terminate = save;
	}
	return pt;
}

void PaintBanner( Image *b, int Wide ){
	int SaveWide = CharsWide;
	CharsWide = Wide;
	PaintLine( b, Linei(0), BANNER );
	CharsWide = SaveWide;
}

#define BARWIDTH 14

static Rectangle bar;

#ifndef TAGS
# define LO 0
#else
# define LO 1
#endif

int ClipPaint( Rectangle clip, Pad *p ){		/* nobody said it would be easy */
	int middle, lines, lo, hi, capacity;
	int change, i, selpainted;
	Line *lsent, *l;
	char save, *terminate, blanks[256];
	Point pt;
# ifdef TAGS
	Rectangle s;
	int TagWidth;
# endif

	Tabs = p->tabs;
	Attributes = p->attributes;
	Tilde = Attributes&NO_TILDE ? 0 : '~';
	Lsent = lsent = &p->sentinel;
	if( !PadSized(p->rect) ) return 1;
#ifdef TAGS
	p->brect = p->srect = Trect = insetrect( p->rect, PADBORDER+4 );
	TagWidth = CharsWide = Dx(p->brect) / CHARWIDTH;
	p->brect.min.x -= 1;
	p->brect.min.y -= 1;
	p->brect.max.y = p->brect.min.y + Needs(0)*font->height+1;
	Linei(0)->rect = Rpt(addpt(p->brect.min, Pt(1,0)), p->brect.max);
	p->srect.min.y = Trect.min.y = p->brect.max.y+1;
	p->srect.max.x = (Trect.min.x += BARWIDTH);
	Trect.min.x += 2;
	Trect.max.x = (p->brect.max.x += 1);
#else
	p->srect = Trect = insetrect( p->rect, PADBORDER+4 );
	p->srect.max.x = (Trect.min.x += BARWIDTH);
	Trect.min.x += 2;
#endif
	p->srect.min.x -= 1;
#ifdef SAMTERM
	p->srect.max.x -= 1;
#endif
	p->srect.min.y -= 1;
	p->srect.max.y += 1;

	capacity = Dy(Trect) / font->height;
	CharsWide = Dx(Trect) / CHARWIDTH - 1;

	middle = lines = 0;
	for( l = lsent->down; l != lsent; l = l->down ){
		++lines;
		if( l == Selected.line && rectinrect(p->rect,clip) )
			middle = lsent->key ? Selected.line->key : lines;
	}
	if( lsent->key ) lines = lsent->key;
	if( Scrolly ){
		Scrolly -= p->srect.min.y;
		middle = 1 +
			muldiv(Scrolly,lines,p->srect.max.y-p->srect.min.y);
		if( middle > lines ) middle = lines;
	}
#ifdef SCROLL
	p->lines = lines;
#endif
	if( !middle ){
		middle = (p->lo+p->hi)/2;
		if( middle<1 || middle>lines )
			middle = lsent->key ? 1 : lines;	/* top or bottom? */
	}
	lo = middle+1;
	hi = middle;
	do {
		int d;
		change = 0;
		if( lo>LO && capacity>=(d=Needs(lo-1)) ){
			capacity -= d, --lo;
			change = 1;
		}
		if( hi<lines && capacity>=(d=Needs(hi+1)) ){
			capacity -= d, ++hi;
			change = 1;
		}
	} while( change );
	if( hi>0 && lo>hi ){ Selected.line = 0; return 1; }
	if( Scrolly && lo==p->lo && hi==p->hi ) return 1;
	p->lo = lo;
	p->hi = hi;
	for( l = lsent->down; l != lsent; l = l->down )
		l->rect = ZRectangle;
	if( Osbm && !eqrect(Osbm->r,p->rect) && Osbm!=screen ){
		freeimage(Osbm);
		Osbm = 0;
	}
	if( !Osbm && !(Osbm=allocimage(display, p->rect, screen->chan, 0, 0)) )
		Osbm = screen;
	draw( Osbm, p->rect, display->white, nil, ZPoint );
	draw( Osbm, insetrect(p->rect, 1), padcols[BACK], nil, ZPoint );
	DoubleOutline( Osbm, insetrect(p->rect, PADBORDER-1) );
	if( p == Current ) DoubleOutline( Osbm, insetrect(p->rect,PADBORDER+1) );
# ifdef TAGS
	draw( Osbm, p->brect, tagcols[BACK], nil, ZPoint );
	PaintBanner( Osbm, TagWidth );
	pt = Pt(p->brect.min.x, p->brect.max.y);
	line( Osbm, pt, p->brect.max, Endsquare, Endsquare, 0, padcols[BORD], ZPoint );
# endif
# ifdef SAMTERM
	Point p0 = Pt(p->srect.max.x, p->srect.min.y);
	Point p1 = Pt(p->srect.max.x, p->srect.max.y);
	line( Osbm, p0, p1, Endsquare, Endsquare, 0, padcols[BORD], ZPoint );
	draw( Osbm, p->srect, padcols[BACK], nil, ZPoint );
	draw( Osbm, bar=scrollbar(p->srect,lo,hi+1,LO,lines+1), padcols[BORD], nil, ZPoint );
# endif
	draw( Osbm, bar=scrollbar(p->srect,lo,hi+1,LO,lines+1), padcols[BORD], nil, ZPoint );
# ifdef SCROLL
	p->bar = bar;
# endif
	pt = Trect.min;
# ifdef TAGS
	if( lo == 0 ) ++lo;	/* CAN'T HAPPEN */
# endif
	for( selpainted = 0; lo <= hi; ++lo ){
		l = Linei(lo);
		l->rect.min = pt;
		l->rect.max.x = Trect.max.x;
#ifdef BLANCHEDALMOND
		pt = PaintLine( Osbm,l,(lo==0)?BANNER:Selected.line==l?SELECT:NORMAL );
#else
		pt = PaintLine( Osbm,l, Selected.line==l? SELECT: NORMAL );
#endif
		if( Selected.line == l ) selpainted = 1;
	}
	if( !selpainted && Selected.pad == p ) Selected.line = 0;
	if( !Scrolly ) RequestLines(p);
	if( Osbm != screen ){
		Rectangle clipsrc;
		clipsrc = clip;
		rectclip(&clipsrc,Osbm->r);
		PadBlt( Osbm, clipsrc, p->front );
# ifdef HALFDUP
		flushimage(display, 1); sleep(300);
# endif
		return 1;
	}
	return 0;
}

void LineXOR( Line *l, int code ){ PaintLine( screen, l, code ); }

#define Rmin (r.min)
#define Rmax (r.max)
#define Pmin (p->rect.min)
#define Pmax (p->rect.max)

int SomeVis, SomeInvis;

void PadBlt(Image *b, Rectangle r, Pad *p){
	if( p == &Sentinel ){
		if( b != 0 )
			draw(screen, r, b, nil, r.min);
		SomeVis = 1;
		return;
	}
	if( !rectXrect( p->rect, r ) ){
		PadBlt( b, r, p->front );
		return;
	}
	if( Rmin.y < Pmin.y ){
		PadBlt( b, Rpt(Rmin, Pt(Rmax.x, Pmin.y) ), p->front );
		Rmin.y = Pmin.y;
	}
	if( Rmax.y > Pmax.y ){
		PadBlt( b, Rpt(Pt(Rmin.x, Pmax.y), Rmax ), p->front );
		Rmax.y = Pmax.y;
	}
	if( Rmin.x < Pmin.x ){
		PadBlt( b, Rpt(Rmin, Pt(Pmin.x, Rmax.y) ), p->front );
		Rmin.x = Pmin.x;
	}
	if( Rmax.x > Pmax.x ){
		PadBlt( b, Rpt(Pt(Pmax.x, Rmin.y), Rmax ), p->front );
		Rmax.x = Pmax.x;
	}
	SomeInvis = 1;
}

void Paint( Pad *p ){ ClipPaint(p->rect,p); }

void LineReq(Pad *p, long lo, long hi, int fake){
	Line *InsPos();

	if( lo == 0 ) lo = 1;
	if( !p || !p->object || hi < lo ) return;
	PutRemote( P_LINEREQ );
	SendLong( p->object ); SendShort(p->oid);
	SendLong( p->object ); SendShort(p->oid);
	SendLong( lo );
	SendLong( hi );
	if( fake )
		for( FakeLine.key = lo; FakeLine.key <= hi; ++FakeLine.key ){
			*(FakeLine.text = itoa((int)FakeLine.key)) = Tilde;
			InsAbove(InsPos(p,&FakeLine),&FakeLine);
		}
}
	
void CRequestLines( Pad *p ){
	Line *l;
	int reqhi = -1, reqlo = 0, k, i;

	for( i = p->lo; i <= p->hi; ++i ){
		l = Linei(i);
		k = l->key;
		if( l->attributes&FAKELINE ){
			l->attributes &= ~FAKELINE;
			if( k == reqhi+1 )
				reqhi = k;
			else {
				LineReq(p, reqlo, reqhi, 0);
				reqlo = reqhi = k;
			}
		}
	}
	LineReq(p, reqlo, reqhi, 0);
}

void RequestLines( Pad *p ){
	Line *lsent = Lsent, *l;
	int reqlo, cutlo, cuthi, k, cushion;

	if( !lsent || !p ) return;
	if( !lsent->key ){
		if( p->attributes&FAKELINE )
			CRequestLines(p);
		return;
	}
	cushion = Configuration&BIGMEMORY ? 1000 : 10;
	reqlo = p->lo;
	cutlo = p->lo - cushion;
	cuthi = p->hi + cushion;
	for( l = lsent->down; l != lsent; l = l->down ){
		k = l->key;
		if( k>=reqlo && k<=p->hi ){
			LineReq(p, reqlo, k-1, 1);
			reqlo = k+1;
		} else if( k<cutlo || k>cuthi )
			DelLine(l);
	}
	LineReq(p, reqlo, p->hi, 1);
		
}

#ifndef SCROLL
void Pointing(){
	Pad *p = PickPad(mouse->xy);
	Line *l, *ll, *lsent = &p->sentinel, *sel;
	int i, paint = 0;
	Point pt, ppt;
	extern Rectangle KBDrect;

	if( ptinrect(mouse->xy, KBDrect) ) return;
	if( !p ){
		Select( (Line*)0, (Pad*)0 );
		SetCurrent( (Pad *)0 );
		return;
	}
	if( p != Current ){
		MakeCurrent(p);
		return;
	}
	pt = mouse->xy;
	if( mouse->buttons&1 ){
		paint = 0;
		pt = dxordy(ppt=pt);
		if( ptinrect(pt,p->srect) ){
			if( !ptinrect(ppt,p->srect) && ptinrect(ppt,p->rect) ){
				pt.y = (bar.min.y+bar.max.y)/2;
				cursset( pt );
			}
			Scrolly = pt.y;
			Paint(p);
		} else {
			sel = 0;
			for( l = lsent->down, i = 1; l!=lsent ; l = l->down, ++i ){
				if( ptinrect( mouse->xy, l->rect ) ){
					sel = l;
					break;
				}
			}
			Select( sel, p);
			if( sel )
				if( p->sentinel.key
				 ? (sel->key == p->lo || sel->key == p->hi )
				 : (i == p->lo || i == p->hi) ){
					paint = 1;
					Scrolly = 0;
			}
		}
	}
	if( paint )
		Paint(p);
	if( Scrolly ){
		Scrolly = 0;
		RequestLines(p);
	}
}
#else
void Pointing(){
	Pad *p = PickPad(mouse->xy);
	Line *l, *ll, *lsent = &p->sentinel, *sel;
	int i, paint = 0;
	Point pt, ppt;
	extern Rectangle KBDrect;

	if( ptinrect(mouse->xy, KBDrect) ) return;
	if( !p ){
		Select( (Line*)0, (Pad*)0 );
		SetCurrent( (Pad *)0 );
		return;
	}
	if( p != Current ){
		MakeCurrent(p);
		return;
	}
	pt = mouse->xy;
	if( mouse->buttons&1 ){
		paint = 0;
		pt = mouse->xy;
		sel = 0;
		for( l = lsent->down, i = 1; l!=lsent ; l = l->down, ++i ){
			if( ptinrect( mouse->xy, l->rect ) ){
				sel = l;
				break;
			}
		}
		Select( sel, p);
		if( sel )
			if( p->sentinel.key
			 ? (sel->key == p->lo || sel->key == p->hi )
			 : (i == p->lo || i == p->hi) ){
				paint = 1;
		}
	}
	if( paint )
		Paint(p);
}

void scrtemp(void){
	int h = 2048;

	if(Bkup) return;
	Bkup = allocimage( display, Rect(0,0,32,h),screen->chan,0,0 );
	if( Bkup == 0 )
		abort();
}

int whichbutton(){
	static int which[]={0, 3, 2, 2, 1, 1, 2, 2, };
	return which[mouse->buttons&7];
}

void Doscroll(Pad *p){
	int lines, lo, delta;
	Line *l, *lsent = &p->sentinel;
	int but = whichbutton();
	int dy, y0, yscale;
	Rectangle r, s = ZRectangle;

	if ( but != 2 ) return;
	scrtemp();
	y0 = p->srect.min.y;
	yscale = p->srect.max.y - y0;
#ifdef JTOOLS
	if( lsent->key )
		lines = lsent->key;
	else
		for( l = lsent->down, lines = 0; l!=lsent ; l = l->down )
			lines++;
	lines++;		/* is p->lines sufficient? note 1 extra for sentinel */
#else
	lines = p->lines;
#endif
	delta = p->hi - p->lo + 1;
	draw(Bkup, Rect(0,0,Dx(p->srect),Dy(p->srect)), screen, nil, p->srect.min);
	while(button(but)){
		if( !ptinrect(mouse->xy, p->srect) )
			goto Jnap;
		dy = mouse->xy.y - y0;
		lo = muldiv(dy, lines, yscale) - (delta >> 1);
		if (lo <= 0)
			lo = 0;
		else if ((lo + delta) > lines)
			lo = lines - delta;
		r = scrollbar( p->srect, lo, lo+delta, 0, lines);
		if ( s.min.y != 0 && !eqrect( r, s ) )
			draw(screen, s, padcols[BORD], nil, ZP);
		s = r;
		draw(screen, r, padcols[HIGH], nil, ZP);
Jnap:
		waitMOUSE();
	}
	if( !ptinrect(mouse->xy,p->srect) )
		draw(screen, r, Bkup, nil, Pt(0, r.min.y-p->srect.min.y));
	else {
		Scrolly = mouse->xy.y;
		Paint(p);
		Scrolly = 0;
		RequestLines(p);
	}
}
#endif

# ifdef NOTDEF
void Doscroll(Pad *p){
	int but = whichbutton();
	int y0, y1;
	Point p0, p1;
	Rectangle r, r0, s = ZRectangle;

	if ( !button2() ) return;
	scrtemp();
	r = p->bar;
	p0 = mouse->xy;
	y0 = p->srect.min.y;
	y1 = p->srect.max.y;
	draw(Bkup, Rect(0,0,Dx(p->srect),Dy(p->srect)), screen, nil, p->srect.min);
	draw(screen, r, padcols[HIGH], nil, ZP);
	while(button(but)){
		if( !ptinrect(mouse->xy, p->srect) )
			goto Jnap;
		p1 = mouse->xy;
		if ( !eqpt(p0, p1) ) {
			s = rectaddpt( r, Pt(0,p1.y-p0.y) );
			if ( s.min.y >= y0 && y1 >= s.max.y ) {
				if ( p1.y < p0.y )
					r0 = Rpt( s.min, Pt(r.max.x, r.min.y) );
				else
					r0 = Rpt( Pt(r.min.x, r.max.y), s.max );
				draw(screen, r0, padcols[HIGH], nil, ZP);
				if( p1.y < p0.y )
					r0 = Rpt( Pt(s.min.x, s.max.y), r.max );
				else
					r0 = Rpt( r.min, Pt(s.max.x, s.min.y) );
				draw(screen, r0, Bkup, nil, Pt(0, r0.min.y-p->srect.min.y));
				r = s;
			}
			p0 = p1;
		}
Jnap:
		waitMOUSE();
	}
	if( !ptinrect(mouse->xy,p->srect) )
		draw(screen, r, Bkup, nil, Pt(0, r.min.y-p->srect.min.y));
	else {
		Scrolly = mouse->xy.y;
		Paint(p);
		Scrolly = 0;
		RequestLines(p);
	}
}
# endif
