#ifndef ASM_H
#define ASM_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class Instr : public PadRcv {
PUBLIC(Instr,U_INSTR)
	long	addr;
	long	next;
	Asm	*_asm;
	int	fmt;
	Cslfd	m;
	char	bpt;
	char	reg;
	char	opcode;

	Index	carte();
	void	reformat(int);
	Instr	*sib;
const	char	*literal(long);
const	char	*symbolic(const char* ="");
	Var	*local(UDisc, long);
	Var	*field(Var*, long);
const	char	*regarg(const char*, long);
	void	succ(int);
	void	memory();
	void	dobpt(int);
	void	showsrc();
	void	openframe();
	void	display();

virtual	const char *arg(int);
virtual	const char *mnemonic();
virtual	int	argtype(int);
virtual	int	nargs();
	
		Instr(Asm*,long);
};

class Asm : public PadRcv {
	friend class Instr;
	friend class M68kInstr;
	friend class SparcInstr;
	friend class Mac32Instr;
	friend class Dsp32Instr;
	friend class MipsInstr;
	friend class I386Instr;
	friend class I686Instr;
protected:
	int	fmt;
	Core	*core;
	Pad	*pad;
	Instr	*instrset;
	void	instrstep(long);
	void	stepover();
	void	displaypc();
	void	go();
	void	pop();
	void	stop();

virtual const char *literaldelimiter();
virtual Instr	*newInstr(long l);
PUBLIC(Asm,U_ASM)
		Asm(Core*);
	char	*kbd(char *);
	char	*help();
const	char	*enchiridion(long);
	void	userclose();
	void	open(long=0);
	void	banner();
};
#endif
