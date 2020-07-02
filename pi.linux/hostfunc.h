#include <sys/param.h>
#include "unix.h"
#include "elf.h"

#ifdef sigmask
# undef sigmask
#endif

#ifdef NGREG
# undef NGREG
#endif
#define NGREG		17
#define WORDSIZE	sizeof(long)
#define	uregs		user_regs_struct	/* avoid Linux */

#ifdef EMULATE
#define ptracereq	__ptrace_request
#else
enum ptracereq {
	R_TRACEME,
	R_PEEK,
	R_PEEKU,
	R_POKE,
	R_POKEU,
	R_CONT,
	R_KILL,
	R_SINGLESTEP,
	R_GETREGS,
	R_SETREGS,
	R_ATTACH,
	R_DETACH,
	NREQ,
	R_READ = NREQ,
	R_WRITE,
};
#endif

enum {
	WAIT_POLL	= 0x1,
	WAIT_PCFIX	= 0x2,
	WAIT_DISCARD	= 0x4
};

union ptraceval {
	long word;
	char byte[WORDSIZE];
};

/*
 * Data structure private to each process or core dump
 */

typedef struct Coreseg Coreseg;

struct Coreseg {
	int	fd;
	ulong	start;
	ulong	end;
	ulong	offset;
};
enum {
	STAB,
	CORE
};

typedef struct Localproc Localproc;

struct Localproc {
	int		pid;
	int		opencnt;
	long		sigmsk;
	ulong		startdata;
	Unixstate	state;
	prstatus_t	prstat;
	Localproc	*next;
	char		*statfile;	/* Cpu usage, proctime */
	/* For core dumps */
	int		corefd;		/* Data, stack, regs */
	int		stabfd;		/* text */
	int		nseg;
	Coreseg		*seg;
};

typedef struct Stat Stat;

struct Stat {
	int	pid;
	char	aout[PATH_MAX];	/** The filename of the executable **/
	char	state;		/** 1 **/
	uint	euid;
	uint	egid;
	int	ppid;
	int	pgrp;
	int	sid;		/** session id **/
	int	ttynr;
	int	ttypgid;
	uint	flags;		/** The flags of the process. **/
	uint	minflt;		/** The number of minor faults **/
	uint	cminflt;	/** The number of minor faults with children **/
	uint	majflt;		/** The number of major faults **/
	uint	cmajflt;	/** The number of major faults with children **/
	int	utime;		/** user mode jiffies **/
	int	stime;		/** kernel mode jiffies **/
	int	cutime;		/** user mode jiffies with childs **/
	int	cstime;		/** kernel mode jiffies with childs **/
	int	counter;	/** process's next timeslice **/
	int	nice;		/** the standard nice value, plus fifteen **/
	uint	timeout;	/** The time in jiffies of the next timeout **/
	uint	itrealvalue;	/** The time before the next SIGALRM is sent to the process **/
	int	starttime;	/** Time the process started after system boot **/
	uint	vsize;		/** Virtual memory size **/
	uint	rss;
	uint	rlim;		/** Current limit in bytes on the rss **/
	uint	startcode;	/** The address above which program text can run **/
	uint	endcode;	/** The address below which program text can run **/
	uint	startstack;	/** The address of the start of the stack **/
	uint	kstkesp;	/** The current value of ESP **/
	uint	kstkeip;	/** The current value of EIP **/
	int	signal;		/** The bitmap of pending signals **/
	int	blocked;	/** The bitmap of blocked signals **/
	int	sigignore;	/** The bitmap of ignored signals **/
	int	sigcatch;	/** The bitmap of catched signals **/
	uint	wchan;
	int	sched;		/** scheduler **/
	int	priority;	/** scheduler **/
};

class Hostfunc {
	friend	class HostCore;
	friend	class Core;
	Elf	*e[2];
	Localproc *phead;
	char	*pend;
	char	procname[32];			/* /proc/nnnnn/filename */
	char	procbuffer[128];
	int	hangpid;
	int	corepid;
	long	_regaddr;
struct	uregs	regs;
	int	first;
	long	ticks;
	Stat	pstat;
	uchar	bkptinstr;
	char	pscmd[32];
	char	*pscmds[32];
	int	open(char*, int);
	int	open(char*, int, mode_t);
	void	close(int);
	char	*pathexpand(char*, char*, int);
	char	*regrw(Localproc*, char*, int, int, int);
	void	coreclose(Localproc*);
	char	*corerw(Localproc*, char*, ulong, int, int);
	char	*coreregrw(Localproc*, char*, int, int, int);
	int	ptrace(Localproc*, enum ptracereq, long, int, long*);
	int	procwaitstop(Localproc*, int);
	int	procwait(Localproc*, int);
	void	bcopy(char*, char*, int);
	void	bzero(char*, int);
public:
		Hostfunc();
		~Hostfunc();
	int	sigmaskinit();
	char	*osname();
	Localproc *open(int);
	Localproc *pidtoproc(int);
	void	close(Localproc*);
	int	getsymtabfd(Localproc*);
	char	*run(Localproc*);
	char	*stop(Localproc*);
	char	*destroy(Localproc*);
	void	getstate(Localproc*, Unixstate*);
	char	*readwrite(Localproc*, char*, ulong, int, int);
	char	*liftbpt(Localproc*, long, char*);
	char	*laybpt(Localproc*, long);
	char	*step(Localproc*);
	char	*waitstop(Localproc*);
	char	*ssig(Localproc*, int);
	char	*csig(Localproc*);
	int	nsig() { return NSIG; }
	char	*sigmask(Localproc*, int);
	char	*signalname(int);
	char	*proctime(Localproc*);
	int	getppid(Localproc*);
	int	getpsfield()		{ return 4; }
	int	atsyscall(Localproc*);
	char	*exechang(Localproc*, int);
	int	exechangsupported()	{ return 1; }
	int	hang(char*);
	Localproc *coreopen(char*, char*);
	int	regaddr()		{ return _regaddr; }
	char	**getpscmds()		{ return pscmds; }
	long	scratchaddr()		{ return 0x10000; }
	int	stabfdsupported()	{ return 0; }
};
