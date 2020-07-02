#ifndef UNIV_H
#define UNIV_H

#include <pads.h>
#include <stdio.h>
# ifdef CFRONT
#include <osfcn.h>
#include <libc.h>
# endif
#include <time.h>

#include "lib.h"

/* sccsid[] = "%W%\t%G%" */

enum {
	MAGIC1=101,
	MAGIC2=503
}; 

enum Op{
	O_CONST		= 1,
	O_DEREF		= 2,
	O_REF		= 3,
	O_INDEX		= 4,
	O_DOT		= 5,
	O_ARROW		= 6,
	O_PLUS		= 7,
	O_MINUS		= 8,
	O_MULT		= 9,
	O_DIV		= 10,
	O_CALL		= 11,
	O_MOD		= 12,
	O_ASSIGN	= 13,
	O_COMMA		= 14,
	O_SIZEOF	= 15,
	O_TYPEOF	= 16,
	O_QINDEX	= 17,
	O_CAST		= 18,
	O_QCAST		= 19,
	O_EQ		= 20,
	O_NE		= 21,
	O_LT		= 22,
	O_GT		= 23,
	O_LE		= 24,
	O_GE		= 25,
	O_ENV		= 26,
	O_LOGAND	= 27,
	O_LOGOR		= 28,
	O_LOGNOT	= 29,
	O_BITAND	= 30,
	O_BITOR		= 31,
	O_1SCOMP	= 32,
	O_BITXOR	= 33,
	O_FABS		= 34,
	O_QSTRUCT	= 35,
	O_QENUM		= 36,
	O_RANGE		= 37,
	O_SPECIAL	= 40,
	O_LSHIFT	= 41,
	O_RSHIFT	= 42
};

enum EDisc{
	E_UNARY		= 1,
	E_BINARY	= 2,
	E_LCONST	= 3,
	E_DCONST	= 4,
	E_ID		= 5,
	E_SYMNODE	= 6
};

// c.f. unix.h
enum Behavs {
	ERRORED		= 0,
	BREAKED		= 1,
	HALTED		= 2,
	ACTIVE		= 3,
	PENDING		= 4,
	INCOHATE	= 5,
	STMT_STEPPED	= 6,
	INST_STEPPED	= 7
};

class Asm;
class Block;
class Bls;
class Bpts;
class BptReq;
class CallStk;
class Context;
class Core;
class CoreContext;
class Cslfd;
class DType;
class DwarfAbbrev;
class DwarfAbbrevs;
class DwarfRec;
class Expr;
class Frame;
class Func;
class Globals;
class Hostfunc;
class DwarfType;
class DwarfTShare;
class DwarfSource;
class DwarfSymTab;
class Index;
class Journal;
class Master;
class Memory;
class Pad;
class Phrase;
class Process;
class Remote;
class SigBit;
class SigMask;
class Source;
class SrcDir;
class Stmt;
class SymTab;
class Symbol;
class Trap;
class UType;
class Var;

inline int eqstr(const char *a, const char *b){
	return !a ? !b : b && *a==*b && !strcmp(a+1,b+1);
}

enum MemLayout { MSBFIRST, LSBFIRST };

struct Range {
	long	lo;
	long	hi;
};

class Cslfd {
	 void	init(long l, double d);
public:
	 char	*flterr;
	 double	dbl;
	 float	flt;
	 long	lng;
	 short	sht;
	 char	chr;
unsigned char	uch();
unsigned short	ush();
	 char	*floaterror();
	 	Cslfd(long);
	 	Cslfd(double);
		Cslfd(int);
		Cslfd();
};

enum UDisc {
	U_ERROR		= 0,

	U_FUNC		= 1,
	U_GLB		= 2,
	U_STA		= 3,
	U_STMT		= 4,
	U_UTYPE		= 5,
#				define TOSYM 0x7	/* 2^n-1 */
	U_ARG		= 10,
	U_AUT		= 11,
	U_BLOCK		= 12,
	U_FST		= 13,
	U_MOT		= 14,
	U_SOURCE	= 15,

	U_ASM		= 20,
	U_AUDIT		= 21,
	U_BLKVARS	= 22,
	U_BPTS		= 23,
	U_CELL		= 24,
	U_CORE		= 25,
	U_DTYPE		= 26,
	U_EXPR		= 27,
	U_MASTER	= 28,
	U_GLOBALS	= 29,
	U_INSTR		= 30,
	U_MEMORY	= 32,
	U_PHRASE	= 33,
	U_PROCESS	= 34,
	U_REG		= 35,
	U_SRCTEXT	= 39,
	U_SYMTAB	= 41,
	U_TRAP		= 42,
	U_TYPMEMS	= 43,
	U_WD		= 44,
	U_SIGMASK	= 45,
	U_HELP		= 46,
	U_SRCDIR	= 47,
	U_CONTEXT	= 48,
	U_HOSTMASTER	= 49,
	U_SIGBIT	= 50,
	U_BPTREQ	= 51,
	U_JOURNAL	= 52,
	U_FRAME		= 53
};

#define PUBLIC(c,d)\
	int	disc();\
public:\
	int	ok()	{ return this && disc() == d; }

#define SVP	1	/* != 0 */
#endif

#define SIG_ARG_TYP SIG_TYP

extern char *PATH;
extern Hostfunc *hfn;
