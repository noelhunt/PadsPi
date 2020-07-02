#include "elf.h"
#include "dwarf.h"
#include "symbol.h"
#include "dwarfline.h"
#include <assert.h>
SRCFILE("dwarfline.c")

static char sccsid[ ] = "%W%\t%G%";

#define CAPACITY 1024

DwarfLine::DwarfLine(uchar *data, ulong len, int size, uint memlayout){
	_size = 0;
	_capacity = CAPACITY;
	table = new Pclu[CAPACITY];
	base = data;
	blen = len;
	addrsz = size;
	lsb = memlayout == ElfDataLsb;
}

DwarfLine::~DwarfLine(){}

void DwarfLine::grow() {
	Pclu* ptr = new Pclu[2*_capacity];
	for (int i=0; i<_capacity; i++)		// memcpy(p,ptr,sizeof(Llist)*_capacity)
		ptr[i] = table[i];
	delete [] table;
	table = ptr;
	_capacity *= 2;
}

void DwarfLine::push(Pclu& v) {
	while (_size >= _capacity)
		grow();
	table[_size++] = v;
}

/* Skip over a LEB128 value(signed or unsigned). */

void DwarfLine::skipleb128(LineData *leb) {
	while(leb->cpos != leb->end && *leb->cpos >= 0x80)
		leb->cpos++;
	if(leb->cpos != leb->end)
		leb->cpos++;
}

/* Read a ULEB128 into a 64-bit word.  Return(uint64_t)-1 on overflow
   or error.  On overflow, skip past the rest of the uleb128. */

uint64_t DwarfLine::readuleb128(LineData *leb) {
	uint64_t result = 0;
	int bit = 0;

	do  {
		uint64_t b;

		if(leb->cpos == leb->end)
			return(uint64_t) -1;

		b = *leb->cpos & 0x7f;

		if(bit >= 64 || b << bit >> bit != b)
			result =(uint64_t) -1;
		else
			result |= b << bit, bit += 7;
	} while(*leb->cpos++ >= 0x80);
	return result;
}

/* Read a SLEB128 into a 64-bit word.  Return 0 on overflow or error
  (which is not very helpful).  On overflow, skip past the rest of
   the SLEB128.  For negative numbers, this actually overflows when
   under -2^62, but since this is used for line numbers that ought to
   be OK... */

int64_t DwarfLine::readsleb128(LineData * leb) {
	const uint8_t * start_pos = leb->cpos;
	uint64_t v = readuleb128(leb);
	uint64_t signbit;

	if(v >= 1ull << 63)
		return 0;
	if(leb->cpos - start_pos > 9)
		return v;

	signbit = 1ull <<((leb->cpos - start_pos) * 7 - 1);

	return v | -(v & signbit);
}

/* Free a LineData structure. */

void DwarfLine::dealloc(LineData *lnd) {
	if(!lnd)
		return;
	if(lnd->dirnames)
		free(lnd->dirnames);
	if(lnd->filenames)
		free(lnd->filenames);
	free(lnd);
}

/* Return the pathname of the file in S, or NULL on error. 
   The result will have been allocated with malloc. */

char *DwarfLine::file(LineData *lnd, long n) {
	const uint8_t * prev_pos = lnd->cpos;
	size_t filelen, dirlen;
	uint64_t dir;
	char *result;

	/* I'm not sure if this is actually an error. */
	if(n == 0 || n > lnd->nfile)
		return NULL;

	filelen = strlen((const char *)lnd->filenames[n - 1]);
	lnd->cpos = lnd->filenames[n - 1] + filelen + 1;
	dir = readuleb128(lnd);
	lnd->cpos = prev_pos;
	if(dir == 0
	    || lnd->filenames[n - 1][0] == '/')
		return strdup((const char *)lnd->filenames[n - 1]);
	else if(dir > lnd->ndir)
		return NULL;

	dirlen = strlen((const char *) lnd->dirnames[dir - 1]);
	result = new char[dirlen+filelen+2];
	memcpy(result, lnd->dirnames[dir - 1], dirlen);
	result[dirlen] = '/';
	memcpy(result + dirlen + 1, lnd->filenames[n - 1], filelen);
	result[dirlen + 1 + filelen] = '\0';
	return result;
}

/* Initialize a state S. */

void init(struct LineInfo *s) {
	s->file = 1;
	s->line = 1;
	s->col = 0;
	s->pc = 0;
	s->endseq = 0;
}

/* Read a debug_line section. */

LineData *DwarfLine::lineopen() {
	LineData *lnd = NULL;
	int dwarf64;
	size_t size = pend-data;
	uint64_t len, plen;
	const uint8_t *table_start;

	if(size < 12)
		return NULL;

	lnd = new LineData;
	lnd->lsb = lsb;
	lnd->cpos = data;
	lnd->len = len = read_32(lnd->cpos);
	lnd->cpos += 4;
	if(len == 0xffffffff) {
		len = read_64(lnd->cpos);
		lnd->cpos += 8;
		dwarf64 = 1;
	} else if(len > 0xfffffff0)	/* Not a format we understand. */
		goto error;
	else
		dwarf64 = 0;

	if(size < len + (dwarf64 ? 12 : 4)
	  || len <(dwarf64 ? 15 : 11))	/* Too small. */
		goto error;

	if((lnd->version = read_16(lnd->cpos)) != 2)	/* Unknown line number format. */
		goto error;

	lnd->cpos += 2;
	lnd->plen = plen = dwarf64 ? read_64(lnd->cpos) : read_32(lnd->cpos);
	lnd->cpos += dwarf64 ? 8 : 4;
# ifdef NOTDEF
	if(len < plen +(lnd->cpos - data) || plen < 7)	/* why? */
		goto error;
# endif

	lnd->instrmin = lnd->cpos[0];
	lnd->isstmt = lnd->cpos[1];
	lnd->lbase = lnd->cpos[2];
	lnd->lrange = lnd->cpos[3];
	lnd->opbase = lnd->cpos[4];

	if(lnd->opbase == 0)
		/* Every valid line number program must use at
		 * least opcode 0 for LNEEndSequence. */
		goto error;

	lnd->stdlen = lnd->cpos + 5;
	if(plen < 5 +(lnd->opbase - 1))
		/* Header not long enough. */
		goto error;
	lnd->cpos += 5 + lnd->opbase - 1;
	lnd->end = data + plen +(dwarf64 ? 22 : 10);

	/* Make table of offsets to directory names. */
	table_start = lnd->cpos;
	lnd->ndir = 0;
	while(lnd->cpos != lnd->end && *lnd->cpos) {
		lnd->cpos = (const uchar*)memchr(lnd->cpos, 0, lnd->end-lnd->cpos);
		if(! lnd->cpos)
			goto error;
		lnd->cpos++;
		lnd->ndir++;
	}
	if(lnd->cpos == lnd->end)
		goto error;

	if(lnd->ndir){
		lnd->dirnames = new const uchar*[lnd->ndir];
		lnd->ndir = 0;
		lnd->cpos = table_start;
		while(*lnd->cpos) {
			lnd->dirnames[lnd->ndir++] = lnd->cpos;
			lnd->cpos = (uint8_t*)memchr(lnd->cpos, 0, lnd->end-lnd->cpos) + 1;
		}
	}
	lnd->cpos++;

	/* Make table of offsets to file entries. */
	table_start = lnd->cpos;
	lnd->nfile = 0;
	while(lnd->cpos != lnd->end && *lnd->cpos) {
		lnd->cpos = (const uchar*)memchr(lnd->cpos, 0, lnd->end-lnd->cpos);
		if(! lnd->cpos)
			goto error;
		lnd->cpos++;
		skipleb128(lnd);
		skipleb128(lnd);
		skipleb128(lnd);
		lnd->nfile++;
	}
	if(lnd->cpos == lnd->end)
		goto error;
	assert(lnd->nfile != 0);
	lnd->filenames = new const uchar*[lnd->nfile];
	lnd->nfile = 0;
	lnd->cpos = table_start;
	while(*lnd->cpos) {
		lnd->filenames[lnd->nfile++] = lnd->cpos;
		lnd->cpos =(uint8_t*)memchr(lnd->cpos, 0, lnd->end - lnd->cpos) + 1;
		skipleb128(lnd);
		skipleb128(lnd);
		skipleb128(lnd);
	}
	lnd->cpos++;
	lnd->finit = lnd->nfile;
	lnd->cpos = lnd->init = lnd->end;
	lnd->end = data + len +(dwarf64 ? 12 : 4);
	init(&lnd->cur);
	return lnd;

error:
	if(lnd){
		if(lnd->dirnames)	free(lnd->dirnames);
		if(lnd->filenames)	free(lnd->filenames);
		free(lnd);
	}
	return NULL;
}

/* Reset back to the beginning. */
void DwarfLine::reset(struct LineData * lnd) {
	lnd->cpos = lnd->init;
	lnd->nfile = lnd->finit;
	init(&lnd->cur);
}

/* Is there no more line data available? */

int atend(LineData *lnd){ return lnd->cpos == lnd->end; }

uchar DwarfLine::nextstate(LineData *lnd) {
	if(lnd->cur.endseq)
		init(&lnd->cur);
	for(;;) {
		uint8_t op;
		uint64_t tmp;

		if(lnd->cpos == lnd->end)
			return 0;
		op = *lnd->cpos++;
		if(op >= lnd->opbase) {
			op -= lnd->opbase;
			lnd->cur.line += op % lnd->lrange + lnd->lbase;
			lnd->cur.pc +=(op / lnd->lrange * lnd->instrmin);
			return 1;
		}
		else switch(op) {
		case LNSExtendedOp: {
			uint64_t sz = readuleb128(lnd);
			const uint8_t * op = lnd->cpos;

			if(lnd->end - op < sz || sz == 0)
				return 0;
			lnd->cpos += sz;
			switch(*op++) {
			case LNEEndSequence:
				lnd->cur.endseq = 1;
				return 1;

			case LNESetAddress:
				if(sz == 9)
					lnd->cur.pc = read_64(op);
				else if(sz == 5)
					lnd->cur.pc = read_32(op);
				else
					return 0;
				break;

			case LNEDefineFile: {
					const uint8_t * * filenames;
					filenames = (const uchar**)realloc 
					   (lnd->filenames, 
					   (lnd->nfile + 1)*sizeof(const uint8_t*));
					if(! filenames)
						return 0;
					/* Check for zero-termination. */
					if(! memchr(op, 0, lnd->cpos - op))
						return 0;
					filenames[lnd->nfile++] = op;
					lnd->filenames = filenames;
					/*
					 * There's other data here, like file sizes and
					 * modification times, but we don't need to read
					 * it so skip it.
					 */
				}
				break;

			default:	/* Don't understand it, so skip it. */
				break;
			}
			break;
		}
		case LNSCopy:
			return 1;
		case LNSAdvancePc:
			tmp = readuleb128(lnd);
			if(tmp ==(uint64_t) -1)
				return 0;
			lnd->cur.pc += tmp * lnd->instrmin;
			break;
		case LNSAdvanceLine:
			lnd->cur.line += readsleb128(lnd);
			break;
		case LNSSetFile:
			lnd->cur.file = readuleb128(lnd);
			break;
		case LNSSetColumn:
			lnd->cur.col = readuleb128(lnd);
			break;
		case LNSConstAddPc:
			lnd->cur.pc +=((255 - lnd->opbase) / lnd->lrange
			    * lnd->instrmin);
			break;
		case LNSFixedAdvancePc:
			if(lnd->end - lnd->cpos < 2)
				return 0;
			lnd->cur.pc += read_16(lnd->cpos);
			lnd->cpos += 2;
			break;
		default: {
				/* Don't know what it is, so skip it. */
				int i;
				for(i = 0; i < lnd->stdlen[op - 1]; i++)
					skipleb128(lnd);
				break;
			}
		}
	}
}

/* Set RESULT to the next 'interesting' line state, as indicated
   by STOP, or return FALSE on error.  The final(end-of-sequence)
   line state is always considered interesting. */

int DwarfLine::nextline(LineData *lnd, LineInfo *rv, enum StopConstants stop) {
	for(;;) {
		LineInfo prev = lnd->cur;

		if(! nextstate(lnd))
			return 0;

		if(lnd->cur.endseq)
			break;
		if(stop == StopAlways)
			break;
		if((stop & StopPc) && lnd->cur.pc != prev.pc)
			break;
		if((stop & StopPosMask) && lnd->cur.file != prev.file)
			break;
		if((stop & StopPosMask) >= StopLine
		    && lnd->cur.line != prev.line)
			break;
		if((stop & StopPosMask) >= StopCol
		    && lnd->cur.col != prev.col)
			break;
	}
	*rv = lnd->cur;
	return 1;
}

/*
 * Find  the  region (START->pc through END->pc) in the debug_line informa-
 * tion which contains PC. This routine starts searching at the current po-
 * sition  (which  is  returned as END), and will go all the way around the
 * debug_line information. It will return 0 if an error occurs or if  there
 * is  no  matching  region;  these  may  be  distinguished  by  looking at
 * START->endseq, which will be 0 on error and 1 if there was  no  matching
 * region.
 * You  could  write  this  routine using DwarfLine::next, but this version
 * will be slightly more efficient, and of course more convenient.
 */ 

int DwarfLine::findaddr(LineData *lnd, LineInfo *start, LineInfo *end, ulong pc) {
	const uint8_t * startpos;
	struct LineInfo prev;

	if(lnd->cur.endseq && lnd->cpos == lnd->end)
		reset(lnd);
	startpos = lnd->cpos;
	do {
		prev = lnd->cur;
		if(! nextstate(lnd)) {
			start->endseq = 0;
			return 0;
		}
		if(lnd->cur.endseq && lnd->cpos == lnd->end)
			reset(lnd);
		if(lnd->cpos == startpos) {
			start->endseq = 1;
			return 0;
		}
	} while(lnd->cur.pc <= pc || prev.pc > pc || prev.endseq);
	*start = prev;
	*end = lnd->cur;
	return 1;
}

/* Data is transformed such that:
 *
 *	f(x) = 1 if x > target
 *	       0 otherwise
 */

int DwarfLine::findlo(ulong k){
	int lo = lower, mid, hi = upper;
	while(lo != hi) {
		mid =(lo+hi)/2;		/* Or a fancy way to avoid int overflo */
		if(table[mid].pc <= k){
			/* This index, and everything belo it, must not be the
			 * first element greater than what  we're  looking for
			 * because this element is no greater than the element.
			 */
			lo = mid + 1;
		}else{
			/* This element is at least as large as the element, so
			 * anything after it can't be the first element that's
			 * at least as large.
			 */
			hi = mid;
		}
	}
	/* Now, lo and hi both point to the element in question. */
	assert(lo == hi);
	return table[lo].line;
}

/* Data is transformed such that:
 *
 *	f(x) = 1 if x < target
 *	       0 otherwise
 */

/*
 * For  this opposite problem it is also necessary to adjust the cal-
 * culation of mid. Due to the combination of  the  round-down-effect
 * in  the  division and the subtracting, it is possible to get nega-
 * tive high values without a modification. This could be  solved  by
 * changing  to a round-up-behavior in this calculation, for instance
 * by adding the modulus 2 term again.
 */

int DwarfLine::findhi(ulong k){
	int lo = lower, mid, hi = upper;
	while(lo != hi) {
		mid =(lo+hi)/2 + (lo+hi)%2;
		if(table[mid].pc >= k)
			hi = mid - 1;
		else
			lo = mid;
	}
	assert(lo == hi);
	return table[lo].line;
}

const char *DwarfLine::newcu(ulong unit, ulong loff){
	LineData *ld;
	LineInfo le;

	if(loff >= blen)
		return "bad stmtlist";

	data = base + loff;
	pend = base + blen;
	if((ld = lineopen())){
		Pclu l;
		l.unit = unit;
		while(nextline(ld, &le, StopPc)){
			l.pc = le.pc;
			l.line = le.line;
			push(l);
		}
		return "";
	}
	return "bad linedata";
}

void DwarfLine::dump(){
	printf("Line Table _size %d\n\n", _size);	
	printf("%-10s line unit\n", "pc");
	for( int i = 0; i < _size; i++ )
		printf( "0x%08x %4d 0x%08x\n", 
			table[i].pc, table[i].line, table[i].unit );
}

Range DwarfLine::range(ulong unit, int lo, int hi){
	int i = 0;
	Range r;

	while( i<_size && table[i].unit<unit ) ++i;
	while( i<_size && table[i].unit==unit && table[i].pc<lo ) ++i;
	r.lo = i++;
	while( i<_size && table[i].unit==unit && table[i].pc<=hi ) ++i;
	r.hi = --i;
	return r;

}

void DwarfLine::bounds(ulong unit, int lo, int hi){
	int i = 0;

	while( i<_size && table[i].unit<unit ) ++i;
	while( i<_size && table[i].unit==unit && table[i].pc<lo ) ++i;
	lower = i++;
	while( i<_size && table[i].unit==unit && table[i].pc<=hi ) ++i;
	upper = --i;
}

Range DwarfLine::range(ulong unit){
	int i = 0;
	Range r;

	while( i<_size && table[i].unit < unit ) ++i;
	r.lo = i;
	while( i<_size && unit==table[i].unit ) ++i;
	r.hi = i-1;
	return r;
}

void DwarfLine::bounds(ulong unit){
	int i = 0;

	while( i<_size && table[i].unit < unit ) ++i;
	lower = i;
	while( i<_size && unit==table[i].unit ) ++i;
	upper = i-1;
}

Pclu *DwarfLine::line(int i){
	return &table[i];
}

Pclu *DwarfLine::sline(){
	if(lower > upper)
		return 0;
	return &table[lower++];
}
