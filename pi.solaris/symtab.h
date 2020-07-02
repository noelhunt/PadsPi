#ifndef SYMTAB_H
#define SYMTAB_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

#include "elf.h"
#include "mip.h"

extern int FunctionGathered, UTypeGathered, FunctionStubs, UTypeStubs;
extern int IdToSymCalls, StrCmpCalls;

class SSet {
	friend	class SymTab;
	friend	class LookupCache;
	UDisc	v[8];
public:
		SSet(UDisc=U_ERROR);
		SSet(UDisc,UDisc,UDisc=U_ERROR,UDisc=U_ERROR,
		     UDisc=U_ERROR,UDisc=U_ERROR,UDisc=U_ERROR);
};

class LookupCache {
	SSet	set;
	Symbol	*sym;
	long	loc;
	char	*id;
public:
		LookupCache() {}
	Symbol	*match(SSet, long);
	void	save(SSet, long, Symbol*);
};

class SymTab : public PadRcv {
	friend class DwarfType;
	friend class Core;
protected:
	Pad	*_pad;
	int	fd;
	char	*strings;
	long	strsize;
	long	entries;
	long	_magic;
	long	relocation;
#define HASH	 	101	/* prime */
	Symbol	*hashtable[TOSYM+1][HASH];
	Core	*_core;
const	char	*stabpath();
	Source	*_root;
	Block	*fakeblk();
const	char	*dump();
	Block	*_blk;
const	char	*_warn;
	Index	castix[UNDEF];	// only [STRTY] [ENUMTY] used
	UType	*utype;
	SymTab	*inherit;
	Var	*globregs(Block*, int);
#ifdef DEMANGLE
	void	uncfront(Var *, char*);
#endif
	void	showutype(UType*);
	LookupCache
		loctosymcache;

virtual char	*gethdr()		{ return "SymTab.gethdr"; }
virtual	Source	*tree()			{ return 0; }
PUBLIC(SymTab,U_SYMTAB)
		SymTab(Core*,int,SymTab* =0,long=0);
virtual		~SymTab();
	void	read();
	void	enter(Symbol*);
	Symbol	*idtosym(SSet,const char*,int=1);
	Symbol	*loctosym(SSet,long,int=1);
	Pad	*pad();
	Core	*core();
const	char	*symaddr(long);
	Source	*root();
	long	modtime();
	Block	*blk();
const	char	*warn();
	Index	utypecarte(short);
	long	magic();
	UType	*utypelist();
	void	banner();
virtual	Block	*gatherfunc(Func*);
virtual	Var	*gatherutype(UType*);
	void	opentypes();
const	char	*enchiridion(long);
};

const char *DiscName(UDisc);
#endif
