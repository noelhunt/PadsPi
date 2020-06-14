#include <ctype.h>
#include <demangle.h>
#include "mip.h"
#include "elf.h"
#include "dwarf.h"
#include "format.h"
#include "symtab.h"
#include "symbol.h"
#include "dwarftype.h"
#include "dwarfsymtab.h"
SRCFILE("dwarftype.c")

static char sccsid[ ] = "%W%\t%G%";

Entry::Entry(ulong n, DType *d){
	addr = n;
	type = *d;
	rsib = 0;
};

DwarfType::DwarfType(Source *sp, DwarfTShare *sh, Dwarf *d){
	share = sh;
	src = sp;
	dwarf = d;
	used = 1;
}

DwarfType::~DwarfType(void){}

#ifdef DEMANGLE
const char *DwarfType::demangle(const char *name, char *buf2){
	cplus_demangle(name, buf2, MAXLINE);
	switch( cplus_demangle(name, buf2, MAXLINE) ){
	case 0:
		return buf2;
	case DEMANGLE_ENAME:
	case DEMANGLE_ESPACE:
		return name;
	}
}
#endif

struct typemap {
	const char *name;
	int type;
};

void DwarfType::maketype(DwarfRec& r){
	int n;
	int depth = r.depth();
	char *name;
	DType *dp, *dp1;
	DwarfUType *u;
	ulong unit, uoff;
	typemap *tm;
	static typemap typemap[] = {
		"char",			CHAR,
		"short",		SHORT,
		"int",			INT,
		"long",			LONG,
		"long long",		LONG,
		"signed char",		CHAR,
		"signed short",		SHORT,
		"signed int",		INT,
		"signed long",		LONG,
		"signed long long",	LONG,
		"unsigned char",	UCHAR,
		"unsigned short",	USHORT,
		"unsigned int",		UNSIGNED,
		"unsigned long",	ULONG,
		"unsigned long long",	ULONG,
		"float",		FLOAT,
		"double",		DOUBLE,
		"long double",		DOUBLE,
		"void",			VOID,
		"long int",		LONG,
		"long unsigned int",	ULONG,
		"short int",		SHORT,
		"long long int",	LONG,
		"short unsigned int",	USHORT,
		"long long unsigned int", ULONG,
		0,			0
	};

	switch( r.tag ){
	case T_TYPEDEF:
		dp = new DType;
		if( !r.attrs.have.type )
			dp->pcc = VOID;
		else {
			dp1 = share->findtype( r.attrs.type );
			if( !dp1 ){
				dp->over = r.attrs.type;
				dp->pcc = TYD;
			} else {
				dp->pcc = dp1->pcc;
				dp->dim = dp1->dim;
				dp->univ = dp1->univ;
			}
		}
		share->entertype( r.unit(), r.uoff() , dp);
		break;

	case T_PTRTYPE:
	case T_CONSTTYPE:
		dp = new DType;
		dp->univ = share->findtype( r.attrs.type );
		if (!dp->univ) dp->over = r.attrs.type;
		dp->pcc = ( r.tag == T_CONSTTYPE ) ? CNST : PTR;
		share->entertype( r.unit(), r.uoff(), dp );
		break;

	case T_BASETYPE:
		name = r.attrs.name;
		switch(r.attrs.encoding){
		default:
		case TypeAddress:
			break;
		case TypeBoolean:
		case TypeUnsigned:
		case TypeSigned:
		case TypeSignedChar:
		case TypeUnsignedChar:
			break;
		case TypeFloat:
			break;
		case TypeComplexFloat:
			break;
		case TypeImaginaryFloat:
			break;
		}
		for( tm = typemap; tm->name; tm++ )
			if ( !strcmp(name, tm->name) )
				break;
		dp = new DType;
		if( !tm->name )
			dp->pcc = INT;
		else
			dp->pcc = tm->type;
		share->entertype( r.unit(), r.uoff(), dp );
		break;

	case T_STRUCTTYPE:
	case T_UNIONTYPE:
	case T_ENUMTYPE:
		if(r.attrs.have.name)
			name = r.attrs.name;
		else
			name = sf("anon_%d", ++gen);
		u = new DwarfUType(src->symtab, r, name);
		u->src = src;
		u->range.lo = r.attrs.bytesize;
		switch( r.tag ) {
		case T_ENUMTYPE:
			u->type.pcc = ENUMTY;
			break;
		case T_STRUCTTYPE:
			u->type.pcc = STRTY;
			break;
		case T_UNIONTYPE:
			u->type.pcc = UNIONTY;
			break;
		}
		u->type.univ = (DType*)u;
		u->rsib = src->symtab->utype;
		src->symtab->utype = u;
		if( !share->findtype(r.unit(),r.uoff()) ){
			dp = new DType;
			dp->univ = (DType*)u;
			dp->pcc = u->type.pcc;
			share->entertype( r.unit(), r.uoff(), dp );
		}
		break;

	case T_ARRAYTYPE:
		dp = new DType;
		dp->pcc = ARY;
		dp->univ = share->findtype( r.attrs.type );
		unit = r.unit(), uoff = r.uoff();
		while(dwarf->nextsymat(r, depth+1) == 1){
			if(r.tag != T_SUBRANGETYPE)
				continue;
			dp->dim = r.attrs.upperbound;
		}
		dp->dim++;
		share->entertype( unit, uoff, dp);
		break;

	case T_SUBRTYPE:
		break;
	}
}

DType DwarfType::gettype(ulong addr){
	return chain( share->findtype( addr ) );
}

DType DwarfType::chain(DType *dp){
	DType d;

	if (!dp) {		// Shouldn't happen - fix this if ever figure
		d.pcc = INT;	// out how the include file references are
		return d;	// shared. I give up ....
	}
	d.pcc = dp->pcc;
	d.dim = dp->dim;
	d.univ = dp->univ;
	if( dp->pcc & TMASK ){
		if (!dp->univ)
			dp->univ = share->findtype( dp->over );
		d.univ = new DType;
		*(d.ref()) = chain(dp->ref());
	}
	return d;
}

#ifdef HASHTYPES
DwarfTShare::~DwarfTShare(){
	delete [] hashtab;
}

DwarfTShare::DwarfTShare(){
	hashsize = MAGIC2;
	for (int i = 0 ; i < hashsize ; i ++)
		hashtab[i] = 0 ;
}

void DwarfTShare::entertype(ulong n1, ulong n2, DType *d){
	Entry *entry;
	ulong addr = n1 + n2;
	int i = hashloc (addr) ;
	for(entry = hashtab[i]; entry; entry = entry->rsib)
		if (addr == entry->addr)
			return ;
#ifdef HASHSTATS
	extern int TypeGathered, HashStats[];
	++HashStats[i];
	++TypeGathered;
#endif
	entry = new Entry(addr , d) ;
	entry->rsib = hashtab[i];
	hashtab[i] = entry; 
}

DType *DwarfTShare::findtype(ulong n1, ulong n2){
	Entry *entry; 
	int i = hashloc (n1 + n2) ; 
	for(entry = hashtab[i]; entry; entry = entry->rsib)
		if((n1 + n2) == entry->addr)
			return entry->dtype(); 
	return 0; 
}

int DwarfTShare::hashloc(ulong addr){
	return (addr*MAGIC1) % MAGIC2;
}
#else
DwarfTShare::~DwarfTShare(){ }

DwarfTShare::DwarfTShare(){
	list = new Skiplist( 0.5 );
}

void DwarfTShare::entertype(ulong n1, ulong n2, DType *d){
	ulong addr = n1 + n2;
	if( list->search( addr ) )
		return;
	list->insert(addr, d);
}

DType *DwarfTShare::findtype(ulong n1, ulong n2){
	DType *d;
	ulong addr = n1 + n2;
	if( (d = (DType*)list->search( addr )) )
		return d;
	return 0; 
}
#endif