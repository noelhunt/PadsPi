/*
 * Copyright (c) 1990-1992 AT&T All Rights Reserved
 *	This is unpublished proprietary source code of AT&T
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *
 * Pi Unix driver for Sun x86 machines running Solaris 5.11
 *	This file uses /proc to examine running processes.
 *
 * N. L. Hunt	22/5/15
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ulimit.h>
#include <pwd.h>
#include <procfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/regset.h>
#include "elf.h"
#include "hostfunc.h"

#define	I386_SYSCALL	0x80
#define I386_BPT	0xCC	/* INT 3 */
#define BPTFLT		3
#define TRAP_LEN	1
#define TRAP_INST	0xCC

#if defined(__i386)
#define TRAP_MASK	0xFFFFFF00
#elif defined(__x86_64)
#define TRAP_MASK	0xFFFFFFFFFFFFFF00
#endif

#define	bit(x)		(1<<(x-1))
#define	ISCORE(p)	((p)->pid < -1)
#define REGSIZE		(NGREG * 4)

# define SIZEOF2(a, b)	(sizeof(a) + sizeof(b))

extern Hostfunc *hfn;

/*
 * Register layout transmitted to pi
 */
typedef struct dbregs dbregs;

struct dbregs {
	int r[NGREG];
};

struct ctl {
	long cmd;
	union {
		long		signal;   /* PCKILL, PCUNKILL	*/
		long		timeo;    /* PCTWSTOP		*/
		ulong_t		flags;    /* PCRUN, PCSET, PCUNSET */
		caddr_t		vaddr;    /* PCSVADDR		*/
		siginfo_t	siginfo;  /* PCSSIG		*/
		sigset_t	sigset;   /* PCSTRACE, PCSHOLD	*/
		fltset_t	fltset;   /* PCSFAULT		*/
		sysset_t	sysset;   /* PCSENTRY, PCSEXIT	*/
		prgregset_t	regset;   /* PCSREG, PCAGENT	*/
		prfpregset_t	fpregset; /* PCSFPREG		*/
		prwatch_t	watch;    /* PCWATCH		*/
		priovec_t	iovec;    /* PCREAD, PCWRITE	*/
	} arg;
};

extern "C" void waithandler(int i){
	int pid, cursig;
	int tstat;
	Localproc *p;

	pid = wait(&tstat);
	signal(SIGCLD, waithandler);
	if (pid < 0 || !(p = hfn->pidtoproc(pid)))
		return;

	if (WIFSTOPPED(tstat)) {
		cursig = WSTOPSIG(tstat);
		if (cursig == SIGSTOP)
			p->state.state = UNIX_HALTED;
		else if (cursig == SIGTRAP)
			p->state.state = UNIX_BREAKED;
		else {
			p->state.state = UNIX_PENDING;
			p->state.code = cursig;
		}
	} else {
		p->state.state = UNIX_ERRORED;
		if (WIFSIGNALED(tstat))
			p->state.code = WTERMSIG(tstat);
		if (WIFEXITED(tstat))
			p->state.code = WEXITSTATUS(tstat);
	}
}

extern "C" void alarmcatcher(int s){}

Hostfunc::Hostfunc(){
	first = 1;
	corepid = -1;
	_regaddr = -REGSIZE - 4;
	bkpt_instr = I386_BPT;
	pscmds[0] = 0;
	pscmds[1] = "/usr/bin/ps -a ";
	pscmds[2] = "/usr/bin/ps -e ";
	pscmds[3] = 0;
}

/* Also use for initialization */
char *Hostfunc::osname(){
	struct passwd *pw;

	if (first) {
#ifdef NOTDEF
		close(2); /* Otherwise error messages from ps goto rtpi */
		open("/dev/null", 2);
#endif
		if (pw = ::getpwuid(getuid()))
			sprintf(pscmd, "/usr/bin/ps -u %s", pw->pw_name);
		else
			sprintf(pscmd, "/usr/bin/ps");
		pscmds[0] = pscmd;
		signal(SIGCHLD, waithandler);
		first = 0;
	}
	return "SunOS 5.11 i86pc";
}

long Hostfunc::sigmaskinit(){
	return	bit(SIGILL)|bit(SIGINT)|bit(SIGTRAP)|bit(SIGIOT)|
		bit(SIGEMT)|bit(SIGFPE)|bit(SIGBUS)|bit(SIGSEGV)|
		bit(SIGSYS)|bit(SIGPIPE);
}

Localproc *Hostfunc::pidtoproc(int pid){
	Localproc *p;

	for (p = phead; p; p = p->next)
		if (p->pid == pid)
			return p;
	return 0;
}

int Hostfunc::open(char *path, int oflag){
	return ::open(path, oflag);
}

int Hostfunc::open(char *path, int oflag, mode_t mode){
	return ::open(path, oflag, mode);
}

Localproc *Hostfunc::open(int pid){
	Localproc *p = phead;
	struct ctl ctl;

	if (pid < 0)
		return 0;
	if ((p = pidtoproc(pid))) {
		p->opencnt++;
		return p;
	}

	p = new Localproc;
	snprintf(procname, sizeof(procname), "/proc/%05d", pid);
        pend = procname + strlen(procname);

	(void) strcpy(pend, "/as");
	if((p->corefd = open(procname, O_RDWR|O_EXCL, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}
	(void) strcpy(pend, "/path/a.out");
	if((p->stabfd = open(procname, O_RDONLY, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}
	(void) strcpy(pend, "/ctl");
	if((p->ctlfd = open(procname, O_WRONLY, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}
	(void) strcpy(pend, "/status");
	if((p->statusfd = open(procname, O_RDONLY, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}

	p->pid = pid;
	p->opencnt = 1;
	p->next = phead;
	phead = p;
	state(p);
	ctl.cmd = PCSENTRY|PCSEXIT;
	premptyset(&ctl.arg.sysset);
	(void) ::write(p->ctlfd, (char *)&ctl, SIZEOF2(long,sysset_t));
	ctl.cmd = PCSET;
	ctl.arg.flags = PR_PTRACE;
	(void) ::write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long));
	return p;
}

void Hostfunc::close(int fd){
	(void) ::close(fd);
}

void Hostfunc::close(Localproc *p){
	Localproc *q;
	struct ctl ctl;

	if (--p->opencnt != 0)
		return;
	if (ISCORE(p))
		coreclose(p);
	else {
		if (p->state.state != UNIX_ERRORED) {
			ctl.cmd = PCSENTRY|PCSEXIT;
			premptyset(&ctl.arg.sysset);
			(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,sysset_t));
			ctl.cmd = PCSET;
			ctl.arg.flags = PR_RLC | PR_FORK;
			(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long));
		}
		close(p->corefd);	p->corefd = -1;
		close(p->stabfd);	p->stabfd = -1;
		close(p->ctlfd);	p->ctlfd = -1;
		close(p->statusfd);	p->statusfd = -1;
	}
	if (p == phead)
		phead = p->next;
	else {
		for (q = phead; q->next != p; q = q->next)
			;
		q->next = p->next;
	}
	delete p;
}

int Hostfunc::getsymtabfd(Localproc *p){ return p->stabfd; }

char *Hostfunc::run(Localproc *p){
	struct ctl ctl;

	ctl.cmd = PCRUN;
	ctl.arg.flags = 0;
	if (write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long)) != SIZEOF2(long,long))
		return "DFCRUN error";
	return 0;
}

char *Hostfunc::stop(Localproc *p){
	struct ctl ctl;

	ctl.cmd = PCSTOP;
	ctl.arg.flags = 0;
	if (write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long)) != SIZEOF2(long,long))
		return "DFCSTOP error";
	return 0;
}

char *Hostfunc::destroy(Localproc *p){
	struct ctl ctl;

	state(p);
	ctl.cmd = PCKILL;
	ctl.arg.flags = SIGKILL;
	if (write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long)) != SIZEOF2(long,long))
		return "DFCKILL error";
	p->state.state = UNIX_ERRORED;
	p->state.code = SIGKILL;
	return 0;
}

void Hostfunc::getstate(Localproc *p, struct Unixstate *s){
	if (!ISCORE(p)) state(p);
	*s = p->state;
}

void Hostfunc::state(Localproc *p){
	struct ctl ctl;

	ctl.cmd = PCTWSTOP;	/* process might not stop */
	ctl.arg.flags = 30;
	(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long));

	if((pread(p->statusfd, &p->s, sizeof(pstatus_t), (off_t)0))!=sizeof(pstatus_t)){
		/*
		 * If the call failed, it is probably  because  the  process
		 * has disappeared. With /proc, there is no way to determine
		 * the exist status or reason the program died, so set it to
		 * make  it look like a signal that cannot really happen was
		 * sent. If  the  server  is  the  parent  of  the  process,
		 * handling  the  SIGCLD  signal  will set the correct state
		 * value.
		 * Otherwise, the setting below remains.
		 */
		if (p->state.state != UNIX_ERRORED) {
			p->state.state = UNIX_ERRORED;
			p->state.code = NSIG;
		}
		return;
	}
	if (!(p->s.pr_flags & PR_STOPPED))
		p->state.state = UNIX_ACTIVE;
	else
		switch(p->s.pr_lwp.pr_why) {
		default:
			p->state.state = UNIX_HALTED;
			break;
		case PR_SIGNALLED:
			p->state.state = UNIX_PENDING;
			break;
		case PR_FAULTED:
			p->state.state = UNIX_BREAKED;
			ctl.cmd = PCCFAULT;
			ctl.arg.flags = 0;
			(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long));
			break;
		}
	p->state.code = p->s.pr_lwp.pr_what;
}

char *Hostfunc::readwrite(Localproc *p, char *buf, unsigned long addr, int r, int w){
	int count, ret;

	if (ISCORE(p))
		return corerw(p, buf, addr, r, w);
	if (addr >= _regaddr)
		return regrw(p, buf, (int)(addr - _regaddr), r, w);
	if (lseek(p->corefd, addr, SEEK_SET) == -1)
		return "lseek on /proc failed";
	if (w) {
		ret = write(p->corefd, buf, w);
		count = w;
	} else {
		ret = read(p->corefd, buf, r);
		count = r;
	}
	if (ret != count)
		 return "read/write on /proc failed";
	return 0;
}

char *Hostfunc::regrw(Localproc *p, char *buf, int offset, int r, int w){
	struct ctl ctl;
	pstatus_t status;
	char *error = 0;
	int wasactive = 0;

	if (p->state.state == UNIX_ACTIVE) {
		stop(p);
		state(p);
		if (p->s.pr_lwp.pr_why == PR_REQUESTED)
			wasactive = 1;
	}
	if((pread(p->statusfd, &status, sizeof(pstatus_t), (off_t)0))!=sizeof(pstatus_t)){
		error =  "DFCGETREGS failed";
		goto out;
	}
	if (w) {
		memcpy((char*)&status.pr_lwp.pr_reg + offset, buf, w);
		ctl.cmd = PCSREG;
		memcpy(&ctl.arg.regset, &status.pr_lwp.pr_reg, sizeof(prgregset_t));
		(void) write(p->ctlfd, &ctl, SIZEOF2(long, prgregset_t));
	} else
		memcpy(buf, (char*)&status.pr_lwp.pr_reg + offset, r);
out:
	if (wasactive)
		run(p);
	return error;
}

char *Hostfunc::setbkpt(Localproc *p, long addr){
	return readwrite(p,(char*)&bkpt_instr,addr,0,sizeof(bkpt_instr));
}

char *Hostfunc::step(Localproc *p){
	struct ctl ctl;

	ctl.cmd = PCRUN;
	ctl.arg.flags = PRSTEP|PRCFAULT;
	if((write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long))) != SIZEOF2(long,long))
		return "DFCSSTEP error";

	return 0;
}

char *Hostfunc::waitstop(Localproc *p){
	char *err = 0;
	struct ctl ctl;
	void (*save)(int);

	save = signal(SIGALRM, alarmcatcher);
	alarm(15);
	ctl.cmd = PCSTOP;
	ctl.arg.flags = 0;
	if((write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long))) != SIZEOF2(long,long))
		err = "timeout waiting for breakpoint";
	alarm(0);
	signal(SIGALRM, save);
	return err;
}

char *Hostfunc::ssig(Localproc *p, long sig){
	struct ctl ctl;

	ctl.cmd = PCSSIG;
	ctl.arg.siginfo.si_signo = sig;
	if((write(p->ctlfd, (char *)&ctl, SIZEOF2(long,siginfo_t))) != SIZEOF2(long,siginfo_t))
		return "DFCSSIG error";
	return 0;
}

char *Hostfunc::csig(Localproc *p){
	int n;
	struct ctl ctl;

	ctl.cmd = PCCSIG;
	ctl.arg.flags = 0;
	if((write(p->ctlfd, (char *)&ctl, (n = SIZEOF2(long,long)))) != n)
		return "DFCCSIG error";
	return 0;
}

char *Hostfunc::sigmask(Localproc *p, long mask){
	int i;
	struct ctl ctl;
	sigset_t sset;

	ctl.cmd = PCSTRACE;
	premptyset(&sset);
	for(i = 1; i <= 32; i++)
		if (mask & bit(i))
			praddset(&sset, i);
	(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,sigset_t));
	return 0;
}

char *Hostfunc::signalname(long sig){
	char *cp;

	switch (sig) {
	case SIGHUP:	cp = "hangup"; break;
	case SIGINT:	cp = "interrupt"; break;
	case SIGQUIT:	cp = "quit"; break;
	case SIGILL:	cp = "illegal instruction"; break;
	case SIGTRAP:	cp = "trace trap"; break;
	case SIGIOT:	cp = "IOT instruction"; break;
	case SIGEMT:	cp = "EMT instruction"; break;
	case SIGFPE:	cp = "floating point exception"; break;
	case SIGKILL:	cp = "kill"; break;
	case SIGBUS:	cp = "bus error"; break;
	case SIGSEGV:	cp = "segmentation violation"; break;
	case SIGSYS:	cp = "bad argument to system call"; break;
	case SIGPIPE:	cp = "write on a pipe with no reader"; break;
	case SIGALRM:	cp = "alarm clock"; break;
	case SIGTERM:	cp = "software termination signal from kill"; break;
	case SIGUSR1:	cp = "user defined signal 1"; break;
	case SIGUSR2:	cp = "user defined signal 2"; break;
	case SIGCLD:	cp = "child status change"; break;
	case SIGPWR:	cp = "power-fail restart"; break;
	case SIGWINCH:	cp = "window size change"; break;
	case SIGURG:	cp = "urgent socket condition"; break;
	case SIGPOLL:	cp = "pollable event occured"; break;
	case SIGSTOP:	cp = "stop"; break;
	case SIGTSTP:	cp = "user stop requested from tty"; break;
	case SIGCONT:	cp = "stopped process has been continued"; break;
	case SIGTTIN:	cp = "background tty read attempted"; break;
	case SIGTTOU:	cp = "background tty write attempted"; break;
	case SIGVTALRM:	cp = "virtual timer expired"; break;
	case SIGPROF:	cp = "profiling timer expired"; break;
	case SIGXCPU:	cp = "exceeded cpu limit"; break;
	case SIGXFSZ:	cp = "exceeded file size limit"; break;
	case SIGWAITING: cp = "reserved for threading code"; break;
	case SIGLWP:	cp = "reserved for threading code"; break;
	case SIGFREEZE:	cp = "CPR special signal"; break;
	case SIGTHAW:	cp = "CPR special signal"; break;
	case SIGCANCEL:	cp = "thread cancellation"; break;
	case SIGLOST:	cp = "resource lost"; break;
	case SIGXRES:	cp = "resource control exceeded"; break;
	case SIGJVM1:	cp = "JVM reserved signal"; break;
	case SIGJVM2:	cp = "JVM reserved signal"; break;
	default:
		cp = procbuffer;
		sprintf(procbuffer, "signal %d", sig);
		break;
	}
	return cp;
}

char *Hostfunc::proctime(Localproc *p){
	sprintf(procbuffer, "%d.%02du %d.%02ds",
		p->s.pr_utime.tv_sec, p->s.pr_utime.tv_nsec/10000000,
		p->s.pr_stime.tv_sec, p->s.pr_stime.tv_nsec/10000000);
	return procbuffer;
}

int Hostfunc::getppid(Localproc *p){
	return (int)p->s.pr_ppid;
}

int Hostfunc::atsyscall(Localproc *p){
	long pc, instr; 
	readwrite(p, (char *)&pc, _regaddr + REG_PC*4, sizeof(pc), 0);
	readwrite(p, (char *)&instr, pc, sizeof(instr), 0);
        return (instr == I386_SYSCALL);
}

char *Hostfunc::exechang(Localproc *p, long add){
	int n;
	pstatus_t status;
	struct ctl ctl;

	if(pread(p->statusfd, &status, sizeof(pstatus_t), (off_t)0) != sizeof(pstatus_t))
		return sf("DFCSTATUS failed: %s", strerror(errno));

	ctl.cmd = PCSEXIT;
	ctl.arg.sysset = status.pr_sysexit;
	if (add)
		praddset(&ctl.arg.sysset, SYS_execve);
	else
		prdelset(&ctl.arg.sysset, SYS_execve);
	(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,sysset_t));
	return 0;
}

/*
 * Expand a path
 */
char *Hostfunc::pathexpand(char *f, char *path, int a){
	static char file[128];
	char *p;

	if (*f != '/'
	    && strncmp(f, "./", 2)
	    && strncmp(f, "../", 3) && path!=0){
		while(*path){
			for(p=file; *path && *path!=':';)
				*p++ = *path++;
			if(p!=file)
				*p++='/';
			if(*path)
				path++;
			(void)strcpy(p, f);
			if (access(file, a) != -1)
				return file;
		}
	}
	if (access(f, a) != -1 )
		return f;
	return 0;
}

/*
 * This function and the function below are for starting new
 * processes from pi.
 */
int Hostfunc::hang(char *cmd){
	char *argv[10], *cp;
	int i, fd;
	char *file;
	Localproc *p;
	int nfiles;
	struct ctl ctl;
	
	i = strlen(cmd);
	if (++i > sizeof(procbuffer)) {
		i = sizeof(procbuffer) - 1;
		procbuffer[i] = 0;
	}
	memcpy(procbuffer, cmd, i);
	argv[0] = cp = procbuffer;
	for(i = 1;;) {
		while(*cp && *cp != ' ')
			cp++;
		if (!*cp) {
			argv[i] = 0;
			break;
		} else {
			*cp++ = 0;
			while (*cp == ' ')
				cp++;
			if (*cp)
				argv[i++] = cp;
		}
	}
	if (!(file = pathexpand(argv[0], getenv("PATH"), 5)))
		return 0;
	hangpid = fork();
	if (!hangpid){
		nfiles = ulimit(UL_GDESLIM,0);
		for (fd = 0; fd < nfiles; ++fd)
			close(fd);
		open("/dev/null", O_RDWR);
		dup2(0, 1);
		dup2(0, 2);
		setpgrp();
		sprintf(procname, "/proc/%05d/ctl", getpid());
		if ((fd = open(procname, O_WRONLY)) < 0)
			exit(1);
		ctl.cmd = PCSEXIT;
		premptyset(&ctl.arg.sysset);
		praddset(&ctl.arg.sysset, SYS_execve);
		(void) ::write(fd, (char *)&ctl, SIZEOF2(long,sysset_t));
		close(fd);
		execv(file, argv);
		exit(0);
	}
	if (hangpid < 0)
		return 0;

	p = new Localproc;

	snprintf(procname, sizeof(procname), "/proc/%05d", hangpid);
        pend = procname + strlen(procname);

	strcpy(pend, "/as");
	if((p->corefd = open(procname, O_RDWR|O_EXCL, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}
	strcpy(pend, "/path/a.out");
	if((p->stabfd = open(procname, O_RDONLY, 0)) < 0){
		perror(argv[0]);
		delete p;
		return 0;
	}
	strcpy(pend, "/ctl");
	if((p->ctlfd = open(procname, O_WRONLY, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}
	strcpy(pend, "/status");
	if((p->statusfd = open(procname, O_RDONLY, 0)) < 0){
		perror(procname);
		delete p;
		return 0;
	}
	p->pid = hangpid;
	p->opencnt = 0;
	p->next = phead;
	phead = p;

	state(p);
	/* If the exec failed - get rid of it */
	if (p->s.pr_lwp.pr_errno) {
		ssig(p, 9);
		state(p);
		close(p);
		return 0;
	}
	ctl.cmd = PCSET;
	ctl.arg.flags = PR_RLC|PR_FORK|PR_BPTADJ;
	(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,long));
	ctl.cmd = PCSEXIT;
	premptyset(&ctl.arg.sysset);
	(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,sysset_t));
	ctl.cmd = PCSFAULT;
	premptyset(&ctl.arg.fltset);
	praddset(&ctl.arg.fltset, FLTBPT);
	praddset(&ctl.arg.fltset, FLTTRACE);
	praddset(&ctl.arg.fltset, FLTWATCH);
	(void) write(p->ctlfd, (char *)&ctl, SIZEOF2(long,fltset_t));
	return hangpid;
}

Localproc *Hostfunc::coreopen(char *corep, char *symtabp){
	Localproc *p;
	Coreseg *cs;
	int i, n;
	int symtabfd, corefd = -1;
	ElfNhdr *note;
	uchar *np, *end;
	long *lp;
	int statusfound = 0;

	if ((symtabfd = open(symtabp, 0)) < 0) {
		perror(symtabp);
		return 0;
	}

	e[STAB] = new Elf();
	if(e[STAB]->open(symtabp) < 0)
		return 0;

	e[CORE] = new Elf();
	if(e[CORE]->open(corep) < 0)
		return 0;

	p = new Localproc;
	p->pid = --corepid;
	p->opencnt = 1;
	p->next = phead;
	phead = p;
	p->corefd = e[CORE]->fd;
	p->stabfd = e[STAB]->fd;
	p->nseg = e[CORE]->nseg(ElfProgLoad);

	ElfProg *notep = e[CORE]->findprog(ElfProgNote);
	if(notep == 0)
		goto fatal;
	e[CORE]->map(notep);
	n = notep->filesz;
	end = notep->base+n;
	for(np = notep->base; np < end; ) {
		note = (ElfNhdr*)np;
		switch(note->type){
		case NotePstatus:
                        p->s = *(pstatus_t *)ELFNOTE_DESC(note);
			statusfound = 1;
			break;
		}
		np = ELFNOTE_NEXT(note);
	}
	if (!statusfound)
		goto fatal;

	p->nseg++;			/* One for text segment from symtab file */
	cs = p->seg = new Coreseg[p->nseg];
	for(i = 0; i < e[CORE]->nprog; i++) {
		if (e[CORE]->prog[i].type == PTLOAD && e[CORE]->prog[i].filesz) {
			cs->start = e[CORE]->prog[i].vaddr;
			cs->end = cs->start + e[CORE]->prog[i].filesz;
			cs->offset = e[CORE]->prog[i].offset;
			cs->fd = e[CORE]->fd;
			cs++;
		}
	}
	for(i = 0; i < e[STAB]->nprog; i++) {
		if (e[STAB]->prog[i].type == PTLOAD && e[STAB]->prog[i].filesz &&
		    e[STAB]->prog[i].flags == (PFREAD|PFEXEC)) {
			cs->start = e[STAB]->prog[i].vaddr;
			cs->end = cs->start + e[STAB]->prog[i].filesz;
			cs->offset = e[STAB]->prog[i].offset;
			cs->fd = e[STAB]->fd;
			cs++;
			break;
		}
	}
	p->state.state = UNIX_PENDING;
	p->state.code = p->s.pr_lwp.pr_cursig;
	return p;
fatal:
	delete p;
	e[CORE]->close();
	e[STAB]->close();
	return 0;
}

void Hostfunc::coreclose(Localproc *p){
	e[CORE]->close();
	close(p->corefd);
	close(p->stabfd);
	delete [] p->seg;
	delete e[CORE];
}

char *Hostfunc::coreregrw(Localproc *p, char *buf, int offset, int r, int w){
	dbregs reg;

        memcpy((char*)&reg.r[0], (char*)&p->s.pr_lwp.pr_reg, sizeof(gregset_t));
	if (w) {
		memcpy((char*)&reg + offset, buf, w);
	} else {
		memcpy(buf, (char*)&reg + offset, r);
	}
	return 0;
}

char *Hostfunc::corerw(Localproc *p, char *buf, unsigned long addr, int r, int w){
	Coreseg *cs, *csend;

	if (addr >= _regaddr)
		return coreregrw(p, buf, (int)(addr - _regaddr), r, w);
	cs = p->seg;
	csend = &cs[p->nseg];
	for (; cs < csend; cs++)
		if (cs->start <= addr && addr < cs->end)
			break;
	if (cs == csend)
		return "corerw:invalid address";
	addr = addr - cs->start + cs->offset;
	if (lseek(cs->fd, addr, SEEK_SET) == -1)
		return "corerw:lseek failed";
	if (w) {
		if (write(cs->fd, buf, w) != w)
			return "corerw:write error";
	} else {
		if (read(cs->fd, buf, r) != r)
			return "corerw:read error";
	}
	return 0;
}
