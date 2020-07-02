/*
 * Copyright (c) 1990-1992 AT&T All Rights Reserved
 *	This is unpublished proprietary source code of AT&T
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *
 * Pi Unix driver for Linux x86_64 machines running RHEL 7.
 *	This file uses ptrace to examine running processes.
 *
 * N. L. Hunt	21/6/20
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ulimit.h>
#include <setjmp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/ucontext.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <linux/a.out.h>
#include "unix.h"
#include "elf.h"
#include "hostfunc.h"

#ifndef ALIGN
# define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
# define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#endif

#define	I386SYSCALL	0x80
#define I386BPT		0xCC		/* INT 3 */
#define BPTFLT		3
#define TRAPLEN		1
#define TRAPINST	0xCC

#if defined(__i386)
#define TRAPMASK	0xFFFFFF00
#elif defined(__x86_64)
#define TRAPMASK	0xFFFFFFFFFFFFFF00
#endif

#define	bit(x)		(1<<(x-1))
#define	STEPWAIT	15
#define	ISCORE(p)	((p)->pid < -1)
#define REGSIZE		(NGREG * 4)
#define REG_PC		REG_EIP

static __ptrace_request ptracecmd[NREQ] = {
	PTRACE_TRACEME,
	PTRACE_PEEKTEXT,
	PTRACE_PEEKUSER,
	PTRACE_POKETEXT,
	PTRACE_POKEUSER,
	PTRACE_CONT,
	PTRACE_KILL,
	PTRACE_SINGLESTEP,
	PTRACE_GETREGS,
	PTRACE_SETREGS,
	PTRACE_ATTACH,
	PTRACE_DETACH
};

extern Hostfunc *hfn;

/*
 * Register layout transmitted to pi
 */
typedef struct dbregs dbregs;

struct dbregs {
	int r[NGREG];
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

Hostfunc::Hostfunc(){
	first = 1;
	corepid = -1;
	_regaddr = -REGSIZE - 4;
	bkptinstr = I386BPT;
	pscmds[0] = 0;
	pscmds[1] = "/usr/bin/ps -a ";
	pscmds[2] = "/usr/bin/ps -e ";
	pscmds[3] = 0;
	ticks = sysconf(_SC_CLK_TCK);
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
#ifdef FIXED
		signal(SIGCHLD, waithandler);
#endif
		first = 0;
	}
	return "Linux RHEL 7 i386";
}

int Hostfunc::sigmaskinit(){
	return	bit(SIGILL)|bit(SIGINT)|bit(SIGTRAP)|bit(SIGIOT)|
		bit(SIGFPE)|bit(SIGBUS)|bit(SIGSEGV)|bit(SIGSYS)|
		bit(SIGPIPE);
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
	char buf[64], *end;
	Localproc *p = phead;
	struct exec exec;

	if (pid < 0)
		return 0;
	if ((p = pidtoproc(pid))) {
		p->opencnt++;
		return p;
	}

	p = new Localproc;
	snprintf(buf, sizeof(buf), "/proc/%05d", pid);
        end = buf + strlen(buf);
	(void) strcpy(end, "/stat");
	p->statfile = StrDup(buf);
	kill(pid, SIGCONT);
	if (ptrace(p, R_ATTACH, 0, 0, 0)) {
		delete p;
		return 0;
	}
	p->sigmsk = sigmaskinit();
	p->pid = pid;
	p->opencnt = 1;
	p->next = phead;
	phead = p;
	procwait(p, 0);
	return p;
}

void Hostfunc::close(int fd){
	(void) ::close(fd);
}

void Hostfunc::close(Localproc *p){
	Localproc *q;

	if (--p->opencnt != 0)
		return;
	if (ISCORE(p))
		coreclose(p);
	else if (p->state.state == UNIX_ACTIVE) {
		stop(p);
		procwait(p, WAIT_PCFIX);
		ptrace(p, R_DETACH, 0, 0, 0);
	} else if (p->state.state != UNIX_ERRORED) {
		kill(p->pid, SIGSTOP);
		ptrace(p, R_DETACH, 0, 0, 0);
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

extern "C" void alarmcatcher(int s){}

char *Hostfunc::waitstop(Localproc *p){
	procwaitstop(p, WAIT_PCFIX);
	if (p->state.state == UNIX_BREAKED)
		return 0;
	else
		return "timeout waiting for breakpoint";
}

int Hostfunc::procwaitstop(Localproc *p, int flag){
	if (!procwait(p, flag) || p->state.state != UNIX_HALTED)
		return 1;
	return 0;
}

int Hostfunc::procwait(Localproc *p, int flag){
	int tstat;
	int cursig;

again:
	if (p->pid != waitpid(p->pid, &tstat, (flag&WAIT_POLL)? WNOHANG: 0))
		return 0;

	if (flag & WAIT_DISCARD)
		return 1;
	if (WIFSTOPPED(tstat)) {
		cursig = WSTOPSIG(tstat);
		if (cursig == SIGSTOP)
			p->state.state = UNIX_HALTED;
		else if (cursig == SIGTRAP) {
			p->state.state = UNIX_BREAKED;
			if (flag & WAIT_PCFIX) {
				ptrace(p, R_GETREGS, 0, 0, (long*)&regs);
				regs.eip -= 1;					/* bkptsize */
				ptrace(p, R_SETREGS, 0, 0, (long*)&regs);
			}
		} else {
			if (p->state.state == UNIX_ACTIVE &&
			    !(p->sigmsk&bit(cursig))) {
				ptrace(p, R_CONT, 0, 0, (long*)cursig);
				goto again;
			}
			else {
				p->state.state = UNIX_PENDING;
				p->state.code = cursig;
			}
		}
	} else {
		p->state.state = UNIX_ERRORED;
		p->state.code = WEXITSTATUS(tstat) & 0xFFFF;
	}
	return 1;
}

char *Hostfunc::run(Localproc *p){
	int cursig = 0;

	if (p->state.state == UNIX_PENDING)
		cursig = p->state.code;
	ptrace(p, R_CONT, 0, 0, (long*)cursig);
	p->state.state = UNIX_ACTIVE;
	return 0;
}

char *Hostfunc::stop(Localproc *p){
	kill(p->pid, SIGSTOP);
	return 0;
}

char *Hostfunc::destroy(Localproc *p){
	if (p->state.state == UNIX_ERRORED)
		return "already dead";
	if (p->state.state == UNIX_ACTIVE) {
		stop(p);
		procwait(p, WAIT_PCFIX);
	}
	kill(p->pid, SIGKILL);
	procwait(p, WAIT_DISCARD|WAIT_POLL);
	p->state.state = UNIX_ERRORED;
	p->state.code = SIGKILL;
	return 0;
}

void Hostfunc::getstate(Localproc *p, Unixstate *s){
	if (!ISCORE(p) && p->state.state == UNIX_ACTIVE)
		procwait(p, WAIT_POLL|WAIT_PCFIX);
	*s = p->state;
}

char *Hostfunc::readwrite(Localproc *p, char *buf, ulong addr, int r, int w){
	char *err = 0;
	int wasactive = 0;

	if (ISCORE(p))
		return corerw(p, buf, addr, r, w);
	if (p->state.state == UNIX_ACTIVE) {
		stop(p);
		procwait(p, WAIT_PCFIX);
		if (p->state.state != UNIX_HALTED) {
			ptrace(p, R_CONT, 0, 0, 0);
			procwaitstop(p, WAIT_DISCARD);
		} else
			wasactive = 1;
	}
	if (addr >= _regaddr)
		err = regrw(p, buf, (int)(addr - _regaddr), r, w);
	else if (ptrace(p, w? R_WRITE: R_READ, (int)addr, w? w: r, (long*)buf))
		err = "ptrace failed";
	if (wasactive)
		run(p);
	return err;
}

char *Hostfunc::regrw(Localproc *p, char *buf, int offset, int r, int w){
	if (offset == REGSIZE)	{ /* %g0 */
		if (r) bzero(buf, 4);
		return 0;
	}
	if (ptrace(p, R_GETREGS, 0, 0, (long*)&regs))
		return "PTRACE_GETREGS failed";
	if (w) {
		bcopy(buf, (char*)&regs + offset, w);
		if (ptrace(p, R_SETREGS, 0, 0, (long*)&regs))
			return "PTRACE_SETREGS failed";
	} else
		bcopy((char*)&regs + offset, buf, r);
	return 0;
}

char *Hostfunc::liftbpt(Localproc *p, long addr, char *saved){
	return readwrite(p, saved, addr, 0, 1);
}

char *Hostfunc::laybpt(Localproc *p, long addr){
	return readwrite(p, (char*)&bkptinstr, addr, 0, 1);
}

char *Hostfunc::step(Localproc *p){
	ptrace(p, R_SINGLESTEP, 0, 0, 0);
	procwait(p, 0);
	return 0;
}

int Hostfunc::ptrace(Localproc *p, enum ptracereq req, long addr, int n, long *data){
	int ok = 1, bytes = 0;
	char *a2 = (char*) data;
	int i = 0, j, k;
	union ptraceval buf;
	enum __ptrace_request cmd = ptracecmd[req];

	switch (cmd) {
		case PTRACE_CONT:
		case PTRACE_KILL:
		case PTRACE_SINGLESTEP:
		case PTRACE_ATTACH:
		case PTRACE_DETACH:
		case PTRACE_SETREGS:
		case PTRACE_GETREGS:
			return ::ptrace(cmd, p->pid, NULL, data);
	}
	if (req == R_READ) {
		for(i=0, j=n, errno=0; j >= WORDSIZE; ++i, j-=WORDSIZE, errno=0){
			buf.word = ::ptrace(PTRACE_PEEKDATA, p->pid, addr+i*WORDSIZE, NULL);
			if(errno) goto Error;
#define BCOPY
#ifndef BCOPY
			for(k = 0; k < WORDSIZE; ++k) a2[k] = buf.byte[k];
#else
			bcopy(buf.byte, a2, WORDSIZE);
#endif
			a2 += WORDSIZE;
		}
		if( j ){
			buf.word = ::ptrace(PTRACE_PEEKDATA, p->pid, addr+i*WORDSIZE, NULL);
			if(errno) goto Error;
#ifndef BCOPY
			for(k = 0; k < j; ++k) a2[k] = buf.byte[k];
#else
			bcopy(buf.byte, a2, j);
#endif
		}
	} else {
		for(i=0, j=n, errno=0; j >= WORDSIZE; ++i, j-=WORDSIZE, errno=0){
#ifndef BCOPY
			for(k = 0; k < WORDSIZE; ++k) buf.byte[k] = a2[k];
#else
			bcopy(a2, buf.byte, WORDSIZE);
#endif
			::ptrace(PTRACE_POKEDATA, p->pid, addr+i*WORDSIZE, buf.word);
			if(errno) goto Error;
			a2 += WORDSIZE;
		}
		if( j ){
			/* read word at this address first and splice */
			buf.word = ::ptrace(PTRACE_PEEKDATA, p->pid, addr+i*WORDSIZE, 0);
			if(errno) goto Error;
#ifndef BCOPY
			for(k = 0; k < j; ++k) buf.byte[k] = a2[k];
#else
			bcopy(a2, buf.byte, j);
#endif
			::ptrace(PTRACE_POKEDATA, p->pid, addr+i*WORDSIZE, buf.word);
			if(errno) goto Error;
		}
	}
	procwait(p, WAIT_POLL|WAIT_DISCARD);
	return 0;
Error:
	return errno;
}

char *Hostfunc::ssig(Localproc *p, int sig){
	kill(p->pid, sig);
	return 0;
}

char *Hostfunc::csig(Localproc *p){
	if (p->state.state == UNIX_PENDING)
		p->state.state = UNIX_HALTED;
	return 0;
}

char *Hostfunc::sigmask(Localproc *p, int mask){
	p->sigmsk = mask;
	return 0;
}

char *Hostfunc::signalname(int sig){
	char *cp;

	switch (sig) {
	case SIGHUP:	cp = "hangup"; break;
	case SIGINT:	cp = "interrupt"; break;
	case SIGQUIT:	cp = "quit"; break;
	case SIGILL:	cp = "illegal instruction"; break;
	case SIGTRAP:	cp = "trace trap"; break;
	case SIGIOT:	cp = "IOT instruction"; break;
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
	default:
		cp = procbuffer;
		sprintf(procbuffer, "signal %d", sig);
		break;
	}
	return cp;
}

void readint(FILE *fp, int* x)	{ fscanf(fp, "%lld ", x);	}
void readstr(FILE *fp, char* x)	{ fscanf(fp, "%s ", x);	}
void readchr(FILE *fp, char* x)	{ fscanf(fp, "%c ", x);	}

char *Hostfunc::proctime(Localproc *p){
	FILE *fp;
	if((fp = fopen(p->statfile, "r")) == NULL)
		return "";

	readint(fp, &pstat.pid);
	readstr(fp, pstat.aout);
	readchr(fp, &pstat.state);
	readint(fp, &pstat.ppid);
	readint(fp, &pstat.pgrp);
	readint(fp, &pstat.sid);
	readint(fp, &pstat.ttynr);
	readint(fp, &pstat.ttypgid);
	readint(fp, (int*)&pstat.flags);
	readint(fp, (int*)&pstat.minflt);
	readint(fp, (int*)&pstat.cminflt);
	readint(fp, (int*)&pstat.majflt);
	readint(fp, (int*)&pstat.cmajflt);
	readint(fp, &pstat.utime);
	readint(fp, &pstat.stime);
	fclose( fp );
#define NTICKS(x) (((double)x)/ticks)
	sprintf(procbuffer, "%fu %fs", NTICKS(pstat.utime), NTICKS(pstat.stime));
	return procbuffer;
}

int Hostfunc::getppid(Localproc *p){
	return (int)p->prstat.pr_ppid;
}

int Hostfunc::atsyscall(Localproc *p){
	long pc, instr; 
	readwrite(p, (char *)&pc, _regaddr + REG_PC*4, sizeof(pc), 0);
	readwrite(p, (char *)&instr, pc, sizeof(instr), 0);
        return (instr == I386SYSCALL);
}

void Hostfunc::bcopy(char *s1, char *s2, int len){
	register int n;

	if ((n = len) <= 0)
		return;

	if ((s1 < s2) && (n > abs(s1 - s2))) {		/* overlapped */
		s1 += (n - 1);
		s2 += (n - 1);
		do
			*s2-- = *s1--;
		while (--n);
	} else {					/* normal */
		 do
			*s2++ = *s1++;
		while (--n);
	}
}

void Hostfunc::bzero(char *sp, int len){
	register int n;

	if ((n = len) <= 0)
		return;
	do
		*sp++ = 0;
	while (--n);
}


char *Hostfunc::exechang(Localproc *p, int add){ return 0; }

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
	int i;
	Localproc *p;
	struct exec exec;
	struct rlimit rlim;
	
	i = strlen(cmd);
	if (++i > sizeof(procbuffer)) {
		i = sizeof(procbuffer) - 1;
		procbuffer[i] = 0;
	}
	bcopy(cmd, procbuffer, i);
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
	hangpid = fork();
	if (!hangpid){
		int fd, nfiles = 20;
		if(getrlimit(RLIMIT_NOFILE, &rlim))
			nfiles = rlim.rlim_cur;
#define DEBUG
#ifdef DEBUG
		for( fd = 0; fd < nfiles; ++fd )
			close(fd);
		open("/dev/null", 2);
		dup2(0, 1);
		dup2(0, 2);
#endif
		setpgid(0, 0);
		::ptrace(PTRACE_TRACEME, 0, 0, 0);
		execvp(argv[0], argv);
		exit(0);
	}
	if (hangpid < 0)
		return 0;
	p = new Localproc;
	if (!p) {
		kill(9, hangpid);
		return 0;
	}
	p->sigmsk = sigmaskinit();
	p->pid = hangpid;
	if (!procwait(p, 0)) {
		delete p;
		return 0;
	}
	if (p->state.state == UNIX_BREAKED)
		p->state.state = UNIX_HALTED;
	p->opencnt = 0;
	p->next = phead;
	phead = p;
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
		case NotePrstatus:
                        p->prstat = *(prstatus_t *)ELFNOTE_DESC(note);
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
	p->state.code = p->prstat.pr_cursig;
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

        memcpy((char*)&reg.r[0], (char*)&p->prstat.pr_reg, sizeof(gregset_t));
	if (w) {
		memcpy((char*)&reg + offset, buf, w);
	} else {
		memcpy(buf, (char*)&reg + offset, r);
	}
	return 0;
}

char *Hostfunc::corerw(Localproc *p, char *buf, ulong addr, int r, int w){
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
