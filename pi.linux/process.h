#ifndef PROCESS_H
#define PROCESS_H
#ifndef UNIV_H
#include "univ.h"
#endif
#include "format.h"

/* sccsid[] = "%W%\t%G%" */

enum FName { FN_ENTRY = 0, FN_BASE = 1, FN_FULL = 2 };

class Process : public PadRcv {
	friend class Master;	
	friend class SigMask;
	friend class SrcDir; friend class SrcText;
	friend class DbmonMaster;	
	friend class NrtxMaster; friend class NrtxCore;	
	friend class UnixProcess; friend class UnixMaster;	
	friend class HostProcess; friend class HostMaster;	
protected:
	int	isdead;
	Asm	*_asm;
	Bpts	*_bpts;
	Core	*core;
	CallStk	*callstk;
	Memory	*memory;
	Journal	*_journal;
	Pad	*pad;
	Process	*sibling;
	Master	*master;
	Process *parent;
	SrcDir	*srcdir;
	char	*srcpath;
	Behavs	prev_behavs;
	char	stoprequest;
	char	cycles;
	Process	*_slave;
#define BEHAVSKEY 1
#define ERRORKEY 2
	int	padlines;
	Bls	bls[3];			/* error [1] behavs [2] */
	void	openbpts();
	void	opentypes();
	void	mergeback(long);
	void	srcfiles();
	void	habeascorpus(Behavs,long);
	void	banner();
	int	changes();
	void	docycle();
	void	insert(long,PRINTF_TYPES);
	void	closeframes();
	void	merge();
	char	*bptreq(BptReq*);
	void	slavedriver(Process*);
	void	cycle();
virtual	Index	carte();
	void	linereq(int,Attrib=0);
PUBLIC(Process,U_PROCESS)
	Globals	*globals;
		Process(Process* =0, const char* =0, const char* =0, const char* =0);
	char	*procpath;
	char	*stabpath;
	char	*comment;
	FName	fnametype;
	void	openglobals(Menu* =0);
	void	openmemory(long=0);
	void	openasm(long=0);
	void	openjournal();
	Journal	*journal();
	Frame	*frame(long);
	SymTab	*symtab();
	Bpts	*bpts();
	Process	*slave();
	void	go();
	void	pop(long);
	void	stmtstep(long);
	void	stepinto();
	void	instrstep(long);
	void	stepover(long,long);
	void	openframe(long,char* =0);
	void	openpad();
	void	currentstmt();
	void	error(const char*);

	char	*help();
	char	*kbd(char*);
const	char	*enchiridion(long);
virtual	void	userclose();
virtual	void	stop();
};
#endif
