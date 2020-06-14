#include <pads.h>
#include <time.h>

class Creator : public PadRcv {
	void	linereq(long,Attrib=0);
public:
	Pad	*pad;
		Creator();
	char	*kbd(char*);
};

char *Creator::kbd(char *s){
	int lo, hi;
	if( 2 == sscanf(s, "%d %d", &lo, &hi) )
		pad->createline(lo, hi );
	return 0;
}

Creator::Creator(){
	pad = new Pad( (PadRcv*) this );
	pad->options(ACCEPT_KBD);
	pad->banner( "%s=%s", "Banner", "Creator" );
	pad->name( "%s=%s", "name", "creator" );
	pad->makecurrent();
	for( long i = 1; i <= 50; i += 10 ){
		pad->createline(i);
		pad->createline(i+3, i+7);
	}
}

void Creator::linereq(long i, Attrib a){
	pad->insert(i, a, "line\t%d\t[]", i);
}

void PadsRemInit();

int main(int argc, char **argv){
	new long;
	if (argc == 2 && !strcmp(argv[1],"-R"))
		PadsRemInit();
	else
		PadsInit( argc, argv );
	extern const char *TapTo;
	TapTo = ".tapto";
	new Creator;
	PadsServe();
}
