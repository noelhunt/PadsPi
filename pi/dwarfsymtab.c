#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include "format.h"
#include "core.h"
#include "symtab.h"
#include "dtype.h"
#include "symbol.h"
#include "elf.h"
#include "dwarf.h"
#include "dwarftype.h"
#include "dwarfsymtab.h"
#include "dwarfline.h"
#include "core.h"
SRCFILE("dwarfsymtab.c")

static char sccsid[ ] = "%W%\t%G%";

void SymbolStats();

DwarfSymTab::DwarfSymTab(Core* c, int fd, SymTab *i, long reloc)
 :SymTab(c, fd, i, reloc) {}

DwarfSymTab::~DwarfSymTab(){}

char *DwarfSymTab::gethdr(){
	e = new Elf();
	if( e->fdopen( fd ) < 0 )
		return e->perror();

	switch(e->encoding()){
	case ElfDataLsb: memlayout = LSBFIRST; break;
	case ElfDataMsb: memlayout = MSBFIRST; break;
	}
	dwarf = new Dwarf;
	if( dwarf->open(e) < 0 || dwarf->start(r) < 0 )
		return dwarf->error();

	entries = 1;
	return 0;
}

void DwarfSymTab::dosymtab(Var *glb, DwarfSource *src){
	int i;
	const char *name;
	Var *resolve;
	Func *f;
	DType *t;
	ElfSym s;
	Block *fake = fakeblk();

	for (i = 0; i < e->nsymtab; i++) {
		e->sym(i, &s);
		switch(s.type) {
		case STT_FUNC:
			name = s.name;
			if ((f = (Func*)idtosym(U_FUNC, name, 0))) {
				if (!f->range.lo)
					f->range.lo = s.value + relocation;
				if (!f->range.hi)
					f->range.hi = f->range.lo + s.size;
				break;
			}
			name = sf("%s", name);
			f = new Func(this, name, 0, 0);
			f->range.lo = s.value + relocation;
			f->range.hi = f->range.lo + s.size;
			f->_blk = fake;
			t = new DType;
			t->pcc = LONG;
			f->type = t->incref();
			f->type.pcc = FTN;
			break;

		case STT_OBJECT:
			if (s.bind != STB_GLOBAL && s.bind != STB_WEAK)
				break;
			name = s.name;
			resolve = (Var*)idtosym(U_GLB, name, 0);
			s.value += relocation;
			if (resolve) {
				if(!resolve->range.lo)
					resolve->range.lo = s.value;
			} else {
				name = sf("%s", name);
				glb = new Var(this, _blk, glb, U_GLB, name);
				if(!_blk->var )
					_blk->var = glb;
				glb->range.lo = s.value;
				glb->type.pcc = LONG;
			}
			break;
		}
	}
}

Source *DwarfSymTab::tree(){
	Var	*glb = 0;
	DwarfSource *lsrc = 0;
	DType	*dp;

	glb = globregs(_blk, _core->nregs());
	share = new DwarfTShare;
	dp = new DType();
	dp->pcc = VOID;
	share->entertype( 0, 0, dp );
	dwarfline = new DwarfLine(dwarf->sect[D_LINE].data,
		dwarf->sect[D_LINE].len, dwarf->addrsz, e->ehdr.encoding);
	while( dwarf->nextsymat(r, 0) == 1 )
		lsrc = (DwarfSource*) gathertypes(lsrc, &glb, 0);
	while( lsrc && lsrc->lsib )
		lsrc = (DwarfSource*)lsrc->lsib;
	dosymtab( glb, lsrc );
	return lsrc;
}

Source *DwarfSymTab::gathertypes(DwarfSource *lsrc, Var **glb, char *p){
	int		lo;
	int		depth = r.depth();
	DType		*t;
	Func		*func = 0;
	Var		*fst = 0, *v;
	char		*name;
	char		*opts[64];
	DwarfSource	*src = 0, *hsrc = 0;

	switch(r.tag){
	case T_COMPILEUNIT:
		assert(r.attrs.have.stmtlist != 0);
		dwarfline->newcu(r.unit(), r.attrs.stmtlist);
		lsrc = new DwarfSource(this,lsrc,r.attrs.name,0);
		hsrc = src = lsrc;
		src->dir = r.attrs.compdir;
		dwarftype = src->typeinfo = new DwarfType(src, share, dwarf);
#ifdef NOTDEF
		fstpre = 0;
		if(r.attrs.have.SUNcompileoptions){
			int nopt;
			setfields( ";" );
			nopt = getfields(r.attrs.SUNcompileoptions, opts, 64);
			for(int i=0; i<nopt; ++i)
				if( !strncmp(opts[i], "G=", 2) )
					fstpre = StrDup(opts[i]+2);
		}
#endif
		fst = 0;

		/* RECURSE */

	case T_LEXICALBLK:
Recurse:
		while(dwarf->nextsymat(r, depth+1) == 1)
			lsrc = (DwarfSource*) gathertypes(lsrc, glb, p);
		break;

	case T_SUBPROGRAM:
		if( r.attrs.have.declaration )
			break;

		++FunctionStubs;
		name = r.attrs.name;
		dwarfline->bounds( r.unit() );
		lo = dwarfline->findlo( r.attrs.lowpc );
		func = new Func( this,name,lsrc,lo );
		func->namewithargs = name;
		func->range.lo = r.attrs.lowpc;
		func->range.hi = r.attrs.highpc;
		func->lines.hi = dwarfline->findhi( r.attrs.highpc );
		t = new DType( dwarftype->gettype(r.attrs.type) );
		func->type = t->incref();
		func->type.pcc = FTN;
		func->save = r.getstat();
		goto Recurse;

	case T_ARRAYTYPE:
	case T_PTRTYPE:
	case T_REFTYPE:
	case T_SUBRTYPE:
	case T_TYPEDEF:
	case T_SUBRANGETYPE:
	case T_BASETYPE:
	case T_CONSTTYPE:
	case T_VOLATILE:
	case T_RESTRICT:
		dwarftype->maketype( r );
		break;

	case T_CLASSTYPE:
	case T_ENUMTYPE:
	case T_STRUCTTYPE:
	case T_UNIONTYPE:
		dwarftype->maketype( r );
		goto Recurse;

	case T_VARIABLE:
		if( !r.attrs.have.visibility )
			break;

		switch( r.attrs.visibility ){
		case V_EXPORTED:		/* need to test external? */
			if( !idtosym(U_GLB, r.attrs.name, 0) )
				gathervar( r, glb, _blk, U_GLB, dwarftype );
			break;
		case V_QUALIFIED:
			if( lsrc )
				gathervar( r, &fst, lsrc->blk, U_FST, dwarftype );
		}
		break;
	}
	return lsrc;
}

Block *DwarfSymTab::gatherfunc(Func *func){
	int depth;
	Pclu *l;
	DType *t;
	DwarfRec rec;
	Block *ablk, *lblk;
	Var *arg = 0, *lcl = 0;
	Stmt *stmt = 0;
	DwarfSource *src = (DwarfSource *)func->typeinfo;

	++FunctionGathered;
	SymbolStats();
	rec.setstat( func->save );
	depth = rec.depth();
	ablk = new Block( this, 0, 0, sf("%s().arg_blk",rec.attrs.name) );
	lblk = new Block( this, ablk, 0, sf("%s().lcl_blk",rec.attrs.name) );
	ablk->child = lblk;
	ablk->range.hi = ablk->range.lo = func->range.lo;
	while(dwarf->nextsymat(rec, depth+1) == 1){
		switch( rec.tag ){
		case T_FORMALPARAM:
			gathervar( rec, &arg, ablk, U_ARG, dwarftype );
			if (arg->type.isstrun()) {
				t = new DType(arg->type);
				arg->type = t->incref();
			}
			break;
	
		case T_VARIABLE:
			if( rec.attrs.visibility == V_LOCAL )
				gathervar( rec, &lcl, lblk, U_AUT, dwarftype );
			else if( rec.attrs.visibility == V_QUALIFIED )
				gathervar( rec, &lcl, lblk, U_STA, dwarftype );
			break;
		}
	}
	dwarfline->bounds(rec.unit(), func->range.lo, func->range.hi);
	while((l = dwarfline->sline())){
		if (stmt)
			stmt->range.hi = l->pc;
		stmt = new Stmt(this,lblk,stmt);
		if( !ablk->stmt ) ablk->stmt = stmt;
		stmt->lineno = l->line;
		stmt->range.lo = l->pc;
	}
	if (func->range.hi) {
		if (stmt) stmt->range.hi = func->range.hi;
		ablk->range.hi = func->range.hi;
	} else if (stmt) {
		stmt->range.hi = stmt->range.lo + 20;
		ablk->range.hi = stmt->range.lo;
	}

	return ablk;
}

void DwarfSymTab::gathervar(DwarfRec& s, Var **v, Block *b, UDisc d, DwarfType *dt){
	DType t;
	IF_LIVE( !v ) return;
	*v = new Var( this, b, *v, d, s.attrs.name );
	if( b && !b->var ) b->var = *v;
	t = dt->gettype( s.attrs.type );
	(*v)->range.lo = dwarf->location(s)->lng;
	(*v)->type = t;
}

Var *DwarfSymTab::gatherutype(UType *ut){
	DwarfUType *u = (DwarfUType*)ut;
	DwarfType *typeinfo = ((DwarfSource *)u->src)->typeinfo;
	DwarfRec s;
	Var *first = 0, *v = 0;
	char *name;
	int bitsize, bitoffset;
	int n, depth;
	int isenum = (u->type.pcc == ENUMTY);

	++UTypeGathered;
	SymbolStats();
	s.setstat( u->save );
	depth = s.depth();
	while(dwarf->nextsymat(s, depth+1) == 1){
		if((!isenum && (s.tag != T_MEMBER     || !s.attrs.have.type))
		|| ( isenum && (s.tag != T_ENUMERATOR || !s.attrs.have.constvalue)) )
				continue;
		name = s.attrs.name;
		if( isenum ){
			v = new Var( this, 0, v, U_MOT, name );
			v->range.lo = VALOF( s.attrs.have.constvalue,
						&s.attrs.constvalue );
			v->type.pcc = MOETY;
			if( !first ) first = v;
			continue;
		}
		bitsize = 0;
		if( u->type.pcc == UNIONTY )
			bitoffset = 0;
		else if( s.attrs.have.bitoffset
		&&	 s.attrs.have.bitsize
		&&	 s.attrs.have.bytesize ){
			bitsize = s.attrs.bitsize;
			switch( dwarf->memlayout() ){
			case MSBFIRST:
				bitoffset = s.attrs.bitoffset;
				break;
	
			case LSBFIRST:
				if(s.attrs.bytesize == 0){
					PadsWarn("integral type bit field botch");
					break;
				}
				bitoffset =
				  8*s.attrs.bytesize-s.attrs.bitoffset-s.attrs.bitsize;
				break;
	
			default:
				abort();
			}
		}else
			bitoffset = 8*VALOF(s.attrs.have.datamemberloc, &s.attrs.datamemberloc);
		v = new Var( this, 0, v, U_MOT, name );
		v->type = typeinfo->gettype( s.attrs.type );
		if((bitsize & 0x7) || (bitoffset & 0x7)){
			if( memlayout == MSBFIRST )
				v->range.lo = ((bitoffset >> 5) << 5) +
					      32 - bitsize - (bitoffset & 0x1f);
			
			else
				v->range.lo = bitoffset;
			v->type.pcc = UBITS;
			v->type.dim = bitsize;
		}else
			v->range.lo = bitoffset >> 3;
		if( !first ) first = v;
	}
	return first;
}

DwarfSource::DwarfSource(SymTab *stab, Source *left, char *t, long c)
 :Source(stab, left, t, c)	{}

DwarfSource::~DwarfSource()	{ delete typeinfo; }

DwarfUType::DwarfUType(SymTab *stab, DwarfRec& r, char *id)
 :UType(stab, 0, 0, id){
	save = r.getstat();
}

DwarfUType::~DwarfUType(){
	delete [] name;
}
