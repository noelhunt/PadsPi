#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <cursor.h>
#include <keyboard.h>
#define PADS_TERM
#include <pads.h>

#define ASSERT
#ifdef assert
#undef assert
#endif
long assertf();
#define assert(e) ( assertf( (long) (e) ) )
#define salloc(s) ((struct s*) Alloc(sizeof(struct s)) )
#define ISPAD(p)	((p) != &Sentinel)
#define ISLINE(l,p)	((l) != &(p)->sentinel)	/* used how much? */

#if defined(SAMTERM) && !defined(SCROLL)
#define SCROLL
#endif

enum Resource {
        RHost,
	RKeyboard,
	RMouse,
	RAlarm,
	RResize,
	NRes
};

enum {
	BACK,
	HIGH,
	BORD,
	TEXT,
	HTEXT,
	NCOL,
	NORMAL,
	BANNER,
	SELECT,
	UNSELECT,
	Outlinewidth=2
};

#define	READBUFSIZE	8192

typedef struct	Readbuf	Readbuf;

struct Readbuf{
        short   n;			/* # bytes in buf */
        uchar   data[READBUFSIZE];	/* data bytes */
};

typedef struct String{
	char *s;	/* pointer to string */
	short n;	/* number used, no terminal null */
	short size;	/* size of allocated area */
} String;

extern Readbuf hostbuf[];

typedef struct Timer	Timer;

struct Timer {
        int	dt;
        int	cancel;
        Channel *c;     /* chan(int) */
        Timer   *next;
};

extern Timer *Alarm;

typedef unsigned char uchar;
typedef int  (*PFI)();
typedef long (*PFL)();
typedef char (*PFC)();

typedef struct Line {
	short		oid;
	long		object;
	Index		carte;
	uchar		ptop;
	uchar		phit;
	Attrib		attributes;
/* ***************************** */
 struct	Line		*down;
 struct	Line		*up;
	char		*text;
	long		key;
	Rectangle	rect;
} Line;

typedef struct Pad {
	short		oid;
	long		object;
	Index		carte;
	uchar		ptop;
	uchar		phit;
	Attrib		attributes;
/* ***************************** */
 struct	Pad		*front;
 struct	Pad		*back;
	Rectangle	rect;
#ifdef TAGS
	Rectangle	brect;
#endif
	Rectangle	srect;
#ifdef SCROLL
	Rectangle	bar;
#endif
	char		*name;
	Line		sentinel;
	long		selkey;
	short		lo;
	short		hi;
	short		ticks;
	short		tabs;
#ifdef SCROLL
	int		lines;
#endif
} Pad ;

typedef struct Selection {
	Pad	*pad;
	Line	*line;
} Selection;

typedef struct Entry {
	char	*text;
	void	(*action)();
	long	opand;
struct	Script	*script;
} Entry;

typedef struct Script{
	Entry	*(*generator)();
	void	(*limits)();
struct	Script	*more;
	Index	cindex;		/* bogus! bogus! bogus! */
	short	items;
	short	width;
	short	prevtop;
	short	prevhit;
} Script;

Entry *ScriptHit();

typedef enum Cover { CLEAR, PARTIAL, COMPLETE } Cover;

extern Selection Selected;	/* selected line 			*/
extern Pad *Current;		/* current pad				*/
extern Pad *HostObject;		/* global arg to HostAction(Index)	*/
extern Pad *HostParent;		/*  and its pad's object		*/
extern short Scrolly;		/* suggest middle for Paint()		*/

extern Point ZPoint;
extern Rectangle ZRectangle;
extern Pad Sentinel;
extern Pad *DirtyPad;
extern Rectangle KBDrect;

extern int Pstate;
extern int hostfd[2];
extern Channel *hostc;
extern Mouse* mouse;
extern Mousectl* mousectl;
extern Image *Arrow;
extern Image *tagcols[];
extern Image *padcols[];
extern Image *kbdcols[];

#define BIGMEMORY 1
#define NOVICEUSER 2
int Configuration;

enum {
	GAP=2,
	PADBORDER=3,
	SCROLLWID=14
};

#define CHARWIDTH       (stringwidth(font, "m"))

char *Alloc();

extern Cursor Danger;
extern Cursor Bullseye;
extern Cursor NoMemory;
extern Cursor NoGCMemory;
extern Cursor Coffee;
extern Cursor HostBusy;
extern Cursor Sweep;
extern Cursor Drag;
extern Cursor *Jcursor;

extern Index CIndex;

enum {
	Button1 = 1,
	Button2 = 2,
	Button3 = 4
};

#define BUTT1 1
#define BUTT2 2
#define BUTT3 4

#define	UP	0
#define	DOWN	1

#define button(i)       (mouse->buttons)&(1<<((i)-1))
#define button1()       (mouse->buttons&01)
#define button2()       (mouse->buttons&02)
#define button3()       (mouse->buttons&04)
#define button12()      (mouse->buttons&03)
#define button13()      (mouse->buttons&05)
#define button23()      (mouse->buttons&06)
#define button123()     (mouse->buttons&07)
#define butts		(mouse->buttons&07)

extern int	quitok;

typedef struct RectList RectList ;
struct RectList {
	Rectangle	*rp;
	RectList	*more;
};

#ifdef PLAN9
ulong umuldiv(ulong, ulong, ulong);
long muldiv(long, long, long);
#else
#define	muldiv(a,b,c)		( (long)((a)*( (long)b)/(long)(c) ) )
#endif
#define max( low, high )	( (high > low)? ( high ): ( low ) )
#define min( low, high )	( (low < high)? ( low ): ( high ) )

#define	SECOND		* 1000
#define	SECONDS		* 1000
#define INTERVAL	10000

void trace(char*,...);

/* buttons.c */
Entry *CarteEntry(Index, short);
void MOUSEServe(void);
void FlashBorder(Pad*);
/* cache.c */
void CacheOp(Protocol);
char *IndexToStr(Index);
Carte *IndexToCarte(Index);
/* host.c */
void FlushRemote(void);
int GetRemote(void);
void PutRemote(char);
void ToHost(Protocol, long);
void HostAction(Index*);
void HostNumeric(long);
void RCVServe(void);
void HelpString(void);
void ProtoErr(char*);
/* io.c */
void initio(void);
void waitMOUSE(void);
void mouseunblock(void);
void kbdblock(void);
int Jwait(void);
int ecankbd(void);
int ekbd(void);
int kbdchar(void);
int qpeekc(void);
int rcvchar(void);
void ALARMServe(void);
/* lib.c */
void waitMOUSE(void);
char *itoa(int);
int dictorder(char*, char*);
long assertf(long);
Point dxordy(Point);
Rectangle boundrect(Rectangle, Rectangle);
Rectangle scrollbar(Rectangle, int, int, int, int);
int rectinrect(Rectangle, Rectangle);
char *Alloc(int);
Rectangle moverect(Rectangle, Rectangle);
void cursset(Point);
void cursswitch(Cursor*);
void Quit(void);
void panic(char*);
void dprint(int, char*, ...);
/* lineops.c */
void DoCut(Line*);
void CutLine(void);
void Sever(void);
/* master.c */
void ALARMServe(void);
/* pad.c */
int PadSized(Rectangle);
void DelLine(Line*);
Line *InsAbove(Line*, Line*);
Line *InsPos(Pad*, Line*);
void KBDServe(void);
void SetCurrent(Pad*);
void LineXOR(Line*, int);
void Dirty(Pad*);
void PadOp(Protocol op);
void PickOp(void);
Pad *PickPad(Point pt);
void DeletePick(void);
void DelLines(Pad*);
void DeletePad(Pad*);
void Select(Line*, Pad*);
void MakeCurrent(Pad*);
void Move(void);
void Reshape(void);
void PadClip(void);
void PadStart(Rectangle);
void InvertKBDrect(char*, char*);
void PaintKBD(void);
int LayerReshaped(void);
void FoldToggle(Attrib*);
Entry *FoldEntry(Attrib*);
/* paint.c */
void DoubleOutline(Image*, Rectangle);
void HeavyBorder(Pad*);
int ClipPaint(Rectangle, Pad*);
void PadBlt(Image*, Rectangle, Pad*);
void Paint(Pad*);
void LineReq(Pad*, long, long, int);
void CRequestLines(Pad*);
void RequestLines(Pad*);
void Pointing(void);
/* scripthit.c */
Entry *ScriptHit(Script*, int, RectList*);
/* plan9.c */
void padsnarf(String*);
/* time.c */
uint msec(void);
Timer *timerstart(int);
