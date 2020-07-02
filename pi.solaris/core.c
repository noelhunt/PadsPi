#include "process.h"
#include "frame.h"
#include "symtab.h"
#include "symbol.h"
#include "core.h"
#include "master.h"
#include "bpts.h"
SRCFILE("core.c")

static char sccsid[ ] = "%W%\t%G%";

static MemLayout hostmemlayout;

Cslfd::Cslfd(long l)		  { init(l, 0.0); }	
Cslfd::Cslfd(double d)		  { init(0,   d); }
Cslfd::Cslfd(int i)		  { init(i, 0.0); }
Cslfd::Cslfd()			  { init(0, 0.0); }
char *Cslfd::floaterror()	  { return flterr; }

void Cslfd::init(long l, double d){
	flterr = 0;
	flt = dbl =d;
	lng = l;
	chr = sht = (short)l;
}

Core::Core(Process *p, Master *m){
	_process = p;
	_symtab = (m && m->core) ? m->core->_symtab : 0;
	corefd = stabfd = -1;
	_online = 0;
	behavs_problem = "?";
}

int Core::disc()			{ return U_CORE;		 }
char *Core::read(long l,char *b,int n)	{ return readwrite(l,b,n,0);	}
char *Core::write(long l,char *b,int n)	{ return readwrite(l,b,0,n);	}
char *Core::readwrite(long, char*, int, int) { return "readwrite"; }
char *Core::step(long lo, long hi)	{ return dostep(lo,hi,1);	}
char *Core::dostep(long, long, int) 	{ return "dostep"; 		}
char *Core::docall(long,int)		{ return "docall";		}
int Core::fpvalid(long fp)		{ return fp != 0;		}
Behavs Core::behavetype()		{ return (Behavs)0;		}
long Core::scratchaddr()		{ return 0;			}
int Core::nregs()			{ return 0;			}
long Core::regaddr()			{ return 0;			}
int Core::atjsr(long)			{ return 0;			}
int Core::atreturn(long)		{ return 0;			}
int Core::atsyscall()			{ return 0;			}
long Core::instrafterjsr()		{ return 0;			}
long Core::callingpc(long)		{ return 0;			}
long Core::callingfp(long)		{ return 0;			}
void Core::newSymTab(long)		{				}
int Core::nsig()			{ return 0;			}
char *Core::signalclear()		{ return "signalclear";		}
char *Core::signalsend(long)		{ return "signalsend";		}
char *Core::signalmask(long)		{ return "signalmask";		}
char *Core::signalname(long)		{ return "signalname";		}
long Core::signalmaskinit()		{ return 0;			}
char *Core::exechang(long)		{ return "exechang";		}
int Core::exechangsupported()		{ return 0;			}

long Core::apforcall(int)		{ return 0;			}
long Core::returnregloc()		{ return 0;			}
int Core::corefstat()			{ return fstat(corefd,&corestat); }
int Core::stabfstat()			{ return fstat(stabfd,&stabstat); }
Asm *Core::newAsm()			{ return 0;			}
Behavs Core::behavs()			{ return (Behavs)0;		}
Cslfd *Core::peekcode(long l)		{ return peek(l);		}
Frame Core::frameabove(long)		{ Frame f; return f;		}
Process *Core::process()		{ return _process;		}
SymTab *Core::symtab()			{ return _symtab;		}
char *Core::special(char*,long)		{ return "special";		}
char *Core::destroy()			{ return "destroy";		}
char *Core::eventname()			{ return "eventname";		}
char *Core::laybpt(Trap*)		{ return "laybpt";		}
char *Core::open()			{ return "open";		}
char *Core::procpath()			{ return _process->procpath;	}
char *Core::readcontrol()		{ return "readcontrol";		}
char *Core::regname(int)		{ return "regname";		}
char *Core::regpoke(int r,long l)	{ return poke(regloc(r), l, 4);	}
char *Core::reopen(char*,char*)		{ return "reopen";		}
char *Core::resources()			{ return "";			}
char *Core::run()			{ return "run";			}
char *Core::stabpath()			{ return _process->stabpath;	}
char *Core::stepprolog()		{ return 0;			}
char *Core::stop()			{ return "stop";		}
int Core::REG_AP()			{ return 0;			}
int Core::REG_FP()			{ return 0;			}
int Core::REG_PC()			{ return 0;			}
int Core::REG_SP()			{ return 0;			}
char *Core::specialop(char*) 		{ return 0;			}
int Core::event()			{ return 0;			}
int Core::online()			{ return _online;		}
long Core::ap()				{ return regpeek(REG_AP());	}
long Core::fp()				{ return regpeek(REG_FP());	}
long Core::pc()				{ return regpeek(REG_PC());	}
long Core::sp()				{ return regpeek(REG_SP());	}
long Core::regpeek(int r)		{ return peek(regloc(r))->lng;	}
long Core::saved(Frame*,int,int)	{ return 0;			}
int Core::argalign()			{ return 4;			}
int Core::argdir()			{ return 1;			}
int Core::argstart()			{ return 0;			}

int Core::instack(long curfp, long prevfp){
	return fpvalid(curfp) &&
	    (stackdir == GROWDOWN ? (curfp > prevfp) : (curfp < prevfp));
}

char *Core::liftbpt(Trap *t){
	if( behavs() == ERRORED || !bptsize) return 0;
	return write(t->stmt->range.lo, t->saved, bptsize);
}

char *Core::problem(){
	return sf("unanticipated problem: %s", behavs_problem);
}

/*
 * The routines peek, poke, and pokedbl handle MSBFIRST  and
 * LSBFIRST  memory  layouts.   They  assume  floating point
 * formats use the same byte ordering schemes as longs.
 */
#define CYCLE 4
Cslfd *Core::peek(long loc, Cslfd *fail){
	static int i;
	static Cslfd *c, *f;
	long *lp;
	union Peekdata {
		double d;
		unsigned char raw[8];
	} pd;
	unsigned char *raw = pd.raw;

	if( read(loc, (char*)raw, 8) ) {
		if( fail != PEEKFAIL ) return fail;
		if( !f ) f = new Cslfd;
		return f;
	}
	if( !c ) {
		c = new Cslfd[CYCLE];
		long tmp = 1;
		if (*(char*)&tmp)
			hostmemlayout = LSBFIRST;
		else
			hostmemlayout = MSBFIRST;
	}
	Cslfd *p = c+(i++,i%=CYCLE);
	p->chr = raw[0];
	p->flterr = 0;
	if (hostmemlayout == memlayout) {
		p->sht = *(short *)raw;
		p->lng = *(long *)raw;
		p->flt = *(float *)raw;
		p->dbl = *(double *)raw;
	} else {
		if (memlayout == MSBFIRST) {
			p->sht = raw[0]<<8 | raw[1];
			p->lng = raw[0]<<24 | raw[1]<<16 | raw[2]<<8 | raw[3];
			lp = (long *)&p->dbl;
			*lp++ = raw[4]<<24 | raw[5]<<16 | raw[6]<<8 | raw[7];
			*lp = p->lng;
		} else {
			p->sht = raw[1]<<8 | raw[0];
			p->lng = raw[3]<<24 | raw[2]<<16 | raw[1]<<8 | raw[0];
			lp = (long *)&p->dbl;
			*lp++ = raw[7]<<24 | raw[6]<<16 | raw[5]<<8 | raw[4];
			*lp = p->lng;
		}
		*(long *)&p->flt = p->lng;
	}
	return p;
}

char *Core::peekstring(long loc, char *fail){
	static char buf[256];
	char *error = read(loc, buf, 250);
	if( error )
		return fail ? fail : strcpy(buf,error);
	return buf;
}

char *Core::poke(long l, long d, int n){
	unsigned char buf[4];
	switch (n) {
	case 1:
		buf[0] = (unsigned char)d;
		break;
	case 2:
		if (memlayout == MSBFIRST) {
			buf[0] = (unsigned char)(d >> 8);
			buf[1] = (unsigned char)d;
		} else {
			buf[1] = (unsigned char)(d >> 8);
			buf[0] = (unsigned char)d;
		}
		break;
	case 4:
		if (memlayout == MSBFIRST) {
			buf[0] = (unsigned char)(d >> 24);
			buf[1] = (unsigned char)(d >> 16);
			buf[2] = (unsigned char)(d >> 8);
			buf[3] = (unsigned char)d;
		} else {
			buf[3] = (unsigned char)(d >> 24);
			buf[2] = (unsigned char)(d >> 16);
			buf[1] = (unsigned char)(d >> 8);
			buf[0] = (unsigned char)d;
		}
		break;
	default:
		return "Core poke error";
	}
	return write(l, (char*)buf, n);
}

char *Core::pokedbl(long l, double d, int n){
	float f;
	char buf[8];
	char *from, *to;
	if (n == 4) {	// float
		f = d;
		from = (char*)&f;
	} else		// double
		from = (char*)&d;
	if (hostmemlayout == memlayout)
		return write(l, from, n);
	for (to = &buf[n]; to > buf; )
		*--to = *from++;
	return write(l, buf, n);
}

long Core::regloc(int r, int sz){
	if (r >= 0 && r < nregs()) {
		long ret = 4 * r + regaddr();
		if (memlayout == MSBFIRST && sz && sz < 4)
			ret += 4 - sz;
		return ret;
	}
	return 0;
}

CallStk *Core::callstack(){
	long size;
	long fpcache[1000];
	long *fpp = fpcache;
	*fpp = fp();
	if( !fpvalid(*fpp))
		return (CallStk *)0;
	for( size = 1; size<1000; ++size ){
		long fpnext = callingfp(*fpp);
		if( !instack(fpnext, *fpp) )
			break;
		*++fpp = fpnext;
	}
	CallStk *c = new CallStk(size, this);
	long _pc = pc();
	long i = 0;
	for( fpp = fpcache;; fpp++ ){
		c->fpf[i].fp = *fpp;
		c->fpf[i].func = (Func*) _symtab->loctosym(U_FUNC, _pc);
		if (++i == size)
			break;
		_pc = callingpc(*fpp);
	}
	return c;
}

char *Core::popcallstack(){
	char *error;
	static Trap *t;
	long fp0, fpt;

	if(!t)
		t = new Trap(new Stmt(0,0,0),0);
	if( behavetype() == ACTIVE )
		return "pop callstack: process not stopped";
	if( atsyscall() )
		return "pop callstack: process in system call";
	fp0 = fp();
	t->stmt->range.lo = callingpc(fp0);
	if( error = laybpt(t) ) return error;
	while( !(error = dostep(0,0,0))
	    && fpvalid(fpt = fp())
	    && (stackdir == GROWDOWN ? (fpt <= fp0) : (fpt >= fp0))
	    && !(error = liftbpt(t))
	    && !(error = dostep(0,0,1))
	    && !(error = laybpt(t)) )
		;
	if( error )
		liftbpt(t);
	else
		error = liftbpt(t);
	return error;
}

char *Core::stepoverjsr(){
	char *error;
	static Trap *t;
	long fp0;

	if(!t)
		t = new Trap(new Stmt(0,0,0),0);
	fp0 = fp();
	t->stmt->range.lo = instrafterjsr();
	if(error = laybpt(t)) return error;
	while( !(error = dostep(0,0,0))
	    && (stackdir == GROWDOWN ? (fp() < fp0) : (fp() > fp0))
	    && !(error = liftbpt(t))
	    && !(error = dostep(0,0,1))
	    && !(error = laybpt(t)) )
		;
	if( error )
		liftbpt(t);
	else
		error = liftbpt(t);
	return error;
}

const char *BehavsName(Behavs b){
	switch(b){
	case BREAKED:		return "BREAKPOINT:";
	case ERRORED:		return "ERROR STATE:";
	case HALTED:		return "STOPPED:";
	case ACTIVE:		return "RUNNING:";
	case PENDING:		return "EVENT PENDING:";
	case INST_STEPPED:	return "INSTR STEPPED:";
	case STMT_STEPPED:	return "STMT STEPPED:";
	}
	return Name( "Behavs=%d", b);
}

#include <unistd.h>

void Core::close(){
	trace( "%d.close()", this );	VOK;
	if( corefd>=0 ) ::close(corefd);
	if( stabfd>=0 ) ::close(stabfd);
	if( _symtab ) delete _symtab;
}

char *Core::blockmove(long s, long d, long ct){
	trace( "%d.blockmove(0x%X,0x%X,%d)", this, s, d, ct); OK("Core::blockmove");
	char *buf = new char[ct];
	char *error = read(s, buf, (int)ct);
	if( !error ) error = write(d, buf, (int)ct);
	delete buf;
	return error;
}

int Context::disc()	{ return U_CONTEXT; }

void Context::restore(){
	error = core->write(core->regaddr(), regsave, regbytes);
}

void Context::save(){
	if (core->behavetype() == ACTIVE)
		error = "context save: process not stopped";
	else if (core->atsyscall())
		error = "context save: process in system call";
	else
		error = core->read(core->regaddr(), regsave, regbytes);
}

Context::Context(Core *c){
	core = c;
	regbytes = core->cntxtbytes();
	regsave = new char[regbytes];
}

Context::~Context()		{ delete regsave; }
Context *Core::newContext()	{ return new Context(this); }
int Core::cntxtbytes()		{ return nregs() << 2;}

void Core::slavedriver(Core *sd){
	trace("%d.slavedriver()"); VOK;
	sd->_symtab->inherit = _symtab;
}
