#ifndef SYMBOL_H
#define SYMBOL_H
#ifndef UNIV_H
#include "univ.h"
#endif

#include "dtype.h"
#include "elf.h"
#include "dwarf.h"
#include "dwarfrec.h"

/* sccsid[] = "%W%\t%G%" */

class Stmt; class SrcText;

class Symbol : public PadRcv {				// Symbol
	friend	class SymTab;
	friend	class DwarfSymTab;
	Symbol	*hashlink;
public:
	int	disc();
	int	ok();
		Symbol(Symbol*,Symbol*,const char*);
	Symbol	*parent;
	Symbol	*child;
	Symbol	*rsib;
	Symbol	*lsib;
	char	*_text;
virtual char	*text(long=0);
	Range	range;
	char	*dump();
	Source	*source();
};

enum {
	LEAVE = 0,
	SHOW  = 1,
	HIDE  = 2
};

class Var : public Symbol {				// Var
	friend	class Expr;
	friend	class UType;
	short	_disc;
	short	showorhide;
	Index	carte();
	void	reformat(long);
	char	*fmtlist();
	void	showutype(UType*);
public:
		Var(class SymTab*,class Block*,Var*,UDisc,const char*);
		~Var();
	int	disc();
	int	ok();
	DType	type;
	void	show(int=LEAVE, Attrib=0);
};

class BlkVars : public PadRcv {					// BlkVars
class	Block	*b;		// next block
	Var	*v;		// prev variable
PUBLIC(BlockVars,U_BLKVARS)
	Var	*gen();
		BlkVars(Block*i);
};

class Block : public Symbol {				// Block
	friend	class BlkVars;
	friend	class SymTab;
	friend	class DwarfSymTab;
	friend	class FileDesc;
	Var	*var;
PUBLIC(Block,U_BLOCK)
		Block(SymTab*,Symbol*,Block*,const char*);
		~Block();
class Stmt	*stmt;
};	

class Source : public Symbol {				// Source
	friend	class DwarfSymTab;
	friend	class FileDesc;
	friend	class Func;
	Func	*linefunc;
	Stmt	*linestmt;
PUBLIC(Source,U_SOURCE)
		Source(SymTab*,Source*,char*,long);
virtual		~Source();
	SrcText	*srctext;
	SymTab	*symtab;
	Block	*blk;
	char	*filename();
	Func	*funcafter(int,int);
	Stmt	*stmtafter(Func*,int);
	char	*text(long=0);
	char	*bname;
	char	*dir;
};
	
class Func : public Symbol {
friend	class	DwarfSymTab;
friend	class	I686Core;
	DwarfState save;
	long	regsave;
	Block	*_blk;
	Source	*typeinfo;
	char	*namewithargs;
	void	gather();
PUBLIC(Func,U_FUNC)
		Func(SymTab*,const char*,Source*,long);
		~Func();
	Block	*blk();
	Block	*blk(long);
	DType	type;
	Range	lines;
	Stmt	*stmt(long);	
	char	*text(long=0);
	char	*textwithargs();
	Var	*argument(int);
	int	regused(int);
};

#define Q_BPT   ((Expr*)1)	
class Stmt : public Symbol {				// Stmt
	friend	class DwarfSymTab;
	friend	class Instr;
class Process	*process;
class Pad	*srcpad();
	void	error(const char*);
	char	*contextsearch(char*,int);
PUBLIC(Stmt,U_STMT)
		Stmt(SymTab*,Block*,Stmt*);
		~Stmt();
	void	asmblr();
	void	select(long=0);
	char	*text(long=0);
	short	lineno;
	short	hits;
class Expr	*condition;
	Bls	*condtext;
	void	dobpt(int);
	void	settrace();
	void	openframe();
	char	*kbd(char*);
	char	*help();
const	char	*enchiridion(long);
	void	conditional(Expr*);
	char	*srcline();
	Func	*func();
	char	*journal(Bls&);
};

class UType : public Symbol {				// UType
	friend	class DwarfSymTab;
	friend	class DwarfType;
	friend	class TypMems;
	friend	class Var;				// for symtab
	SymTab	*symtab;
	long	begin;
	long	size;
	char	*canspecial;
	Var	*mem;
	void	gather();
	void	display();
PUBLIC(UType,U_UTYPE)
		UType(SymTab*,long,long,char*);
virtual		~UType();
	DType	type;
	Source	*src;
	Index	carte(enum Op);
	void	show(int=LEAVE, Attrib=0);
};

class TypMems : public PadRcv {				// TypMems
	char	pub_filler[8];
	UType	*ut;
	Var	*v;			// prev
PUBLIC(TypMems,U_TYPMEMS)
		TypMems(UType *);
	Var	*gen();
};

#endif
