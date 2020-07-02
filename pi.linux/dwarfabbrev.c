#include "elf.h"
#include "dwarf.h"
#include "dwarfabbrev.h"
#include <stdio.h>
SRCFILE("dwarfabbrev.c")

/*
 * Dwarf abbreviation parsing code.
 *
 * The convention here is that calling getabbrevs relinquishes access
 * to any abbrevs returned previously. Will have to add explicit ref-
 * erence counting if this turns out not to be acceptable.
 */

static char sccsid[ ] = "%W%\t%G%";

DwarfAttr::~DwarfAttr(){ }

DwarfAttr::DwarfAttr(ulong s, ulong f){
	_name = s;
	_form = f;
	rsib = 0;
}

ulong DwarfAttr::name(){ return _name; }
ulong DwarfAttr::form(){ return _form; }

DwarfAbbrev::~DwarfAbbrev(){ }

DwarfAbbrev::DwarfAbbrev(){
	aoff = num = tag = 0;
	children = nattr = 0;
	INITHEAD(attrlh, DwarfAttr);
	rsib = 0;
}

DwarfAttr *DwarfAbbrev::next(){
	if( ISATTR(ptr) ){
		DwarfAttr *p = ptr;
		ptr = ptr->rsib;
		return p;
	}
	return 0;
}

DwarfAttr *DwarfAbbrev::first(){
	ptr = attrlh.front;
	if( ISATTR(ptr) ){
		ptr = ptr->rsib;
		return attrlh.front;
	}
	return 0;
}

void DwarfAbbrev::linkin(DwarfAttr *attr){
	LINKIN(attrlh, DwarfAttr, attr);
}

void DwarfAbbrev::done(int o, int n, int t, int c, int na){
	aoff = o;
	num = n;
	tag = t;
	children = c;
	nattr = na;
	rsib = 0;
#ifdef DEBUG
	const char *nameof(int);
	printf("new abbrev aoff.%d num.%d tag.%s children.%d nattr.%d\n",aoff,num,nameof(tag),children,nattr);
#endif
}

void DwarfAbbrev::dump(){
	DwarfAttr *a;
	for( a = attrlh.front; a != (DwarfAttr*)&attrlh; a = a->rsib )
		printf("< %d, %d >\n", a->name(), a->form() );
}

DwarfAbbrevs::~DwarfAbbrevs(){ }

DwarfAbbrevs::DwarfAbbrevs(Dwarf *d){
	dwarf = d;
	size = 0;
	INITHEAD(abbrlh, DwarfAbbrev);
}

char *DwarfAbbrevs::parseabbrev(ulong off){
	int na, children;
	ulong num, tag, name, form;
	DwarfBuf b = {0,0,0};
	DwarfAttr *attr;
	DwarfAbbrev *abbr;
	int nabbrevs;

	if(off >= dwarf->sect[D_ABBREV].len)
		return sf("abbrev section offset 0x%lux >= 0x%lux\n",
			off, dwarf->sect[D_ABBREV].len);

	b.bp = dwarf->sect[D_ABBREV].data + off;
	b.ep = dwarf->sect[D_ABBREV].data + dwarf->sect[D_ABBREV].len;

	for(nabbrevs = 0;;){
		if(b.bp == 0)
			return sf("malformed abbrev data");

		if( (num = dwarf->get128(&b)) == 0 )
			break;
		tag = dwarf->get128(&b);
		children = dwarf->get1(&b);
		abbr = new DwarfAbbrev();
		for(na=0;; na++){
			name = dwarf->get128(&b);
			form = dwarf->get128(&b);
			if(name == 0 && form == 0)
				break;
			attr = new DwarfAttr(name, form);
			abbr->linkin( attr );
		}
		abbr->done( off, num, tag, children, na );
		LINKIN(abbrlh, DwarfAbbrev, abbr);
		nabbrevs++;
	}
	return 0;
}

DwarfAbbrev *DwarfAbbrevs::getabbrev(ulong off, ulong num){
	return findabbrev( off, num );
}

DwarfAbbrev *DwarfAbbrevs::findabbrev(ulong off, ulong num){
	int pass = 0;
	DwarfAbbrev *b;
Again:
	for( b = abbrlh.front; ISABBR(b); b = b->rsib )
		if( b->aoff == off && b->num == num )
			return b;

	if( pass++ > 0 ) return 0;		/* can this happen? */
	if( (_error = parseabbrev( off )) )
		return 0;

	goto Again;
}
