#ifndef FRAME_H
#define FRAME_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

struct FpFrame {
	long	fp;
	Func	*func;
	Frame	*frame;
};

class CallStk {
public:
	Core	*core;
	long	size;
	FpFrame	*fpf;
		CallStk(long, Core*);
		~CallStk();
	Frame	frame(long);
};

class Frame : public PadRcv {
	friend class Globals;
	friend class SrcText;
	friend class Process;
	long	saved(int,int);
	void	pickvar(Var*);
	void	pickall();
	Index	varcarte();
	Index	regcarte();
	void	regall();
	long	regloc(int,int);
	void	ambiguous(const char*, class BlkVars&);
	void	pop();
PUBLIC(Frame,U_FRAME)
	long	level;		/* deepest() = 1; main() = core->stacksize() */
	Core	*core;
	long	fp;
	long	ap;
	long	pc;
	long	nargs;
	long	regbase;
	long	regsave;	/* VAX: mask; mac32: no of regs */
	Func	*func;
		Frame(Core*);	/* cfront bug */
		Frame();
	Cslfd	*peek(long, Cslfd* = 0);
	Index	carte();
	Pad	*pad;
	Phrase	*phraset;
	SymTab	*symtab();
	Var	*idtosym(const char*);
	char	*blockmove(long, long, long);
	char	*special(char*, long);
const	char	*enchiridion(long);
	char	*makephrase(Expr*,long=0);
	char	*peekstring(long, char* = 0 );
	char	*poke(long,long,int);
	char	*pokedbl(long,double,int);
	char	*text(Bls&);
	int	addsymbols();
	int	changes(long verbose = 0);
	long	locate(Var*);
	void	opencallersframe();
	Frame	*caller();
	void	freekey(long);
	void	hostclose();
	char	*kbd(char*);
	char	*help();
	void	select(long=0);
virtual	void	banner();
virtual	void	open(long=0);
};

class Globals : public Frame {
	Index	ix;
	void	pickall();
PUBLIC(Frame,U_FRAME)
		Globals(Core *);
	void	open(long=0);
	void	addvars(Menu*);
	void	banner();
const	char	*enchiridion(long);
};

#endif
