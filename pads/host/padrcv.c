#include <pads.h>
SRCFILE("padrcv.c")

void PadRcv::invalidate(){
	static short uniq;

	magic = 0;
	oid = ++uniq;
}
	
#define MAGIC (('P'<<8)|'R')
PadRcv::PadRcv(){
	invalidate();
	magic = MAGIC;
}

int PadRcv::isvalid()		{ return magic == MAGIC; }
PadRcv::~PadRcv()		{ invalidate(); }
char *PadRcv::help()		{ return 0; }

void MissingVirtual(const char *s, const char *f){
	char t[128];
	trace("MissingVirtual(%s,%s)", s, f);
	sprintf(t, "%s object does not define %s()", s, f);
	PadsWarn(t);
}

char *PadRcv::kbd(char*)	{ return (char*)"no kbd() function"; }
void PadRcv::numeric(long)	{ MissingVirtual("NumericRange()", "numeric"); }
void PadRcv::userclose()	{ MissingVirtual("USERCLOSE", "userclose"); }
void PadRcv::usercut()		{ MissingVirtual("USERCUT", "usercut"); }
void PadRcv::cycle()		{ MissingVirtual("alarm()", "cycle"); }
void PadRcv::linereq(long, Attrib) { MissingVirtual("lines()", "linereq"); }

int PadRcv::accept(Action /*a*/){
//	trace("%d.accept(%d)", this, a);
	return 0;
}

int PadRcv::disc()		{ trace("%d.disc()", this); return 0;	}

#ifdef OPENLOOK
void PadRcv::showhelp(long l){
	extern int helptopic(const char*);
	const char *s = help();
	if (!s)
		PadsWarn("Sorry. No help provided");
	else if (!helptopic(s))
		PadsWarn( "Sorry, help topic not found: %s", s);
}
#endif
