#include <procfs.h>
#include "unix.h"
#include "elf.h"

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
	int		corefd;		/* corefile or /proc/<pid>/as		*/
	int		stabfd;		/* a.out file or /proc/<pid>/path/a.out	*/
	int		opencnt;
	Unixstate	state;
	pstatus_t	s;
	Localproc	*next;
	/* For /proc */
	int             ctlfd;          /* /proc/<pid>/ctl	*/
	int		statusfd;	/* /proc/<pid>/status	*/
	/* For core dumps */
	int		nseg;
	Coreseg		*seg;
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
	long	corepid;
	long	_regaddr;
	int	first;
	uchar	bkpt_instr;
	char	pscmd[32];
	char	*pscmds[32];
	int	open(char*, int);
	int	open(char*, int, mode_t);
	void	close(int);
	void	state(Localproc*);
	char	*pathexpand(char*, char*, int);
	char	*regrw(Localproc*, char*, int, int, int);
	void	coreclose(Localproc*);
	char	*corerw(Localproc*, char*, ulong, int, int);
	char	*coreregrw(Localproc*, char*, int, int, int);
public:
		Hostfunc();
		~Hostfunc();
	long	sigmaskinit();
	char	*osname();
	Localproc *open(int);
	Localproc *pidtoproc(int);
	void	close(Localproc*);
	int	getsymtabfd(Localproc*);
	char	*run(Localproc*);
	char	*stop(Localproc*);
	char	*destroy(Localproc*);
	void	getstate(Localproc*, struct Unixstate*);
	char	*readwrite(Localproc*, char*, ulong, int, int);
	char	*setbkpt(Localproc*, long);
	char	*step(Localproc*);
	char	*waitstop(Localproc*);
	char	*ssig(Localproc*, long);
	char	*csig(Localproc*);
	int	nsig() { return NSIG; }
	char	*sigmask(Localproc*, long);
	char	*signalname(long);
	char	*proctime(Localproc*);
	int	getppid(Localproc*);
	int	getpsfield() { return 4; }
	int	atsyscall(Localproc*);
	char	*exechang(Localproc*, long);
	int	exechangsupported()	{ return 1; }
	int	hang(char*);
	Localproc *coreopen(char*, char*);
	char	**getpscmds() { return pscmds; }
	long	regaddr() { return (long)_regaddr; }
	long	scratchaddr() { return 0x10000; }
	int	stabfdsupported() { return 0; }
};
