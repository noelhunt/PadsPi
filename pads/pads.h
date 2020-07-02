#ifndef PADS_H
#define PADS_H

#define SAFE					/* Safe cache allocation */

typedef unsigned short Attrib;

#define PADS_VERSION	0x930125		/* YYMMDD */
#define CARTE		0x80000000
#define NUMERIC		1

enum Protocol {
	P_UCHAR		= 1,
	P_SHORT		= 2,
	P_LONG		= 4,

	P_CACHEOP	= 0x10,
	P_I_DEFINE	= 0x11,
	P_I_CACHE	= 0x12,
	P_C_DEFINE	= 0x13,
	P_C_CACHE	= 0x14,

	P_STRING	= 0x20,
	P_INDEX		= 0x21,

	P_PADDEF	= 0x30,
	P_ATTRIBUTE	= 0x31,
	P_BANNER	= 0x32,
	P_CARTE		= 0x33,
	P_LINES		= 0x34,
	P_NAME		= 0x35,
	P_TABS		= 0x36,
	P_HELPCARTE	= 0x37,

	P_PADOP		= 0x40,
	P_ACTION	= 0x41,
	P_ALARM		= 0x42,
	P_CLEAR		= 0x43,
	P_CYCLE		= 0x44,
	P_DELETE	= 0x45,
	P_KBDSTR	= 0x46,
	P_LINE		= 0x47,
	P_LINEREQ	= 0x48,
	P_MAKECURRENT	= 0x49,
	P_MAKEGAP	= 0x4A,
	P_NEXTLINE	= 0x4B,
	P_NUMERIC	= 0x4C,
	P_USERCLOSE	= 0x4D,
	P_CREATELINE	= 0x4E,
	P_REMOVELINE	= 0x4F,

	P_HOSTSTATE	= 0x50,
	P_BUSY		= 0x51,
	P_IDLE		= 0x52,

	P_USERCUT	= 0x5F,

	P_PICK		= 0x60,

	P_HELPSTR	= 0x70,
	P_SHELL		= 0x71,

	P_VERSION	= 0x80,
	P_QUIT		= 0x81,
	P_ERROR		= 0x82
};

#ifndef PADS_TERM
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define PRINTF_TYPES const char* ...
#define PRINTF_ARGS const char* fmt ...

#ifdef TRACE
#define OK(x)		if( !ok() ) PadsError(__SRC__);
#define VOK		if( !ok() ) PadsError(__SRC__);
#define IF_LIVE(x)	{ if( x )   PadsError(__SRC__); } if(0)
#define SRCFILE(f)	static const char *__SRC__ = f;
typedef int  (*PFI)(PRINTF_ARGS);
extern "C" {
	extern PFI trace_ptr;
	PFI trace_fcn(const char*,int);
};
#define trace  (!(trace_ptr=trace_fcn(__SRC__,__LINE__))) ? 0 : (*trace_ptr)
#else
#define OK(x)		{ if( !ok() ) return x; }
#define VOK		{ if( !ok() ) return ; 	}
#define IF_LIVE(x)	  if( x )
#define SRCFILE(f)
#ifdef __cplusplus
#define trace(...)
#else
#define trace  if( 0 )
#endif
#endif

typedef unsigned char uchar;

char *StrDup(const char*);

#ifdef TAC
class Index {
public:
	ushort	major;
	ushort	minor;
		Index()			{ major = minor = 0;		}
	 	Index(int i)		{ major = i>>8; minor = i&0xFF; }
		Index(int j, int n )	{ major = j;	minor = n;	}
	short	sht();			/* don't inline sht() - sht() etc */
	int	null()			{ return !(major|minor);	}
};
#else
class Index {
public:
	int     indx;
		Index()			{ indx = 0;	}
		Index(int i)		{ indx  = i;	}
       int	null()			{ return !indx;	}
};
#endif
extern Index ZIndex;

class Carte {
public:
	int	size;		/* host.size != term.size */
	int	items;
	uchar	attrib;
	uchar	width;
	uchar	pad[2];
#ifndef SAFE
	Index	bin[1];
		Carte() {};
#else
	Index	*bin;
		Carte(int size){ bin = new Index[size+1]; };
#endif
};
#else
#define MJR(i)	((i)>>8)
#define MNR(i)	((i)&0xFF)
typedef unsigned int Index;
typedef struct Carte Carte;
typedef enum Protocol Protocol;
struct Carte {
	int	size;		/* host.size != term.size */
	uchar	attrib;
	uchar	width;
	uchar	pad[2];
#ifndef SAFE
	Index	bin[1];
#else
	Index	bin[];		/* C99: FLEXIBLE ARRAY MEMBER OF CLASS */
#endif
};
#endif

#define	SELECTLINE	((Attrib)0x0001)
#define SORTED		((Attrib)0x0002)
#define ACCEPT_KBD	((Attrib)0x0004)
#define FOLD		((Attrib)0x0008)
#define TRUNCATE	((Attrib)0x0010)
#define USERCLOSE	((Attrib)0x0020)
#define DONT_CUT	((Attrib)0x0040)
#define FLUSHLINE	((Attrib)0x0080)	/* should not be required */
#define FAKELINE	((Attrib)0x0100)
#define USERCUT		((Attrib)0x0200)
#define DONT_CLOSE	((Attrib)0x0400)
#define NO_TILDE	((Attrib)0x0800)
#define DONT_DIRTY	((Attrib)0x1000)

#ifndef PADS_TERM
#include <stdarg.h>
char *sf(const char*,...);
char *vf(const char*,va_list);
const char *PadsInit(int = 0, char** = 0);
void PadsServe(long = 0);
void NewHelp();
void NewPadStats();
class PadRcv;
typedef void (PadRcv::*Action)(...);
void Pick(char*,Action,long);
long UniqueKey();
Index NumericRange(short,short);
void PadsWarn(PRINTF_TYPES);
void PadsQuit();
extern char *TapTo;

void PadsError(PRINTF_TYPES);

#define	HELP_OVERVIEW	0
#define	HELP_MENU	1
#define	HELP_KEY	2
#define	HELP_LMENU	3
#define	HELP_LKEY	4
#define	HELP_NTOPICS	5

class Remote;
class Pad;
class PadRcv {	friend Remote; friend Pad;
	short	oid;
	short	magic;
	int	isvalid();
public:
		PadRcv();
		~PadRcv();
	void	invalidate();
virtual	int	disc();
virtual char	*kbd(char*);
virtual char	*help();
virtual	void	numeric(long);
virtual	void	userclose();
virtual	void	cycle();
virtual	void	linereq(long,Attrib=0);
virtual	int	accept(Action);
virtual	void	usercut();
#ifdef OPENLOOK
	void	showhelp(long);
#endif
};

class Pad {
	PadRcv	*_object;
const	char	*_name;
const	char	*_banner;
	Attrib	_attributes;
	char	_unused[2];
	long	_lines;
	void	termop(enum Protocol);
	long	errorkey;
	void	nameorbanner(enum Protocol, const char*, void*);
#ifdef HELPMENU
	void	helpmenu();
#endif
	void	vinsert(long, Attrib, PadRcv*, Index, const char*, va_list);
public:
	int	ok();
		Pad(PadRcv *);
		~Pad();
	void	alarm(short=0);
	void	banner(PRINTF_TYPES);
	void	clear();
	void	dump();
	void	error(PRINTF_TYPES);
	void	insert(class Line&);
	void	insert(long, Attrib, PadRcv*, Index, const char*, ...);
	void	insert(long, Attrib, PadRcv*, class Menu&, const char*, ...);
	void	insert(long, Attrib, const char*, ...);
	void	insert(long, const char*, ...);
	void	lines(long);
	void	makecurrent();
	void	makegap(long,long);
	void	menu(Index);
	void	menu(class Menu&);
	void	name(PRINTF_TYPES);
	void	options(Attrib, Attrib=0);
	void	tabs(short);
	void	removeline(long);
	void	createline(long,long);
	void	createline(long);
};

class Line {
public:
	int	ok();
		Line();
	PadRcv	*object;
	char	*text;
	long	key;
	Attrib	attributes;
	char	dummy[2];	/* For buggy SysV i386 compiler */
	Index	carte;
};

class IList {
	friend	Menu;
	Index	index;
	IList	*next;
public:
		IList(Index i, IList *n) { index = i; next = n; }
};

class Item {
public:
const	char	*text;
	Action	action;
	long	opand;
		Item(char*,Action,long);
		Item();			/* ever used ? */
};

class Menu {
	IList	*list;
	int	size;
	void	dump();
public:
		Menu();
		~Menu();
		Menu( char*, Action=0, long=0 );
	Index	index( char* =0, Action=0, long=0 );
	void	first( char*, Action=0, long=0 );
	void	first( Index );
	void	last( char*, Action=0, long=0 );	
	void	last( Index );	
	void	sort( char*, Action=0, long=0 );
	void	sort( Index );
};

class Binary {
public:
	Binary	*left;
	Binary	*right;
	Index	index;
		Binary()	{}
};

class ItemCache;
class CarteCache;
class Cache {
	friend	ItemCache;
	friend	CarteCache;
	Binary	*root;
	Index	current;
	Index	SIZE;
public:
#ifdef TAC
		Cache(uchar,uchar);
#else
		Cache(int);
#endif
	int	ok();
};

class ItemCache : public Cache {
#ifdef TAC
	Item	***cache;
#else
	Item	**cache;
#endif
	int	compare(Item*,Item*);
public:
		ItemCache();
	Index	place(Item);
	Item	*take(Index);
};

class CarteCache : public Cache {
#ifdef TAC
	Carte	***cache;
#else
	Carte	**cache;
#endif
	int	compare(Carte*,Carte*);
public:
		CarteCache();
	Index	place(Carte*);
	Carte	*take(Index);
	Index	numeric(int,int);
	void	cartelimits(Carte*);
};

extern ItemCache  *ICache;
extern CarteCache *CCache;
#endif

#ifndef SAFE
/* Carte already has one Index allocated (bin[1]) */
#define CARTESIZE(s) (sizeof(Carte) + (s)*sizeof(Index))
#else
/* Only used in term */
#define CARTESIZE(s) (sizeof(Carte) + (s+1)*sizeof(Index))
#endif

#ifdef PADS_TERM
long	RcvLong();
ushort	RcvShort();
uchar	RcvUChar();
char	*RcvString();
void	RcvAllocString(char**);

void	SendLong(long);
void	SendShort(short);
void	SendUChar(uchar);
void	SendString(char*);
#else
#include <stdio.h>
FILE *Popen(const char*,const char*);
int Pclose(FILE*);

class Remote {
public:
	int	ifd;
	int	ofd;
	int	pktbase;
	int	pktsize;
	uchar	writebuffer[2048];

	long	shiftin(int);
	void	shiftout(int, long);
	int	writesize;
	void	err(const char * = 0);
	int	get();
	void	checkproto(int);
	void	put(char);
	void	proto(int);

	long	rcvlong();
	short	rcvshort();
	uchar	rcvuchar();
	PadRcv	*rcvobj();

	void	sendlong(long);
	void	sendobj(PadRcv*);
	void	sendshort(short);
	void	senduchar(unsigned char);
	char	*rcvstring(char*);
	void	sendstring(const char*);

	void	pktstart(char);
	void	pktend();
	void	pktflush();

	void	share();

		Remote(char*);
		Remote(int,int);
};

extern Remote *R;

#endif

#if defined(sgi) && defined(_BSD_SIGNALS)
/* C++ on IRIX 5.1 */
#include <errno.h>
#define SIG_TYP SIG_PF
#endif
#endif

