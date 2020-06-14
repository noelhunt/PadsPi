#ifndef HASHTYPES
# include "skiplist.h"
#endif

/* sccsid[] = "%W%\t%G%" */

#define VALOF(x,y)	( x==TConstant ? (y)->c : 0 )

class Entry;

class Entry {
	friend class DwarfTShare;
	DType	type;
	ulong	addr;
	Entry	*rsib; 
public:
		Entry(ulong, DType*);
		Entry();
	DType	*dtype(){ return &type; }
};

class DwarfType {
	Source	*src;
	int	gen;
	Bls	b;
	Dwarf	*dwarf;
	int	nbuf;
# define MAXLINE 1024
	char	buf[8][MAXLINE];
#ifdef DEMANGLE
const	char	*demangle(const char*, char*);
#endif
	void	resolv();
	int	used;
	DwarfTShare *share;
public:
		~DwarfType();
		DwarfType(Source*, DwarfTShare*, Dwarf*);
	void	addinclude(char*, int, long);
	DType	chain(DType*);
	DType	gettype(ulong);
	void 	maketype(DwarfRec&);
};

typedef class DType *DTypep;
typedef DTypep *DTypepar;

class DwarfTShare {
friend	class	DwarfType;
	int	used;
#ifdef HASHTYPES
	int	hashsize;
	Entry	*hashtab[MAGIC2];
	int	hashloc(ulong);
#else
	Skiplist *list;
#endif
	DTypepar types;
public:
		~DwarfTShare();
		DwarfTShare();
	void	entertype(ulong, ulong, DType*);
	DType	*findtype(ulong, ulong=0);
};
