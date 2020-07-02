#ifndef I686CORE_H
#define I686CORE_H

/* sccsid[] = "%W%\t%G%" */

#include "elf.h"

class I686Core : virtual public Core {
protected:
	int	REG_AP();
	int	REG_FP();
	int	REG_PC();
	int	REG_SP();
	int	nregs();
	long	returnregloc();
	int	argstart();
	long	callingpc(long);
	long	callingfp(long);
	CallStk *callstack();
	int	atreturn(long);
	Asm	*newAsm();
	void	newSymTab(long);
	char	*regname(int);
	long	saved(Frame*,int,int);
	Frame	frameabove(long);
	long	instrafterjsr();
	int	atjsr(long);
	char	*stepprolog();
	char	*docall(long,int);
	long	apforcall(int);
public:
		I686Core();
};
#endif /* I686CORE_H */
