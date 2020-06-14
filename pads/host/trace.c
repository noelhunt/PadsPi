#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef __linux__
#include <time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#define INTERVAL	60
#define STRSIZE		64
#define TABLESIZE	128
#define IDLECYCLE	1000

typedef int (*PFI)(const char*,...);

static const char *ControlFile = "TRACE";

struct TraceRange {
	char	srcfile[STRSIZE];
	int	from;
	int	to;
};
static int TableSize;
static struct TraceRange RangeTable[TABLESIZE];
static struct TraceRange Context[16];
static int ContextIndex;
static int Calls;

static FILE *Tty = 0;
static char Device[STRSIZE];

static void ifprintf( const char *fmt, ... ){
	va_list ap;
	if( Tty ){
		va_start(ap, fmt);
		vfprintf( Tty, fmt, ap );
		fflush( Tty );
	}
}

#define SEMANTICS(text) { ifprintf( "Error: %s\n", text ); return 1; }

static int RangeCmd( char *cmd ){
	struct TraceRange r, x;

	switch( sscanf( cmd, "range %s %d %d", r.srcfile, &r.from, &r.to ) ){
		case 2:	r.to = r.from;
			break;
		case 3: break;
		default:
			return 0;
	}
	if( r.to < r.from ) { x = r; x.from = r.to; x.to = r.from; r = x; }
	if( TableSize >= TABLESIZE ) SEMANTICS( "table overflow" );
	RangeTable[TableSize++] = r;
	return 1;
}

static int DeviceCmd( char *cmd ){
	if( sscanf( cmd, "device %s", Device ) != 1 ) return 0;
	return 1;
}

static void Commands(FILE *f){
	char cmd[STRSIZE];

	TableSize = 0;
	(void)strcpy( Device, "/dev/null" );
	while( fgets( cmd, STRSIZE, f ) ){
		ifprintf( "%s", cmd );
		if(	!RangeCmd(cmd)	&&
			!DeviceCmd(cmd)		) ifprintf( "Syntax Error\n" );
	}
}

static void Update(){
	static time_t modified;
	FILE *f;
	struct stat buf;
	char shcmd[STRSIZE];

	ifprintf( "trace 831120\n" );
	if( !(f = fopen( ControlFile, "r" )))
		return;
	fstat( fileno(f), &buf );
	if( buf.st_mtime == modified ){
		fclose(f);
		return;
	}
	modified = buf.st_mtime;
	Commands(f);
	fclose(f);
	if( !strcmp( Device, "/dev/null") ){
		if( Tty ) fclose( Tty );
		Tty = 0;
		return;
	}
	if( !strcmp( Device, "stderr") ){ 
		if( Tty && Tty != stderr )
			fclose( Tty );
		Tty = stderr;
	}else if( f = fopen( Device, "w" ) ){
		if( Tty && Tty != stderr )
			fclose( Tty );
		Tty = f;
	}else
		ifprintf( "Cannot open: %s\n", Device );
}
		
static void CheckTime(time_t t){
	static time_t timestamp = 0;

	if( t > timestamp+INTERVAL ){
		Update();
		timestamp = t;
	}
}

static int ContextSelected(){
	int i, line, from, to;

	for( i = 0; i < TableSize; ++i )
		if( !strcmp(Context[ContextIndex].srcfile, RangeTable[i].srcfile) ){
			line =  Context[ContextIndex].from;
			from = RangeTable[i].from;
			to = RangeTable[i].to;
			if( line>=from && line<=to ) return 1;
			if( -line>=from && -line<=to ) return 0;
		}
	return 0;
}

int TraceArgs( const char *fmt, ... ){
	va_list ap;
	char mmss[32];
	time_t t = time(0L);

	--ContextIndex;
	CheckTime(t);
	if( !Tty || !ContextSelected() ) return 0;
	strcpy( mmss, &ctime(&t)[14] );	/* "Sun Sep 16 01:03:52 1973\n\0" */
	mmss[5] = '\0';
	fprintf(Tty, "%d %s %s:%d ",
		Calls, mmss, Context[ContextIndex].srcfile, Context[ContextIndex].from);
	va_start(ap, fmt);
	vfprintf( Tty, fmt, ap );
	va_end(ap);
	fputc( '\n', Tty );
	fflush( Tty );
	return 1;
}

static long *watchloc = 0, watchval;

PFI trace_ptr;
PFI trace_fcn( const char *srcfile, int line ){
	static int idle = 0;

	++Calls;
	if( !Tty && idle ){
		idle = (idle+1)%IDLECYCLE;
		return 0;
	}
	if( watchloc && watchval!=*watchloc ){
		ifprintf("!!!!!!! %s:%d *%d: %d -> %d\n",
			srcfile, line, watchloc, watchval, *watchloc );
		watchval = *watchloc;
	}
	strncpy( Context[ContextIndex].srcfile, srcfile, STRSIZE );
	Context[ContextIndex].to = Context[ContextIndex].from = line;
	++ContextIndex;
	return TraceArgs;
}

void watch(long *loc){
	if( watchloc = loc ){
		watchval = *watchloc;
		ifprintf("watching *%d = %d\n", watchloc, watchval );
	}
}

#ifdef TRACEN
static FILE *Tfp = stderr;

void trace( const char *fmt, ... ){
	va_list ap;
	char buf[256];
#undef TRACEBODY
#ifdef TRACEBODY
	strcpy(buf, fmt);
	if( !Tfp )
		Tfp = fopen("/tmp/pads.host", "w");
	va_start(ap, fmt);
	vfprintf( Tfp, buf, ap );
	va_end(ap);
	fputc( '\n', Tfp );
	fflush( Tfp );
#endif
}
#endif
