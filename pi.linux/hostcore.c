#include <sys/procfs.h>
#include <sys/utsname.h>
#include "core.h"
#include "process.h"
#include "master.h"
#include "sigmask.h"
#include "frame.h"
#include "memory.h"
#include "symtab.h"
#include "symbol.h"
#include "srcdir.h"
#include "asm.h"
#include "bpts.h"
#include "expr.h"
#include "hostcore.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <utmp.h>
SRCFILE("hostcore.c")

static char sccsid[ ] = "%W%\t%G%";

Behavs HostCore::behavetype()		{ return behavs(); }
char *HostCore::eventname()		{ return signalname(state.code); }
int HostCore::atsyscall()		{ return hfn->atsyscall(localp); }
char *HostCore::signalname(long sig)	{ return hfn->signalname(sig); }
char *HostCore::resources()		{ return hfn->proctime(localp); }
char *HostCore::run()			{ return hfn->run(localp); }
char *HostCore::stop()			{ return hfn->stop(localp); }
char *HostCore::destroy()		{ return hfn->destroy(localp); }
long HostCore::regaddr()		{ return hfn->regaddr(); }
long HostCore::scratchaddr()		{ return hfn->scratchaddr(); }
int HostCore::nsig()			{ return hfn->nsig(); }
long HostCore::signalmaskinit()		{ return hfn->sigmaskinit(); }
int HostCore::exechangsupported()	{ return hfn->exechangsupported(); }
char *HostCore::signalsend(long sig)	{ return hfn->ssig(localp, sig); }
char *HostCore::signalclear()		{ return hfn->csig(localp); }
char *HostCore::exechang(long e)	{ return hfn->exechang(localp, e); }
char *HostCore::signalmask(long m)	{ return hfn->sigmask(localp, m); }

char *HostCore::problem(){
	static char buf[80];
	long sig;

	if (state.code & 0xFF) {
		sig = state.code & 0x7F;
		sprintf(buf, "died from signal %d (%s)", sig, signalname(sig));
	} else
		sprintf(buf, "exited with status %d", (state.code >> 8) & 0xFF);
	return buf;
}

char *HostCore::liftbpt(Trap *t){		/* taken from core.c */
	if( behavs() == ERRORED || !bptsize) return 0;
	return hfn->liftbpt(localp, t->stmt->range.lo, t->saved);
}

char *HostCore::laybpt(Trap *t){
	if (bptsize && read(t->stmt->range.lo, t->saved, bptsize))
		return "laybpt: read failed";
	return hfn->laybpt(localp, t->stmt->range.lo);
}

Behavs HostCore::behavs(){
	hfn->getstate(localp, &state);
	switch (state.state) {
	case UNIX_HALTED:  return HALTED;
	case UNIX_ACTIVE:  return ACTIVE;
	case UNIX_BREAKED: return BREAKED;
	case UNIX_PENDING: return PENDING;
	case UNIX_ERRORED:
	default:
			return ERRORED;
	}
}

char *HostCore::behavsnam(){
	switch (behavs()) {
	case HALTED:	return "HALTED";
	case ACTIVE:	return "ACTIVE";
	case BREAKED:	return "BREAKED";
	case PENDING:	return "PENDING";
	case UNIX_ERRORED:
	default:
			return "ERRORED";
	}
}

void HostCore::close(){
	hfn->close(localp);
	Core::close();
}

char *HostCore::open(){
	char *s = stabpath();
	if (s) {
		stabfd = ::open(s, 0);
		if (stabfd < 0)
			return SysErr("symbol tables: ");
	} else if (!hfn->stabfdsupported())
		return "symbol table file must be specified";
	char *p = procpath();
	if (alldigits(p)) {
		int procid;
		sscanf(p, "%d", &procid);
		_online = 1;
		if (!(localp = hfn->open(procid)))
			return "can't open process";
	} else if (!(localp = hfn->coreopen(p, s)))
			return "can't open core dump";
	if (!s)
		stabfd = hfn->getsymtabfd(localp);
	if(stabfstat() < 0)
		return strerror(errno);

	newSymTab();
	_symtab->read();
	behavs();
	return 0;
}

char *HostCore::reopen(char *newprocpath, char *newstabpath){
	Localproc *newp;
	char *err = 0;
	int compstabfd;

	if( !online() || (newprocpath && !alldigits(newprocpath)) )
		return "reopen core not implemented";
	int procid;
	sscanf(newprocpath, "%d", &procid);
	if (!(newp = hfn->open(procid)))
		return "can't connect to new process";
	if (newstabpath || !hfn->stabfdsupported())
		compstabfd = ::open(newstabpath, 0);
	else
		compstabfd = hfn->getsymtabfd(newp);
	struct stat compstabstat;
	if( compstabfd < 0 || ::fstat(compstabfd, &compstabstat) )
		err = "symbol table error";
	else if( compstabstat.st_mtime != stabstat.st_mtime )
		err = "symbol tables differ (modified time)";
	else if( compstabstat.st_size != stabstat.st_size )
		err = "symbol tables differ (file size)";
	if (compstabfd >= 0)
		::close(compstabfd);
	if (err) {
		hfn->close(newp);
		return err;
	}
	hfn->close(localp);
	localp = newp;
	behavs();
	return 0;
}

char *HostCore::readwrite(long offset, char *buf, int r, int w){
	return hfn->readwrite(localp, buf, offset, r, w);
}

const int	STEPWAIT = 15;
char *HostCore::dostep(long lo, long hi, int sstep){
	char *error;
	time_t time0;
	long fp0, pcs;

	time0 = ::time((time_t*)0);
	for(fp0 = fp(), pcs = pc();;){
		if( hi && atjsr(pcs) ) {
			error = stepoverjsr();
			goto next;
		}
		if (sstep)
			error = hfn->step(localp);
		else {
			error = run();
			if (!error) {
				error = hfn->waitstop(localp);
				if (error)
					stop();
			}
		}
		if( !error && behavetype() != BREAKED)
			return "single step error";
next:
		if( error ) return error;
		if( !hi || (pcs = pc()) < lo || pcs >= hi
		 || ((stackdir == GROWDOWN ? (fp() > fp0) : (fp() < fp0))
		     && !atreturn(pcs)) )
			return 0;
		if( ::time((time_t*)0) > time0+STEPWAIT )
			return sf("single step timeout (%d secs)", STEPWAIT);
	}
}
