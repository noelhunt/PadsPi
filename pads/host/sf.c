#include <pads.h>
#include <stdarg.h>

#define SF_HASH 823
struct SF_CELL {
	SF_CELL *link;
	char     buf[1];
};
static int Calls, Strings, Worst, Bytes;

char *sf(const char *fmt, ...){
	if( !fmt ){
		static char report[128];
		sprintf( report, "strings=%d calls=%d worst=%d bytes=%d",
				  Strings,   Calls,   Worst,   Bytes );
		return report;
	}
	va_list ap;
	va_start(ap, fmt);
	char *r = vf( fmt, ap );
	va_end(ap);
	return r;
}

char *vf(const char *fmt, va_list va){
	char x[1024], *p;
	unsigned long len, h, i;
	struct SF_CELL *s;
	static SF_CELL *Table[SF_HASH];
	char *StrDup(const char*);
	++Calls;		
	vsprintf( x, fmt, va );
	h = 0;
	for( len = 0, p = x; *p; )
		h += (*p++) << (++len%4);
	h %= SF_HASH;
	for( s=Table[h],i=1; s; s=s->link,++i )
		if(!strcmp(x,s->buf))
			return s->buf;
	++Strings;
	if( i>Worst )
		Worst = (int)i;
	len = (len+4+sizeof(SF_CELL*)) / 4 * 4;		/* vax */
	s = (SF_CELL*) new char [len];
	if( !s ) return StrDup("sf(): out of memory");
	Bytes += (int)len;
	s->link = Table[h];
	Table[h] = s;
	strcpy( s->buf, x );
	return s->buf;
}
