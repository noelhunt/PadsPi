#include "core.h"
#include "master.h"
#include "process.h"
#include "srctext.h"
#include "sigmask.h"
SRCFILE("sigmask.c")

static char sccsid[ ] = "%W%\t%G%";

int SigMask::disc()	{ return U_SIGMASK; }

void SigMask::banner(){
	trace( "%d.banner()", this );	VOK;
	if( pad ){
		pad->banner( "Signals: %s", core->procpath() );
		pad->name( "Signals" );
	}
}

const char *SigMask::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Signals Window";
		case HELP_MENU:		return "Signals Menu Bar";
		case HELP_LMENU:	return "Signals Line Menus";
		default:		return 0;
	}
}

SigMask::SigMask(Core *c){
	trace( "%d.SigMask(%d)", this, c );	VOK;
	core = c;
	mask = core->signalmaskinit();
	numlines = core->nsig();
	if (core->exechangsupported()) {
		numlines++;
		exechang = 0;
	}
	updatecore();
}

void SigMask::open(){
	Menu m;
	int i;
	trace( "%d.open()", this );	VOK;
	if( !pad ){
		pad = new Pad( (PadRcv*) this );
		pad->lines(numlines);
		banner();
		m.last("clear pending signal",(Action)&SigMask::clrcurrsig, 0);
		m.last("clear pending and go",(Action)&SigMask::clrcurrsig, 1);
		pad->menu(m.index("signals"));
		pad->options(TRUNCATE);
		for( i = 1; i <= numlines; ++i ) linereq( i, 0 );
	}
	pad->makecurrent();
}

void SigMask::updatecore(const char *error){
	if( !error && core->exechangsupported())
		error = core->exechang(exechang);
	if( !error ) error = core->signalmask(mask);
	core->process()->error(error);
}

void SigMask::execline(long e){
	static char *i[] = { "clear", "hang" };
	static char *l[] = { "", ">>> " };
	Menu m;
	trace( "%d.execline(%d)", this, e );		VOK;
	exechang = e &= 1;
	updatecore();
	long comp = e^1;
	m.last( i[comp], (Action) &SigMask::execline, comp );
	Attrib a = 0;
	pad->insert( numlines, a, (PadRcv*)this, m, "%sexec()", l[e] );
}

void SigBit::set(SigMask *s)		{ s->setsig(bit); }
void SigBit::clr(SigMask *s)		{ s->clrsig(bit); }
void SigBit::send(SigMask *s)		{ s->sendsig(bit); }

void SigMask::linereq(int sig, Attrib a ){
 	Menu m;
	static SigBit *sigbit;
	if( !sigbit )
		sigbit = new SigBit[33];
	trace( "%d.linereq(%d,0x%X)", this, sig, a );	VOK;
	if(sig == numlines && core->exechangsupported()){
		execline(exechang);
		return;
	}
	const char *on = mask&bit(sig) ? ">>> " : "";
 	m.last( "send signal", (Action)&SigBit::send, (long)this );
	if( on[0] == '>' )
		m.first("trace off", (Action)&SigBit::clr, (long)this);
	else
		m.first("trace on",  (Action)&SigBit::set, (long)this);
	sigbit[sig].bit = (int)sig;
	pad->insert(sig, a, (PadRcv*)(sigbit+sig), m, "%s%s", on,
			core->signalname((int)sig));
}

void SigMask::signalmask(long sig){
	trace( "%d.signalmask(%d)", this, sig );	VOK;
	linereq(sig, SELECTLINE);
	updatecore();
}

void SigMask::sendsig(long sig){
	trace( "%d.sendsig(%d)", this, sig );	VOK;
	updatecore(core->signalsend(sig));
}

void SigMask::setsig(long sig){
	trace( "%d.setsig(%d)", this, sig );	VOK;
	mask |= bit(sig);
	signalmask( sig );
}

void SigMask::clrsig(long sig){
	trace( "%d.clrsig(%d)", this, sig );	VOK;
	mask &= ~bit(sig);
	signalmask( sig );
}

void SigMask::hostclose(){
	trace( "%d.hostclose()", this );	VOK;
	if( pad ){
		delete pad;
		pad = 0;
	}
}

void SigMask::clrcurrsig(long andgo){
	trace( "%d.clrcurrsig()", this );	VOK;
	updatecore(core->signalclear());
	if( andgo )
		core->process()->go();
	else
		core->process()->habeascorpus(core->behavs(), 0);
}
