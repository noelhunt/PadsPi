#include "elf.h"
#include "dwarf.h"
#include "dwarfrec.h"
#include "dwarfabbrev.h"
#include <stdio.h>
#include <stdint.h>
SRCFILE("dwarfrec.c")

static char sccsid[ ] = "%W%\t%G%";

#define	NELEM(x)	(sizeof(x)/sizeof((x)[0]))

#ifndef offsetof
#define offsetof(s,m)	(ulong)(&(((s*)0)->m))
#endif

typedef signed char	schar;

static AParse plist[] = {
	A_SIBLING, OFFSET(sibling), TReference,
	A_LOCATION, OFFSET(location), TBlock|TConstant,
	A_NAME, OFFSET(name), TString,
	A_ORDERING, OFFSET(ordering), TConstant,
	A_BYTESIZE, OFFSET(bytesize), TConstant,
	A_BITOFFSET, OFFSET(bitoffset), TConstant,
	A_BITSIZE, OFFSET(bitsize), TConstant,
	A_STMTLIST, OFFSET(stmtlist), TConstant,
	A_LOWPC, OFFSET(lowpc), TAddress,
	A_HIGHPC, OFFSET(highpc), TAddress,
	A_LANGUAGE, OFFSET(language), TConstant,
	A_DISCR, OFFSET(discr), TReference,
	A_DISCRVAL, OFFSET(discrval), TBlock,
	A_VISIBILITY, OFFSET(visibility), TConstant,
	A_IMPORT, OFFSET(import), TReference,
	A_STRINGLEN, OFFSET(stringlen), TBlock|TConstant,
	A_COMMONREF, OFFSET(commonref), TReference,
	A_COMPDIR, OFFSET(compdir), TString,
	A_CONSTVALUE, OFFSET(constvalue), TString|TConstant|TBlock,
	A_CONTAININGTYPE, OFFSET(containingtype), TReference,
	A_DEFAULTVALUE, OFFSET(defaultvalue), TReference,
	A_INLINE, OFFSET(inlined), TConstant,
	A_ISOPTIONAL, OFFSET(isoptional), TFlag,
	A_LOWERBOUND, OFFSET(lowerbound), TConstant|TReference,
	A_PRODUCER, OFFSET(producer), TString,
	A_PROTOTYPED, OFFSET(prototyped), TFlag,
	A_RETURNADDR, OFFSET(returnaddr), TBlock|TConstant,
	A_STARTSCOPE, OFFSET(startscope), TConstant,
	A_STRIDESIZE, OFFSET(stridesize), TConstant,
	A_UPPERBOUND, OFFSET(upperbound), TConstant|TReference,
	A_ABSTRACTORIGIN, OFFSET(abstractorigin), TReference,
	A_ACCESSIBILITY, OFFSET(accessibility), TConstant,
	A_ADDRCLASS, OFFSET(addrclass), TConstant,
	A_ARTIFICIAL, OFFSET(artificial), TFlag,
	A_BASETYPES, OFFSET(basetypes), TReference,
	A_CALLING, OFFSET(calling), TConstant,
	A_COUNT, OFFSET(count), TConstant|TReference,
	A_DATAMEMBERLOC, OFFSET(datamemberloc), TBlock|TConstant|TReference,
	A_DECLCOL, OFFSET(declcol), TConstant,
	A_DECLFILE, OFFSET(declfile), TConstant,
	A_DECLLINE, OFFSET(declline), TConstant,
	A_DECLARATION, OFFSET(declaration), TFlag,
	A_DISCRLIST, OFFSET(discrlist), TBlock,
	A_ENCODING, OFFSET(encoding), TConstant,
	A_EXTERNAL, OFFSET(external), TFlag,
	A_FRAMEBASE, OFFSET(framebase), TBlock|TConstant,
	A_FRIEND, OFFSET(friend_r), TReference,
	A_IDENTCASE, OFFSET(identcase), TConstant,
	A_MACROINFO, OFFSET(macroinfo), TConstant,
	A_PRIORITY, OFFSET(priority), TReference,
	A_SEGMENT, OFFSET(segment), TBlock|TConstant,
	A_SPEC, OFFSET(spec), TReference,
	A_STATICLNK, OFFSET(staticlnk), TBlock|TConstant,
	A_TYPE, OFFSET(type), TReference,
	A_USELOC, OFFSET(uselocat), TBlock|TConstant,
	A_VARPARAM, OFFSET(varparam), TFlag,
	A_VIRTUALITY, OFFSET(virtuality), TConstant,
	A_VTABLEELEMLOC, OFFSET(vtableelemloc), TBlock|TReference,
	A_DATALOC, OFFSET(dataloc), TConstant,
	A_ENTRYPC, OFFSET(entrypc), TReference,
	A_RANGES, OFFSET(ranges), TReference,
	A_CALLCOLUMN, OFFSET(callcolumn), TConstant,
	A_CALLFILE, OFFSET(callfile), TConstant,
	A_CALLLINE, OFFSET(callline), TConstant,
	A_DECIMALSCALE, OFFSET(decimalscale), TConstant,
	A_DECIMALSIGN, OFFSET(decimalsign), TConstant,
	A_EXPLICIT, OFFSET(explicit_r), TFlag,
	A_OBJPTR, OFFSET(objptr), TReference,
	A_ELEMENTAL, OFFSET(elemental), TConstant,
	A_DATABITOFF, OFFSET(databitoff), TConstant,
	A_CONSTEXPR, OFFSET(cnstexpr), TFlag,
	A_SUNVTABLE, OFFSET(SUNvtable), TConstant,
	A_SUNCOMMANDLINE, OFFSET(SUNcommandline), TString,
	A_SUNVBASE, OFFSET(SUNvbase), TUndef,
	A_SUNCOMPILEOPTIONS, OFFSET(SUNcompileoptions), TString,
	A_SUNLANGUAGE, OFFSET(SUNlanguage), TConstant,
	A_SUNVTABLEABI, OFFSET(SUNvtableabi), TConstant,
	A_SUNFUNCOFFSETS, OFFSET(SUNfuncoffsets), TBlock,
	A_SUNCFKIND, OFFSET(SUNcfkind), TConstant,
	A_SUNORIGINALNAME, OFFSET(SUNoriginalname), TString,
	A_SUNPARTLINKNAME, OFFSET(SUNpartlinkname), TString,
	A_SUNLINKNAME, OFFSET(SUNlinkname), TString,
	A_SUNPASSWITHCONST, OFFSET(SUNpasswithconst), TFlag,
	A_SUNRETURNWITHCONST, OFFSET(SUNreturnwithconst), TFlag,
	A_SUNCOMDATFUNCTION, OFFSET(SUNcomdatfunction), TFlag
};

static AParse ptab[ATTRMAX];

DwarfRec::DwarfRec(const DwarfRec& rhs){
	b = rhs.b;
	_unit = rhs._unit;
	_uoff = rhs._uoff;
	_aoff = rhs._aoff;
	_depth = rhs._depth;
	allunits = rhs.allunits;
	nextunit = rhs.nextunit;
	tag = rhs.tag;
	children = rhs.children;
	attrs = rhs.attrs;
}

DwarfRec::DwarfRec(){
	b.bp = b.ep = 0;
	b.addrsz = 0;
	_unit = _uoff = _aoff = 0;
	_depth = 0;
	allunits = 0;
	nextunit = 0;
	children = 0;
	clear();
}

ulong DwarfRec::unit()	{ return _unit; }
uint  DwarfRec::uoff()	{ return _uoff; }
uint  DwarfRec::aoff()	{ return _aoff; }
int   DwarfRec::depth()	{ return _depth; }

void  DwarfRec::clear(){
	memset(&attrs, 0, sizeof(DwarfAttrs));
}

DwarfState DwarfRec::getstat(){
	DwarfState v;
	v.b = b;
	v.unit = _unit;
	v.uoff = _uoff;
	v.aoff = _aoff;
	v.allunits = allunits;
	v.nextunit = nextunit;
	v.depth = _depth;
	v.children = children;
	return v;
}

void DwarfRec::setstat(DwarfState v){
	b = v.b;
	_unit = v.unit;
	_uoff = v.uoff;
	_aoff = v.aoff;
	allunits = v.allunits;
	nextunit = v.nextunit;
	_depth = v.depth;
	children = v.children;
}

int Dwarf::startunit(DwarfRec& s){
	int i;
	ulong unit = s.nextunit;
	ulong aoff, len;

	if(unit >= sect[D_INFO].len){
		errstr("dwarf unit address 0x%lux >= 0x%lux out of range", unit, sect[D_INFO].len);
		return -1;
	}
	s.b.bp = sect[D_INFO].data + unit;
	s.b.ep = sect[D_INFO].data + sect[D_INFO].len;
	len = get4(&s.b);
	s.nextunit = unit + 4 + len;
	if(s.b.ep - s.b.bp < len){
BadHeader:
		errstr("bad dwarf unit header at unit 0x%lux", unit);
		return -1;
	}
	s.b.ep = s.b.bp+len;
	if((i=get2(&s.b)) != 2)
		goto BadHeader;
	aoff = get4(&s.b);
	s.b.addrsz = get1(&s.b);
	if(addrsz == 0)
		addrsz = s.b.addrsz;
	if(s.b.bp == 0)
		goto BadHeader;

	s._aoff = aoff;
	s._unit = unit;
	s._depth = 0;
	return 0;
}

int Dwarf::start(DwarfRec& s){
	s.nextunit = 0;
	if(startunit(s) < 0)
		return -1;
	s.allunits = 1;
	return 0;
}

int Dwarf::nextsym(DwarfRec& s){
	ulong num;
	DwarfAbbrev *a;

	if(s.children)
		s._depth++;
top:
	if(s.b.bp >= s.b.ep){
		if(s.allunits && s.nextunit < sect[D_INFO].len){
			if(startunit(s) < 0)
				return -1;
			s.allunits = 1;
			goto top;
		}
		return 0;
	}

	s._uoff = s.b.bp - (sect[D_INFO].data+s._unit);
	if((num = get128(&s.b)) == 0){
		if(s._depth == 0){
			if(s.allunits && s.nextunit < sect[D_INFO].len){
				if(startunit(s) < 0)
					return -1;
				s.allunits = 1;
				goto top;
			}
			return 0;
		}
		if(s._depth > 0)
			s._depth--;
		goto top;
	}
	if((a = abbrevs->getabbrev(s._aoff, num)) == 0){
		fprintf(stderr, "getabbrev %u %u for %u,%u: %s\n", s._aoff, num, s._unit, s._uoff, abbrevs->error());
		abort();
	}
	if(parseattrs(s, a) < 0)
		return -1;

	return 1;
}

int Dwarf::nextsymat(DwarfRec& s, int depth){
	int r;
	uint sib;

	if(s._depth == depth && s.attrs.have.sibling){
		sib = s.attrs.sibling;
		if(sib < sect[D_INFO].len && sect[D_INFO].data+sib >= s.b.bp)
			s.b.bp = sect[D_INFO].data+sib;
		s.children = 0;
	}

	/*
	 * The funny game with t and s make sure that if  we  get
	 * to  the end of a run of a particular depth, we leave s
	 * so that a call to nextsymat with depth-1 will actually
	 * produce  the  desired  entry.   We  could  change  the
	 * interface to nextrec  instead, but I'm scared to touch
	 * it.
	 */
	DwarfRec t(s);
	for(;;){
		if((r = nextsym(t)) != 1)
			return r;
		if(t._depth < depth)	/* went too far - nothing to see */
			return 0;

		s = t;
		if(t._depth == depth)
			return 1;
	}
}

int Dwarf::parseattrs(DwarfRec& r, DwarfAbbrev *a){
	DwarfBuf *b = &r.b;
	DwarfAttr *attr;
	DwarfAttrs *attrs = &r.attrs;
	int i, f, n, got;
	static int nbad;
	void *v;

	/* initialize ptab first time through for quick access */
	if(ptab[A_NAME].name != A_NAME)
		for(i=0; i<NELEM(plist); i++)
			ptab[plist[i].name] = plist[i];

	r.clear();
	r.tag = a->tag;
	r.children = a->children;
//	for( attr=a->lh.front; attr!=(DwarfAttr*)&a->lh; attr=attr->rsib ){
	for( attr = a->first(); attr; attr = a->next() ){
		n = attr->name();
		f = attr->form();
		if(n < 0 || n >= NELEM(ptab) || ptab[n].name==0){
			if(++nbad == 1)
				fprintf(stderr, "parseattrs: unexpected attribute name 0x%ux\n", n);
			return -1;
		}
		v = (char*)attrs + ptab[n].off;
		got = 0;
		if(f == FormIndirect)
			f = get128(b);
		if((ptab[n].type&(TConstant|TReference|TAddress))
		  && getulong(b, f, r._unit, (ulong*)v, &got) >= 0)
			;
		else if((ptab[n].type&TFlag) && getuchar(b, f, (uchar*)v) >= 0)
			got = TFlag;
		else if((ptab[n].type&TString) && getfstr(b, f, (char**)v) >= 0)
			got = TString;
		else if((ptab[n].type&TBlock) && getblock(b, f, (DwarfBlock*)v) >= 0)
			got = TBlock;
		else{
			if(skipform(b, f) < 0){
				if(++nbad == 1)
					fprintf(stderr, "dwarf parse attrs: cannot skip form %d\n", f);
				return -1;
			}
		}
		if(got == TBlock && (ptab[n].type&TConstant))
			got = constblock((DwarfBlock*)v, (ulong*)v);
		*((uchar*)attrs+ptab[n].haveoff) = got;
	}
	return 0;
}


int Dwarf::getulong(DwarfBuf *b, int form, ulong unit, ulong *u, int *type){
	static int nbad;
	uvlong uv;

	switch(form){
	default:
		return -1;

	/* addresses */
	case FormAddr:
		*type = TAddress;
		*u = getaddr(b);
		return 0;

	/* references */
	case FormRefAddr:
		/* absolute ref in .debug_info */
		*type = TReference;
		*u = getaddr(b);
		return 0;
	case FormRef1:
		*u = get1(b);
		goto relativeref;
	case FormRef2:
		*u = get2(b);
		goto relativeref;
	case FormRef4:
		*u = get4(b);
		goto relativeref;
	case FormRef8:
		*u = get8(b);
		goto relativeref;
	case FormRefUdata:
		*u = get128(b);
relativeref:
		*u += unit;
		*type = TReference;
		return 0;

	/* constants */
	case FormData1:
		*u = get1(b);
		goto constant;
	case FormData2:
		*u = get2(b);
		goto constant;
	case FormData4:
		*u = get4(b);
		goto constant;
	case FormData8:
		uv = get8(b);
		*u = uv;
		if(uv != *u && ++nbad == 1)
			fprintf(stderr, "dwarf: truncating 64-bit attribute constants\n");
		goto constant;
	case FormSdata:
		*u = get128s(b);
		goto constant;
	case FormUdata:
		*u = get128(b);
constant:
		*type = TConstant;
		return 0;
	}
}

int Dwarf::getuchar(DwarfBuf *b, int form, uchar *u){
	switch(form){
	default:
		return -1;

	case FormFlag:
		*u = get1(b);
		return 0;
	}
}

int Dwarf::getfstr(DwarfBuf *b, int form, char **s){
	static int nbad;
	ulong u;

	switch(form){
	default:
		return -1;

	case FormString:
		*s = getstring(b);
		return 0;

	case FormStrp:
		u = get4(b);
		if(u < sect[D_STRINGS].len)
			*s = (char*)sect[D_STRINGS].data + u;
		else{
			if(++nbad == 1)
				fprintf(stderr, "dwarf: bad string pointer 0x%lux in attribute\n", u);
			/* don't return error - maybe can proceed */
			*s = 0;
		}
		return 0;

	}
}

int Dwarf::getblock(DwarfBuf *b, int form, DwarfBlock *bl){
	ulong n;

	switch(form){
	default:
		return -1;
	case FormDwarfBlock:
		n = get128(b);
		goto copyn;
	case FormDwarfBlock1:
		n = get1(b);
		goto copyn;
	case FormDwarfBlock2:
		n = get2(b);
		goto copyn;
	case FormDwarfBlock4:
		n = get4(b);
copyn:
		bl->data = getnref(b, n);
		bl->len = n;
		if(bl->data == 0)
			return -1;
		return 0;
	}
}

int Dwarf::constblock(DwarfBlock *bl, ulong *pval){
	DwarfBuf b;

	memset(&b, 0, sizeof b);
	b.bp = bl->data;
	b.ep = bl->data+bl->len;

	switch(get1(&b)){
	case OpAddr:	*pval = getaddr(&b);		break;
	case OpConst1u:	*pval = get1(&b);		break;
	case OpConst1s:	*pval = (schar)get1(&b);	break;
	case OpConst2u:	*pval = get2(&b);		break;
	case OpConst2s:	*pval = (int16_t)get2(&b);	break;
	case OpConst4u:	*pval = get4(&b);		break;
	case OpConst4s:	*pval = (int32_t)get4(&b);	break;
	case OpConst8u:	*pval = (uint64_t)get8(&b);	break;
	case OpConst8s:	*pval = (int64_t)get8(&b);	break;
	case OpConstu:	*pval = get128(&b);		break;
	case OpConsts:	*pval = get128s(&b);		break;
	case OpPlusUconst: *pval = get128(&b);		break;
	default:
		return TBlock;
	}
	return TConstant;
}

/* last resort */
int Dwarf::skipform(DwarfBuf *b, int form){
	int type;
	DwarfVal val;

	if(getulong(b, form, 0, &val.c, &type) < 0
	  && getuchar(b, form, (uchar*)&val) < 0
	  && getfstr(b, form, &val.s) < 0
	  && getblock(b, form, &val.b) < 0)
		return -1;

	return 0;
}
