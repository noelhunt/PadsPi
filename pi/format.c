#include "univ.h"
#include "format.h"
#include <ctype.h>
#include <stdarg.h>
#include "symtab.h"
#include "symbol.h"
SRCFILE("format.c")

static char sccsid[ ] = "%W%\t%G%";

char *Bls::af(const char *fmt, ...){
	va_list v;
	trace( "%d.af(%s ...) %s", this, fmt, text );
	IF_LIVE(!this) return (char*)"Bls::af";
	va_start(v, fmt);
	vf(fmt, v);
	va_end(v);
	return text;
}

void Bls::vf(const char *fmt, va_list v){
	char x[1024], *q = x;

	trace( "%d.vf(%s ...) %s", this, fmt, text );
	if( p<text+TBLS && fmt ){
		vsprintf(x, fmt, v);
		while( *q && p<text+TBLS ) *p++ = *q++;
		*p = 0;
	}
}

Bls::Bls(const char *fmt, ...){
	va_list v;
	clear();
	va_start(v, fmt);
	vf(fmt, v);
	va_end(v);
}

Bls::Bls(Bls &b){
	clear();
	af("%s", b.text);
}

Format::Format(long f, SymTab *s)	{ format = f; stab = s; }

const char *FmtByte(int c){	static char buf[8];

	switch( c &= 0xFF ){
	case '\0'	: return "\\0";	
	case '\b'	: return "\\b";	
	case '\f'	: return "\\f";	
	case '\n'	: return "\\n";	
	case '\r'	: return "\\r";	
	case '\t'	: return "\\t";	
	case '\v'	: return "\\v";	
	case ' '	: return " ";		/* see ikeya!<ctype.h> */	
	case '\''	: return "\\\'";
	case '\"'	: return "\\\"";
	case '\\'	: return "\\\\";
	}
	sprintf( buf, isascii(c)&&isprint(c)?"%c":"\\%03o", c );
 	return (const char*)buf;
}

const char *FmtAscii(unsigned long c){
	static char buf[32];

	if( c == 0 ) return "0";
	strcpy(buf, "'");
	int lead = 1; int byte = 4;			// cfront bug
	for( ; byte; --byte, c<<=8 ){
		if( (c&0xFF000000) || !lead ){
			strcat(buf, FmtByte(int(c>>24)));
			lead = 0;
		}
	}
	return (const char*)strcat(buf, "'");
}

void Format::grow(const char *b){
	accum.af("%s%s", sep, b);
	sep = "=";
}

void Format::growtime(time_t t){
	grow(ctime(&t));
}
	
void Format::grow(const char *fmt, long l){
	char buf[32];

	sprintf(buf, fmt, l);
	grow(buf);
}

void Format::grow(const char *fmt, double d){
	char buf[32];

	trace( "%d.grow(%s,%g)", this, fmt, d );
	sprintf(buf, fmt, d);
	if( fmt[1]=='g' && !strcmp(buf, "0") )
		grow("0.0");
	else
		grow(buf);
}

const char *Format::f(long lng, double dbl){
	Symbol *s;

	if( !this ) return "Format::f";

	accum.clear();
	sep = "";

	if( !format ) format = F_HEX;

	if( format&F_MASK8  ) lng &= 0xFF;
	if( format&F_MASK16 ) lng &= 0xFFFF;

	if( format&F_FLOAT  ) grow("%.6g",    dbl);
	if( format&F_DOUBLE ) grow("%.15g",   dbl);
	if( format&F_DBLHEX ) grow("0x%X_%X", dbl);

	if( format&F_SYMBOLIC
	 && lng
	 && stab
	 && (s = stab->loctosym(SSet(U_FUNC,U_GLB,U_STA,U_STMT), lng)) ){
		lng -= s->range.lo;
		grow(s->text());
		if( !lng )
			format = 0;
		else {
			sep =  "+";
			if( !(format&F_HOAD) ) format |= F_HEX;
		}
	}

	if( format&F_TIME ) growtime(lng);
	if( (format&F_HOAD) && lng>=0 && lng<=7 ) grow("%d", lng);
	else {
		if( format&F_OCTAL   ) grow("0%o",  lng);
		if( format&F_DECIMAL ) grow("%u",   lng);
		if( format&F_HEX     ) grow("0x%X", lng);
		if( format&F_ASCII   ) grow(FmtAscii(lng));

		if( format&F_EXT8 ) lng = (char ) lng;
		if( format&F_EXT16) lng = (short) lng;

		if( format&F_SIGNED
		 && (lng<0 || !(format&F_DECIMAL)) ) grow("%d", lng);
	}
	return accum.text;
}
