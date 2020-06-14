#include <pads.h>
#include <stdio.h>

class Story : public PadRcv {
	char	file[128];
	FILE	*fp;
	long	lines;
	Pad	*pad;
	void	linereq(long,Attrib);
public:
	void	open();
		Story(char *f)	{ strlcpy( file, f, sizeof( file ) ); }
};

class News  : public PadRcv {
	Pad	*pad;
public:
		News();
};

int main(){
	if( chdir( "/var/tmp/news" ) ) exit(1);
	const char *error = PadsInit();
	if( error ){
		fprintf( stderr, "%s", error );
		exit(1);
	}
	new News();
	PadsServe();
}

News::News(){
	Story *s;
	FILE *fp;
	Menu m( "open", (Action) &Story::open );
	char file[128];
	long uniq = 0;
	pad = new Pad( (PadRcv *) this );
	pad->options(TRUNCATE|SORTED); 
	pad->banner( "News:" );
	pad->name( "News" );
	pad->makecurrent();	
	if( !(fp = Popen("ls", "r")) ){
		pad->insert( 1, "can't ls" );
		return;
	}
	while( fgets(file, 128, fp) ){
		int n = strlen(file);
		file[n-1] = '\0';
		s = new Story( file );
		pad->insert( ++uniq, (Attrib) 0, (PadRcv*) s, m, "%s", file );
	}
	Pclose(fp);
}

void Story::open(){
	char buf[256];

	if( !pad ){
		pad = new Pad( (PadRcv*) this );
		pad->banner( "%s:", file );
		pad->name( file );
		if( !(fp = fopen( file, "r" )) ){
			pad->insert( 1, "cannot open file" );
			return;
		}
		lines = 0;
		while( fgets( buf, 256, fp ) ) ++lines;
		pad->lines(lines);
	}
	pad->makecurrent();
}

void Story::linereq( long i, Attrib a ){
	char buf[256];
	long n;

	fseek( fp, 0, 0 );
	for( n = i; n > 0; --n )
		fgets( buf, 256, fp );
	pad->insert( i, a, "%s ", buf );
}
