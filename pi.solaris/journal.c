#include "journal.h"
#include "core.h"
#include "format.h"
#include <stdarg.h>
SRCFILE("journal.c")

static char sccsid[ ] = "%W%\t%G%";

int Journal::disc()	{ return U_JOURNAL; }

Journal::Journal(const char *p){
	proc = p;
	key = 0;
	length = 200;
	lastreq = 0;
}

void Journal::open(){
	trace("%d.open()", this);		VOK;
	if( !pad ){
		pad = new Pad((PadRcv*) this);
		banner();
	}
	pad->makecurrent();
}

void Journal::hostclose(){
	trace( "%d.hostclose()", this );	VOK;
	if( pad ){
		delete pad;
		pad = 0;
	}
	if( bls ){
		delete bls;
		delete ct;
		bls = 0;
		ct = 0;
	}
	invalidate();
}

void Journal::banner(){
	trace("%d.banner()", this);	VOK;
	if( pad ){
		pad->banner("Journal: %s", proc);
		pad->name("Journal %s", proc);
		pad->tabs(2);
		pad->options(TRUNCATE|NO_TILDE);
		if( !bls ){
			bls = new Bls[length];
			ct = new int[length];
		}
	}
}

const char *Journal::enchiridion(long l){
	 if (l == HELP_OVERVIEW)
		return "Journal Window";
	return 0;
}

void Journal::insert(const char* fmt, ...){
	trace("%d.insert(%s)", this, fmt);
	if( !this ) return;	VOK;
	if( !this->pad || !bls ) return;			// can that be?
	char buf[1024];
	va_list va;
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);
	Bls t(buf);
	if( !t.text[0] ) return;
	long i = key%length;
	if( key==0 || strcmp(bls[i].text, t.text) ){
		i = (++key)%length;
		ct[i] = 0;
		bls[i].clear();
		bls[i].af("%s", t.text);
	}
	if( lastreq == key )
		linereq(key);
	else
		pad->createline(key);
	++ct[i];
	if( key-length > 0 ) pad->removeline(key-length);
}

void Journal::linereq(long i, Attrib a){
	trace("%d.linereq(%d,%x)", this, i, a); VOK;
	if( i <= key-length || !bls ) return;			// too late!
	int mod = int(i%length);
	if( ct[mod] > 1 )
		pad->insert(i, a, "%s (%d)", bls[mod].text, ct[mod]);
	else
		pad->insert(i, a, "%s", bls[mod].text);
	lastreq = i;
}
