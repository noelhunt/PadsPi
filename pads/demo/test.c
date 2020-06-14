#include <pads.h>
#include <time.h>

class Test : public PadRcv {
	void	linereq(long,Attrib=0);
public:
	Pad	*pad;
		Test();
	void	err(long);
	void	lines(long);
	void	exit();
#ifdef CYCLE
	void	cycle();
#endif
	void	tabs(long t)	{ pad->tabs((int)t); }
	void	remove(long k)  { pad->removeline(k); }
	char	*kbd(char*);
	char	*help();
	void	usercut();
};

void Test::usercut() {}

char *Test::kbd(char *s){
	int i;
	if( !sscanf(s,"%d",&i) || i<=0 || i>=1000 )
		return (char*) "out of range";
	linereq(i,SELECTLINE);
	return 0;
}

char *Test::help(){
	return (char*) "Test Line Keyboard";
}

void Test::exit() { ::exit(0); }

#ifdef CYCLE
void Test::cycle(){
	static int i;
	long t;

	pad->alarm(i++/10);
	time(&t);
	pad->insert( 1, "%s", ctime(&t) );
}
#endif

Test::Test(){
	Menu m( "first 1", (Action) &Test::linereq, 1 );
	Menu sub;
	Menu subsub;
	pad = new Pad( (PadRcv*) this );
	pad->options(ACCEPT_KBD);
	pad->lines(1000);
	pad->insert(1001, "one thouseand and one");
	pad->banner( "%s=%s", "Banner", "Test" );
	pad->name( "%s=%s", "name", "test" );
	pad->options(DONT_CLOSE|NO_TILDE);
	pad->makecurrent();
	subsub.sort( "B", (Action) &Test::linereq, 101 );
	subsub.sort( "A", (Action) &Test::linereq, 102 );
	subsub.sort( "D", (Action) &Test::linereq, 103 );
	subsub.sort( "C", (Action) &Test::linereq, 104 );
	sub.first( subsub.index("subsub") );
	sub.last( "b", (Action) &Test::linereq, 110 );
	sub.last( "a", (Action) &Test::linereq, 120 );
	sub.last( "d", (Action) &Test::linereq, 130 );
	sub.last( "c", (Action) &Test::linereq, 140 );
	m.first( sub.index("sub") );
#ifdef CYCLE
	m.first( "cycle", (Action) &Test::cycle );
#endif
	m.first( "tabs=1", (Action) &Test::tabs, 1 );
	m.first( "tabs=3", (Action) &Test::tabs, 3 );
	m.last( "tabs=5", (Action) &Test::tabs, 5 );
	m.last( "tabs=7", (Action) &Test::tabs, 7 );
	m.last( "tabs=0", (Action) &Test::tabs, 0 );
	m.last( "tabs=128", (Action) &Test::tabs, 128 );
	m.last( "remove 100", (Action) &Test::remove, 100 );
	m.last( NumericRange(1,10) );
	pad->menu(m.index());
	pad->alarm();
}

void Test::linereq(long i, Attrib a){
	Menu m;

	switch( i%5 ){
	case 0:
		pad->insert( i, a, "line\t%d\t[]", i  );
		pad->banner( "banner=%d", i );
		break;
	case 1:
		m.last( "150", (Action)&Test::linereq, 150 );
		pad->insert( i, a|USERCUT, (PadRcv*) this, m, "line\t%d", i );
		pad->name( "name=%d", i );
		break;
	case 2:
		m.last("250", (Action)&Test::linereq, 250 );
		pad->insert( i, a|USERCUT, (PadRcv*) this, m, "line\t%d", i );
		break;
	case 3:
		m.last("exit?", (Action)&Test::exit, 0 );
		pad->insert( i, a|USERCUT, (PadRcv*) this, m, "line\t%d", i );
		break;
	case 4:
		pad->insert( i, a|USERCUT, "" );
		break;
	}
	
}

void PadsRemInit();

int main(int argc, char **argv){
	new long;
	if (argc == 2 && !strcmp(argv[1],"-R"))
		PadsRemInit();
	else
		PadsInit( argc, argv );
	new Test;
	PadsServe();
}
