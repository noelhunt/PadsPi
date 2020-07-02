#ifndef CORE_H
#define CORE_H
#ifndef UNIV_H
#include "univ.h"
#endif

#include "elf.h"

/* sccsid[] = "%W%\t%G%" */

const char *BehavsName(Behavs);

union UCslfd {			/* lsb lo */
	 double	dbl;
	 float	flt;
	 long	lng;
	 short	sht;
	 char	chr;
};

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef REG_PC
# undef REG_PC
#endif
#ifdef REG_FP
# undef REG_FP
#endif
#ifdef REG_SP
# undef REG_SP
#endif
#ifdef REG_AP
# undef REG_AP
#endif

class Core;

class Context {
	Core	*core;
	char	*regsave;
	int	regbytes;
PUBLIC(Context,U_CONTEXT)
		Context(Core*);
		~Context();
	char	*error;
virtual	void	restore();
virtual	void	save();
};

enum StackDir { GROWDOWN, GROWUP };

#define PEEKFAIL ((Cslfd*)-1)
class Core : public PadRcv {
	friend	class Context;
protected:
struct	stat	corestat;
struct	stat	stabstat;
const	char	*behavs_problem;
	int	corefd;
	int	stabfd;
	int	_online;
	Process	*_process;
	SymTab	*_symtab;
	StackDir stackdir;
	MemLayout memlayout;
	int	bptsize;
virtual char	 *read(long,char*,int);
virtual char	 *write(long,char*,int);
virtual char	 *readwrite(long,char*,int,int);
virtual char	 *dostep(long,long,int);
virtual int	instack(long,long);
virtual	int	fpvalid(long);
virtual Behavs	behavetype();
virtual long	scratchaddr();
virtual long	regaddr();
virtual int	atjsr(long);
virtual char	*stepoverjsr();
virtual int	atreturn(long);
virtual int	atsyscall();
virtual	long	instrafterjsr();
virtual	long	callingfp(long);
virtual	long	callingpc(long);
virtual	void	newSymTab(long =0);
	int	corefstat();
	int	stabfstat();
PUBLIC(Core,U_CORE)
		Core(Process*, Master*);
		Core() {}	// For multiple virtual inheritance
virtual		~Core() {}
	Process *process();
	SymTab	*symtab();
	char	*procpath();
	char	*stabpath();

virtual	Context	*newContext();
virtual int	cntxtbytes();
virtual char	*liftbpt(Trap*);
virtual char	*laybpt(Trap*);
virtual	int	REG_PC();
virtual	int	REG_FP();
virtual	int	REG_SP();
virtual	int	REG_AP();
virtual	long	saved(Frame*,int,int=0);
virtual	Behavs	behavs();
virtual	Asm	*newAsm();
virtual	Cslfd	*peek(long,Cslfd* =PEEKFAIL);
virtual	Cslfd	*peekcode(long);
virtual	CallStk	*callstack();
virtual	Frame	frameabove(long);
virtual	char	*blockmove(long,long,long);
virtual	char	*special(char*,long);
virtual	char	*destroy();
virtual	char	*eventname();
virtual	char	*open();
virtual	char	*peekstring(long,char* = 0);
virtual	char	*poke(long,long,int);
virtual	char	*pokedbl(long,double,int);
virtual	char	*problem();
virtual	char	*readcontrol();
virtual	char	*regname(int);
virtual	char	*reopen(char*,char*);
virtual	char	*resources();
virtual	char	*run();
virtual	char	*step(long=0,long=0);
virtual	char	*popcallstack();
virtual	char	*stepprolog();
virtual	char	*stop();
virtual char	*specialop(char*);
virtual	int	event();
virtual	int	online();
virtual	long	regloc(int,int=0);
virtual	void	close();
virtual	long	apforcall(int);
virtual char	*docall(long,int);
virtual	long	returnregloc();
virtual char	*regpoke(int r,long l);
virtual	long	regpeek(int r);
virtual	long	fp();
virtual	long	sp();
virtual	long	pc();
virtual	long	ap();
virtual void	slavedriver(Core*);
virtual	int	nregs();
virtual	int	argalign();		/* coff symtab info */
virtual	int	argdir();
virtual	int	argstart();
virtual	int	nsig();			/* signals */
virtual char	*signalclear();
virtual char	*signalsend(long);
virtual char	*signalmask(long);
virtual char	*signalname(long);
virtual	long	signalmaskinit();
virtual char	*exechang(long);	/* hang on exec */
virtual	int	exechangsupported();
};
#endif
