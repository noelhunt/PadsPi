/* /n/seki/usr/tac/pads/doc/middle.c */

#include <stdio.h>
#include <pads.h>

#include "story.h"

class News  : public PadRcv {
	Pad	*pad;
public:
		News();
};

int main() {
	if( chdir( "/export/home/noel/news" ) ) exit(1);
	const char *error = PadsInit();
	if( error ){
		fprintf( stderr, "%s", error );
		exit(1);
	}
	new News();
	PadsServe();
}

News::News()
{
	Story *s;
	FILE *fp;
	Menu m( "open", (Action) &Story::open );
	char file[128];
	long uniq = 0;

	pad = new Pad( (PadRcv *) this );
	pad->options(TRUNCATE|SORTED); 
//	pad->banner( "News:" );
	pad->banner( "Miscellaneous Usenet Articles, CompSci, Japanese, Solaris, Sendmail etc." );
	pad->name( "News" );
	pad->makecurrent();	
	if( !(fp = popen("ls", "r")) ){
		pad->insert( 1, "can't ls" );
		return;
	}
	while( fgets(file, 128, fp) ){
		s = new Story( file );
		pad->insert( ++uniq, (Attrib) 0, (PadRcv*) s, m, "%s", file );
	}
	pclose(fp);
}

#include "storyopen.c"

void Story::linereq( long i, Attrib a )
{
	char buf[256];
	long n;

	fseek( fp, 0, 0 );
	for( n = i; n > 0; --n )
		fgets( buf, 256, fp );
	pad->insert( i, a, "%s ", buf );
}
