#include <pads.h>
#include <stdio.h>
#include <time.h>

class Date : public PadRcv {
public:
	class Pad	*pad;
	void		date();
			Date();
};

Date::Date()
{
	Menu m;
	pad = new Pad( (PadRcv*) this );
	pad->banner( "Current Date" );
	pad->name( "date" );
	m.first( "date", (Action) &Date::date );
	pad->menu( m );
	pad->makecurrent();
}

void Date::date()
{
	long t;
	time(&t);
	pad->insert( 1, "%s", ctime(&t) );
}

int main()
{
	const char *error = PadsInit();
	if( error ){
		fprintf( stderr, "%s", error );
		exit(1);
	}
	new Date;
	PadsServe();
}
