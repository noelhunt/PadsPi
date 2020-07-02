#ifndef BPTS_H
#define BPTS_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class Core;

enum LiftLay { LIFT, LAY };

class Trap : private PadRcv {
	friend	class Bpts;
	long	key;
const	char	*error;
	Trap	*sib;
const	char	*liftorlay(LiftLay,Core*);
PUBLIC(Trap,U_TRAP)
		Trap(Stmt*, Trap *);
	char	saved[4];
	Stmt	*stmt;
};

class Bpts : public PadRcv {
	friend	class Process;
	friend	class UnixProcess;
	friend	class HostProcess;
	Pad	*pad;
	Core	*core;
	Trap	*trap;
	int	layed;
	Trap	*istrap(Stmt*);
	void	select(Trap*);
	void	clearall();
	void	refresh();
	void	liftparents(Bpts*);
PUBLIC(Bpts,U_BPTS)
		Bpts(Core*);
	void	lift();
	void	lay();
	void	set(Stmt*);
	void	clr(Stmt*);
	int	isbpt(Stmt*);
	int	isasmbpt(long);
	Stmt	*bptstmt(long);
	void	hostclose();
	void	banner();
const	char	*enchiridion(long);
};

enum BegEnd { NEITHER = 0, BEGIN = 0x1, END = 0x2, BOTH = 0x3 };

class BptReq : public PadRcv {
	char	*file;
	char	*func;
	BegEnd	be;
	long	line;
	Expr	*expr;
	char	*error;
	char	*setfunc(Process*);
	char	*setline(Process*);
	void	parse(char*);
PUBLIC(BptReq,U_BPTREQ)
	char	*set(Process*);
		BptReq(char*, long,  char* =0);
		BptReq(char*, char*, char* =0);
};
#endif
