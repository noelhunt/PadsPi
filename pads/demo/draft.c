#include <pads.h>
#include <time.h>

#define FROMKEY	1
#define TOKEY	2
#define CCKEY	3
#define BCCKEY	4
#define SUBJKEY	5

const char *keyword[5] = {
	"from:",
	"to:",
	"cc:",
	"Bcc:",
	"subject:"
};

const char *comp[6] = {
	"from: Noel Hunt <noel.hunt@gmail.COM>",
	"to:",
	"cc:",
	"Bcc:",
	"subject:",
	"--------"
};

class Draft : public PadRcv {
	int	uniq;
public:
	Pad	*pad;
		Draft();
	char	*kbd(char*);
};

char *Draft::kbd(char *s){
	int key = 0;
	char *cp = strchr(s, ' ');
	*cp++ = 0;
	if( !strcmp(s, "from:") )	key = FROMKEY;
	else if( !strcmp(s, "to:") )	key = TOKEY;
	else if( !strcmp(s, "cc:") )	key = CCKEY;
	else if( !strcmp(s, "Bcc:") )	key = BCCKEY;
	else if( !strcmp(s, "subject:") ) key = SUBJKEY;
	if( key )
		pad->insert( key, "%s %s", keyword[key], cp );
	else
		cp[-1] = ' ', pad->insert( ++uniq, "%s", s);
	return 0;
}

Draft::Draft(){
	uniq = 0;
	pad = new Pad( (PadRcv*) this );
	pad->options(ACCEPT_KBD);
	pad->banner( "%s=%s", "Banner", "Draft" );
	pad->name( "%s=%s", "name", "Mh draft" );
	pad->makecurrent();
	for( long i = 0; i < 6; i++ )
		pad->insert(++uniq, comp[i]);
}

void PadsRemInit();

int main(int argc, char **argv){
	new long;
	if (argc == 2 && !strcmp(argv[1],"-R"))
		PadsRemInit();
	else
		PadsInit( argc, argv );
	new Draft;
	PadsServe();
}
