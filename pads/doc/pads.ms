.\" From tac@research Mon Jun  5 10:56:08 EDT 1989
.\" Wed May 21 16:39:30 EDT 1986
.am DS
.ft CW
..
.am DE
.ft R
..
.de CW
\\f(CW\\&\\$1\\fP\\$2
..
.de B3
.sp 0.5
.ft CW
.nf
.B1
..
.de B4
.sp 0.5
.ft R
.fi
.B2
.sp 1
..
.TL
Pads Programming Guide \- Revision 1
.AB
.PP
.CW Pads
is an object-oriented user interface package for bitmap terminals, most
suitable for browser-like applications.
Written in C++ and linked with the
.CW Pads
library, an application program instantiates objects and describes them to
.CW Pads ,
creating windows and lines of text, to which the objects are bound.
Asynchronously,
.CW Pads
manages the display of this set of overlapping, scrolling windows 
and maps each semantic operation from the user to a call of a member function
of the appropriate object.
.PP
In the current implementation the application is a timesharing process on
a UNIX\(dg system, communicating with a generic terminal process in a 
DMD 5620 or Blit terminal.
The abstractions used in the
.CW Pads
interface make few assumptions about the terminal's properties and
it should be possible to use
.CW Pads
in a variety of other environments.
.PP
This document contains: (i) an explanation of
.CW Pads '
non-sequential programming model,
(ii) a description of the mechanics of the 5620/Blit human interface,
(iii) some sample programs,
(iv) the details of programming with
.CW Pads ,
which assumes some familiarity with C++.
.FS
\(dg UNIX is a trademark of AT&T Bell Laboratories.
.FE
.AE
.NH
Programming Model
.PP
.CW Pads
is a bitmap user interface package based on an object-oriented programming model.
An
.I object
in
.CW Pads
is the instantiation of a
.CW C++ [1]
class: a collection of data with a set of operations.
The human user is presented with textual descriptions of objects,
each of which is augmented with a set of operations.
When the user selects an object on the screen, its set of operations becomes
available in a menu.
If one of the menu entries is selected, its operation is invoked on the
object.
A line of input from the keyboard may also be directed to an object.
.PP
The global control flow in such a program is unlike the sequential flow of
a conventional procedure-oriented program.
The execution of a
.CW Pads
program is driven by direct invocations of its objects' member functions.
The application has no global control for interpreting an input stream;
member functions are invoked from an external control mechanism.
Usually each invocation results in a change to the display,
showing the user some new or changed information.
Each function invocation is completed in isolation and must leave
the program ready to accept the user's next operation.
.PP
Compared to procedural software, more of the program is devoted to deciding
what set of applicable operations should be presented to the user and
less to trying to guide the user and interpret input.
Error handling techniques illustrate the difference.
Suppose a program finds itself in the middle of a complex operation when it
cannot open a file.
In the procedural model, the program might:
.IP
(a) issue an error message and abort the operation.
The user must fix the problem and re\%start the the operation,
possibly having to repeat a lot of work to recover the lost context.
.IP
(b) issue an error message and pause with a prompt
that lets the user choose whether to try again or give up.
This at least gives the user a chance to continue.
But the program itself cannot help to fix the problem; it is stuck waiting for
an answer to the question.
.LP
In
.CW Pads ,
when an object runs into trouble, it can report the problem and place
a set of recovery actions in its menu.
It does nothing more until the user chooses to send it another operation.
The user can communicate with other objects to fix the problem,
returning to it arbitrarily later, or ignoring it forever.
Effectively, an object
.I cannot
pose a question to the user and demand a response before the program as a whole
proceeds.
To do so it would have to arrange that all other objects remove their menus,
leaving the user no choice.
Rather, it must leave itself in a state from which it can continue, at the
user's behest.
.PP
Different implementations of this model are possible.
.CW Pads
runs the application as a host process, communicating with a generic
process in the terminal.
The host creates objects and binds them to windows or lines of text.
For each object a textual description and set of operations is sent to the
terminal.
The terminal program presents them to the user as a set of overlapping,
scrolling windows.
The user selects a current window, or a line within a window, and makes menu
selections.
Each menu selection results in a message to the host that invokes
the corresponding member function of the object bound to the window or line.
.PP
Host-terminal communications are asynchronous;
there is no notion of a reply, result or acknowledgement.
The host may send object descriptions to the terminal arbitrarily;
the terminal may send operations to the host arbitrarily.
Only the order of messages along each path is guaranteed.
.NH
Human Interaction
.PP
The remainder of this document assumes the terminal to be a DMD 5620 or Blit.
These are the only terminals for which
.CW Pads
has been implemented to date.
Other terminals, say with a one-button mouse, would behave differently.
.PP
The application's objects are displayed as a set of windows containing lines of
text.
The mouse controls window positioning, overlap and scrolling
and selects a current window and line.
Operations are directed to the current window or line from pop-up menus
or the keyboard.
.PP
The left mouse button points.
Pointing at a window makes it current, with a heavy border;
pointing at a line makes it current, video-inverted.
The black scroll bar at the left of each window shows proportionally how
much of the text of a window is visible;
pointing into the scroll region repositions it.
To provide finer scrolling in large files,
selecting the top or bottom line moves it to the middle of the window.
.PP
The middle mouse button menu has line-level operations; they apply to the
current line.
It is in two parts.
The operations above the separator are specific to each line.
The operations below the separator are generic line operations:
.IP
.B cut
removes the line;
.B sever
removes the line and all lines above it;
.B fold 
folds a line that is wider than its window, so all of it appears;
.B truncate
truncates a wide line at the right, so it occupies only one physical line.
.LP
The right mouse button menu has window-level operations.
It is in three parts.
The operations above the upper separator are specific to each window.
Below the lower separator is a list of all the available windows;
selecting one makes it current.
They appear in front-to-back screen order, current at the top.
The operations between the separators are generic window operations:
.IP
\fBreshape, move, close\fP are familiar;
.B fold
and
.B truncate
apply to all the lines in the window.
.PP
Some windows and lines accept input from the keyboard.
Keyboard characters accumulate at the bottom of the layer.
If the current line accepts input, it flashes with each keystroke;
otherwise, if the current window accepts input, its border flashes.
If neither flashes, carriage return is ignored until a line or window that does
flash is selected.
On carriage return, the whole input line is sent to the line or window.
Hitting backspace to see what flashes is a safe way to find out where an input
line would go.
Each line or window that accepts keyboard input produces a one-line help
message in response to
.CW ? .
.PP
The arrow-dot-dot-dot cursor icon indicates that the host is still completing
an operation; the terminal is ready
.I asynchronously.
The exclamation mark icon calls for confirmation of a menu
selection by pressing that menu's button again.
.NH
Sample programs
.PP
The null
.CW Pads
program is:
.DS
#include <pads.pub>

void main()
{
	PadsInit();
	PadsServe();
}
.DE
It is compiled with the header file
.CW /usr/jerq/include/pads.pub
and the library
.CW /usr/jerq/lib/libpads.a .
These pathnames may vary from system to system.
The host code does not depend on the type of terminal;
the same files are used for 256K and 1M DMD 5620 and 68000 Blit.
The program is compiled:
.DS
CC null.c -I/usr/jerq/include /usr/jerq/lib/libpads.a
.DE
When the program is run,
.CW PadsInit()
downloads the terminal program.
It must be the first call to
.CW Pads .
Depending on the
.CW TERM
environment variable the download is
.DS
32ld /usr/jerq/mbin/pads.m
.DE
or
.DS
68ld /usr/blit/mbin/pads.m
.DE
.PP
.CW PadsServe()
hands control of the host process to
.CW Pads .
It never returns; from that point the global control flow of the program
remains with
.CW Pads .
The application code is now passive, invoked by calls from
.CW Pads .
.CW PadsServe()
reads requests from the terminal to invoke operations on objects defined by
the application.
In this program there are none; nothing more can happen.
The host process blocks reading from the terminal in
.CW PadsServe() .
.PP
To produce output, the program must create at least one pad (window).
To receive input, a pad must be bound to an instantiation of one of the program's
classes, and a menu of operations must be supplied.
.PP
The following program creates a window that displays the date when
.CW 'date'
is selected from its only pad's menu:
.DS
#include <pads.pub>
#include <CC/stdio.h>

class Date : public PadRcv {
	class Pad	*pad;
	void		date();
			Date();
};

Date.Date()
{
	Menu m;
	pad = new Pad( (PadRcv*) this );
	pad->banner( "Current Date" );
	pad->name( "date" );
	m.first( "date", (Action) &date );
	pad->menu( m );
	pad->makecurrent();
}

void Date.date()
{
	char *ctime(long*);
	long time(long*), t;
	time(&t);
	pad->insert( 1, "%s", ctime(&t) );
}

void main()
{
	char *error = PadsInit();
	if( error ){
		fprintf( stderr, "%s", error );
		exit(1);
	}
	new Date;
	PadsServe();
}
.DE
.PP
After downloading and checking for an error return from
.CW PadsInit() ,
.CW main()
constructs an instance of
.CW Date ;
.CW Date.Date()
in turn constructs a
.CW Pad :
.sp
.B3
class Pad {
public:
	        Pad(PadRcv *);
	void    banner(char* ...);
	void    name(char* ...);
	void    insert(long, Attrib, PadRcv*, Menu&, char* ...);
	void    insert(long, Attrib, char* ...);
	void    insert(long, char* ...);
	void    makecurrent();
	void    menu(Menu&);
};
.B4
Definitions like this from
.CW pads.pub
are shown in boxes, and for simplicity may be incomplete.
Check
.CW pads.pub
for complete definitions.
.PP
In order to pass itself as the argument to
.CW Pad.Pad()
and become the recipient of operations from the pad's window
in the terminal,
.CW Date
is derived from
.CW PadRcv ,
a
.CW Pads
base class.
The definition of
.CW PadRcv
is not needed yet.
After the construction,
.CW pad
points to an embryonic pad: empty, anonymous and impotent. 
The pointer is, of course, to an object in the host process, an agent
representing an object in the terminal.
.PP
Several different functions are called to establish the pad's properties.
.CW Pad.banner()
takes
.CW printf
arguments producing a string,
.CW 'Current\ Date' ,
to appear at the top of the pad's window.
Similarly, 
.CW Pad.name()
defines a string,
.CW 'date' ,
by which the pad is to be known in the list of pads at the bottom
of the button 3 menu.
.PP
Menus are built using the
.CW Pads
class
.CW Menu :
.sp
.B3
typedef void (*Action) (...);

class Menu {
public:
	        Menu();
	        ~Menu();
	        Menu(  char*, Action, long = 0 );
	void    first( char*, Action, long = 0 );	
	void    last(  char*, Action, long = 0 );	
	void    sort(  char*, Action, long = 0 );
};
.B4
The void constructor makes an empty menu.
The functions in
.CW Menu
add items into the menu.
.CW Menu.first()
adds an item to the top of the menu.
In this example
.CW Menu.first()
is called only once, resulting in a one-element menu.
The string,
.CW 'date' ,
is to be presented to the user, if the menu is raised.
The function
.CW date() ,
i.e.
.CW Date.date() ,
is to be called if the item is selected from the menu.
The third (optional) argument of
.CW Menu.first()
is a 32-bit argument to be passed along when the function is called.
.CW Menu.last()
adds to the bottom of the menu.
.CW Menu.sort()
insertion sorts a single entry into the menu.
(It doesn't sort the whole menu.)
.CW Pad.menu()
binds a menu to a pad;
the menu appears as the top segment of the menu raised by button 3,
when the pad is selected.
The pointer passed to
.CW Pad 's
constructor determines on which instance of the class
.CW Date
the function
.CW Date.date()
is invoked.
.PP
The last statement in
.CW Date.Date()
calls
.CW Pad.makecurrent()
to force the pad to be made visible and selected as current.
If hidden, this bring a pad to the ``front'' of the screen;
if not displayed at all \- the initial state of a pad \- this requests
the user to sweep a rectangle for it.
.PP
After this initialization
.CW main()
calls
.CW PadsServe() ,
but nothing will happen until
.CW 'date'
is selected from the menu.
Then, out of the blue,
.CW Date.date()
is called to determine the time and write it to the pad.
The pad has the default form:
a sequence of lines ordered by a 32-bit key associated with each inserted line
of text.
Key
.CW 1
is the top line; key
.CW 2**32-1
is the bottom.
In
.CW Date.date()
.DS
pad->insert( 1, "%s", ctime(&t) );
.DE
inserts the string constructed from the
.CW printf
arguments as the top (and only) line in the pad.
If
.CW date()
is re-invoked, it inserts again with the same key and overwrites the
previous date.
.PP
The next program is a toy news browser.
It uses an unbounded number of pads to present the contents of
.CW /usr/news .
Its first pad displays a list of file names from
.CW /usr/news ,
one per line.
Selecting a line and choosing
.CW 'open'
from its menu creates a new pad, showing the file's contents.
.DS
#include <pads.pub>
#include <CC/stdio.h>

class Story : public PadRcv {
	char	file[16];
	FILE	*fp;
	long	lines;
	Pad	*pad;
	void	linereq(long,Attrib);
public:
	void	open();
		Story(char *f)	{ strncpy( file, f, strlen(f)-1 ); }
};

class News  : public PadRcv {
	Pad	*pad;
public:
		News();
};

void main()
{
	if( chdir( "/usr/news" ) ) exit(1);
	char *error = PadsInit();
	if( error ){
		fprintf( stderr, "%s", error );
		exit(1);
	}
	new News();
	PadsServe();
}

News.News()
{
	Story *s;
	FILE *fp, *popen(char*,char*);
	Menu m( "open", (Action) &Story::open );
	char file[16];
	long uniq = 0;
	pad = new Pad( (PadRcv *) this );
	pad->options(TRUNCATE|SORTED); 
	pad->banner( "News:" );
	pad->name( "News" );
	pad->makecurrent();	
	if( !(fp = popen("ls", "r")) ){
		pad->insert( 1, "can't ls" );
		return;
	}
	while( fgets(file, 16, fp) ){
		s = new Story( file );
		pad->insert( ++uniq, (Attrib) 0, (PadRcv*) s, m, "%s", file );
	}
	pclose(fp);
}

void Story.open()
{
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

void Story.linereq( long i, Attrib a )
{
	char buf[256];
	long n;

	fseek( fp, 0, 0 );
	for( n = i; n > 0; --n )
		fgets( buf, 256, fp );
	pad->insert( i, a, "%s ", buf );
}
.DE
.PP
.CW News.News()
calls
.CW Pad.options()
to change the properties of its pad.
.CW TRUNCATE
specifies that lines in the pad be truncated at the right when their
length exceeds the current width of the pad.
.CW SORTED
specifies that the lines in the pad be ordered by dictionary order rather
than by key.
.CW TRUNCATE
and
.CW SORTED
are constants of type
.CW Attrib .
.PP
For each file in
.CW /usr/news ,
.CW News.News()
creates a
.CW Story
and inserts the name of the file into the pad.
When the
.CW Story
is created,
.CW Story.Story()
merely copies the name of the file to a private area.
The call to
.CW Pad.insert()
accomplishes quite a lot:
.DS
pad->insert( ++uniq, (Attrib) 0, (PadRcv*) s, m, "%s", file );
.DE
.CW ++uniq
generates an increasing sequence of unique keys.
Since the pad is
.CW SORTED ,
it doesn't matter what keys are used, but they must differ to prevent
an earlier story being overwritten.
The
.CW Attrib
argument is the set of attributes to be set for the line \- none in this case.
The
.CW PadRcv*
and
.CW Menu&
arguments bind an object and menu to the line.
In this case a singleton menu is constructed by giving arguments
directly to
.CW Menu.Menu() .
Nothing distinguishes the objects and menus that are bound to lines from
those that are bound to pads.
The
.CW printf
arguments (necessarily at the end) give the text for the line.
.PP
When the
.CW 'open'
operation is selected on one of these lines,
.CW Story.open()
is called on the corresponding
.CW Story .
For a given
.CW Story ,
only the first call to
.CW open()
is interesting.
If
.CW open()
has been called previously, it merely makes its pad current.
On the first call it creates a pad, reads its file and arranges that
the text of the file be sent to the terminal, on demand.
By creating the pad whether the file is successfully opened or not, it
has somewhere to write an error message if the open fails.
If the file is opened successfully, the length is counted in
.CW lines
and passed to
.CW Pad.lines() .
.PP
.CW Pad.lines()
radically changes the nature of the pad.
Instead of waiting for lines of text to be inserted by the host, the terminal
now knows the exact number of lines in the pad and actively requests the
lines it needs from the pad's object.
As the user scrolls the window and the terminal needs to display a line that is
doesn't have, a request for the line is sent to the pad's object in the host.
To receive the request the host object must define
.CW linereq() ,
one of the virtual functions in
.CW class\ PadRcv :
.sp
.B3
class PadRcv {
public:
virtual	void    cycle();
virtual	void    help();
virtual	void    kbd(char*);
virtual	void    linereq(long,Attrib=0);
virtual	void    userclose();
virtual	void    usercut();
};
.B4
.CW Story.linereq()
will not be called until after the user sweeps out a rectangle in response
to the call to
.CW Pad.makecurrent() .
If the rectangle is big enough to hold 3 lines, say, then there will be
three calls to
.CW linereq() :
.DS
Story.linereq( 1, (Attrib) 0 );
Story.linereq( 2, (Attrib) 0 );
Story.linereq( 3, (Attrib) 0 );
.DE
Host code should make no assumptions about the sequence of calls it will
receive from the the terminal.
In particular, the same line may be requested more than once.
The terminal caches only a bounded number of lines for a given pad and may
re-request those it has discarded.
(However, the implementation of the
.CW Pads
host-terminal protocol is optimized for the most common pattern,
a sequential run of line numbers \- as this program ought to be!)
.PP
.CW linereq() 's
.CW Attrib
argument will always be
.CW 0
on calls from the terminal.
The argument is provided so that calls from the host may specify attributes for the
inserted line.
Some of the attributes for lines are:
.DS
SELECTLINE
FOLD
TRUNCATE
.DE
.CW FOLD " and"
.CW TRUNCATE
specify whether a line should be truncated, overriding the pad's attribute.
.CW SELECTLINE
causes the line to be selected as the current line, as though the user
had pointed to it.
To see how this can interact with
.CW linereq() ,
consider adding a (silly) command to display the middle of a news story.
A function
.CW middle()
is added to
.CW Story :
.B /n/seki/usr/tac/pads/doc/story.c
and a menu is added to the pad in
.CW Story.open() :
.B /n/seki/usr/tac/pads/doc/storyopen.c
Now if
.CW 'middle'
is selected,
.CW middle()
is called.
For a 30-line story, say,
.CW middle() " calls"
.DS
linereq( 15, SELECTLINE )
.DE
The
.CW SELECTLINE
attribute requests the terminal to select the line, as though the user
had pointed to it; and therefore forces the terminal to display the line.
Without this the line might not be displayed unless the user scrolls to see it.
In fact, the terminal displays a
.CW SELECTLINE
line as close to the physical center of the window as possible.
If the surrounding lines are needed,
.CW linereq()
requests will be sent to the host to fetch them.
Host code cannot tell the difference between a
.CW linereq()
generated by the user scrolling the pad and a
.CW linereq()
generated by other host code.
Notice that
.CW middle()
doesn't need to know anything about the size of the window to get a specific
context displayed.
.NH
The PadRcv Base Class
.PP
Skip this section on first reading.
.PP
All host objects that are to receive operations from the terminal must be
instantiations of some application-defined class derived from
.CW class\ PadRcv :
.DS
class Any : public PadRcv {
	...
};
.DE
.B3
class PadRcv {
       OBJ_ID obj_id;
public:
              PadRcv();
              ~PadRcv();
        void  invalidate();
};
.B4
.PP
.CW PadRcv.obj_id
guarantees that operations from the terminal aren't applied
to deleted host objects.
An object is bound to a pad or line by both its address and its unique
.CW obj_id ;
an operation from the terminal is applied only if the identifiers agree.
An in-flight operation may be directed to an object that is concurrently removing
its pad/line and deleting itself and won't exist when the packet arrives.
When deleted,
.CW PadRcv.~PadRcv()
changes
.CW obj_id
so that the operation is ignored.
.PP
The object may explicitly inhibit operations from all the pads and lines to which
it is currently bound by calling
.CW invalidate() .
.PP
.B3
class PadRcv {
public:
virtual char    *kbd(char*)   { return "sorry - no kbd"; }
virtual char    *help()       { return "sorry - no help"; }
};
.B4
If attribute
.CW ACCEPT_KBD
is specified for a pad or line,
.CW kbd()
is called when the user selects the pad or line and enters a line
of text from the keyboard.
There is no
.CW \\\\n " or"
.CW \\\\r
at the end of the string.
A non-zero return from
.CW kbd()
is an error message for the user.
.CW ACCEPT_KBD
is set implicitly by
.CW Pad.Pad(obj) ,
if
.CW obj
defines
.CW kbd() ,
but must be set explicitly for lines:
.DS
pad->insert(key, ACCEPT_KBD ...);
.DE
If the keyboard string is exactly
.CW '?' ,
.CW help()
is called instead of
.CW kbd() .
.CW help()
should return a string to be presented to the user in a single line.
.PP
.B3
class PadRcv {
public:
virtual void   userclose();
virtual void   usercut();
};
.B4
When the user closes a window or cuts a line on the screen, the host object
may be informed.
If the attribute
.CW USERCLOSE(USERCUT)
is specified for the pad(line),
.CW userclose(usercut)
is called for the host object when the user closes(cuts).
All record of the pad(line) in the terminal is removed unless the
. CW DONT_CLOSE(DONT_CUT)
attribute is set.
.CW USERCLOSE
is set implicitly by
.CW Pad.Pad(obj) ,
if
.CW obj
defines
.CW userclose() .
.PP
.B3
class PadRcv {
public:
virtual void    cycle();                /* see Pad.alarm() */
virtual void    linereq(long,Attrib=0); /* see Pad.lines() */
virtual int     accept(Action);         /* see Pick()      */
};
.B4
.NH
Class Pad
.PP
Nearly all communication with the terminal is through class
.CW Pad :
.B3
class Pad {
        PadRcv	*object;
public:
                Pad(PadRcv *);
                ~Pad();
        void    banner(char* ...);
        void    name(char* ...);
        void    options(Attrib, Attrib=0);
        void    makecurrent();
        void    menu(Index);
        void    menu(Menu&);
};
.B4
.PP
.CW Pad.Pad(obj)
initializes a pad bound to
.CW obj.
The default attribute
.CW FOLD
is set.
The attributes
.CW ACCEPT_KBD " and"
.CW USERCLOSE
are set, if
.CW obj
defines
.CW kbd() " and"
.CW userclose() ,
respectively.
Default name and banner are set to
.CW '<name>' " and"
.CW '<banner>' .
The name appears in the list of pads at the bottom of the button 3 menu.
The banner appears at the top of the pad's window.
.CW Pad.~Pad()
removes all record of the pad; the window in the terminal vanishes.
.CW Pad.name() " and"
.CW Pad.banner()
take
.CW printf
arguments to redefine the name and banner.
.CW Pad.options(on,\ off)
switches on the first set attributes and then switches off the second (optional)
set.
The attributes for pads are:
.B3
typedef unsigned short Attrib;

#define FOLD            ((Attrib) 0x.... )
#define TRUNCATE        ((Attrib) 0x.... )
#define SORTED          ((Attrib) 0x.... )
#define ACCEPT_KBD      ((Attrib) 0x.... )
#define USERCLOSE       ((Attrib) 0x.... )
#define DONT_CLOSE      ((Attrib) 0x.... )
.B4
Each is represented by a separate bit; sets are composed by bitwise or.
.CW FOLD
specifies that lines longer than the width of the window be folded so that the
whole text is displayed.
.CW TRUNCATE
specifies that long lines be truncated at the right.
These can be overridden as individual lines are inserted into the pad.
The user can also override any host setting on pad or line.
.CW SORTED
specifies that the lines in the pad be displayed by dictionary order rather
by numeric key.
For the effect of
.CW ACCEPT_KBD " and"
.CW USERCLOSE " see"
.CW PadRcv.kbd() " and"
.CW PadRcv.userclose() ,
respectively.
.CW DONT_CLOSE
inhibits the user from closing the window; attempts are still reported if
.CW USERCLOSE
is set.
Combinations:
.CW DONT_CLOSE|USERCLOSE
means the host has complete control;
.CW USERCLOSE
means the user can close unilaterally and the host is told it happened;
.CW DONT_CLOSE
means the user can't close and the host doesn't care;
neither means the user can close and the host isn't told.
.PP
.CW Pad.makecurrent()
forces the terminal to bring the pad's window to the front of the screen
and select it as current.
If no window exists for the pad \- the initial state \- the terminal
solicits the user (once) to sweep out a rectangle.
.CW Pad.menu()
takes a
.CW Menu
(or a menu's
.CW Index
\- ignore
.CW Index
on first reading) and binds the corresponding menu to the pad.
.B3
class Pad {
public:
        void    insert(long, Attrib, PadRcv*, Index, char* ...);
        void    insert(long, Attrib, PadRcv*, Menu&, char* ...);
        void    insert(long, Attrib, char* ...);
        void    insert(long, char* ...);
        void    removeline(long);
        void    clear();
        void    tabs(short);
        void    lines(long);
        void    createline(long, long);
        void    createline(long);
};
.B4
.PP
.CW Pad.insert()
puts a line of text into a pad.
The first argument is a non-zero 32-bit key.
The trailing
.CW printf
arguments are the text, which is truncated at about 250 characters.
Only lines corresponding to keys for which strings have been
inserted are displayed.
Line
.CW 1
is displayed at the top of the window and line
.CW 2**32-1
at the bottom.
The second (and subsequent) insertions for a given key overwrite the original.
.CW Pad.removeline(k)
deletes the line with key
.CW k .
Tab stops are set every 8 characters.
The default is overridden for each pad individually by
.CW Pad.tabs() .
Non-printable characters other than tab are replaced by blanks.
.PP
The attributes that may be set on a line are:
.B3
typedef unsigned short Attrib;

#define SELECTLINE      ((Attrib) 0x.... )
#define ACCEPT_KBD      ((Attrib) 0x.... )
#define FOLD            ((Attrib) 0x.... )
#define TRUNCATE        ((Attrib) 0x.... )
#define DONT_CUT        ((Attrib) 0x.... )
#define USERCUT         ((Attrib) 0x.... )
#define FLUSHLINE       ((Attrib) 0x.... )
.B4
.CW SELECTLINE
causes the line to be selected as though the user had pointed to it.
.CW FOLD " and"
.CW TRUNCATE
override the pad.
.CW DONT_CUT
makes the line immune to the
.CW 'cut' " and"
.CW 'sever'
terminal operations that remove lines;
.CW USERCUT
calls
.CW PadRcv.usercut()
if the line is cut;
they behave like
.CW DONT_CLOSE
and
.CW USERCLOSE
on pads.
.CW FLUSHLINE
prevents the line from being buffered in the host, but should be needed very rarely.
If
.CW Pads '
buffering heuristic works, the application shouldn't know about this.
.PP
An object is bound to a line by the
.CW PadRcv* " and"
.CW Menu& " (or"
.CW Index )
arguments.
This is just like binding to a pad, except that no attributes are set implicitly.
.PP
.CW Pad.lines(n)
changes the pad's key space from
.CW 1..2**32-1 " to"
.CW 1..n .
It also makes the terminal partially responsible for deciding what lines
should be inserted.
The host can still insert lines at will.
As the user scrolls the window, a call
.CW linereq(k,0)
instructs the pad's object to insert line
.CW k .
So
.CW linereq()
should be defined:
.B3
class PadRcv {
public:
virtual void linereq(long, Attrib=0) { /* do nothing */ }
};
.B4
.LP
There can be no assumptions made about the order in which calls to
.CW linereq()
will occur.
The terminal caches only a bounded number of lines and (re-)requests them
from the host as the user scrolls the window.
However, a common pattern of calls is
.DS
linereq(m,0), linereq(m+1,0) ... linereq(n-1,0), linereq(n,0) 
.DE
for which it may be worth optimizing.
The
.CW Attrib
argument is included for the convenience of calls from host code.
.PP
In pads for which
.CW lines()
has not been called,
.CW Pad.createline(i)
inserts a dummy line with key
.CW i
and
.CW Pad.createline(lo,hi)
inserts a set of dummy lines with keys
.CW lo..hi .
.CW linereq()
is called if any of these lines is to be displayed.
(Theoretically,
.CW createline()
subsumes
.CW lines() ,
but the current implementation is inefficient for large key ranges.)
.PP
.CW Pad.clear()
removes all lines from the pad.
.PP
The terminal's real-time clock can be used to invoke host operations
periodically.
.B3
class Pad {
public:
        void    alarm(short=0);
};

class PadRcv {
public:
virtual void    cycle();
};
.B4
At least n seconds after
.CW pad->alarm(n)
is called,
.CW cycle()
is called on
.CW pad 's
object.
For a given pad, if there is an outstanding call to
.CW alarm() ,
results are undefined for further calls to
.CW alarm()
before
.CW cycle()
is called.
.CW cycle()
is called for
.CW alarm(0)
as soon as all currently outstanding operations from the user and pending
.CW cycle() 's
have been completed.
Thus an arbitrary number of host objects can use
.CW alarm(0)
for round-robin scheduling, with interleaved user operations.
.NH
Class menu
.PP
Class
.CW Menu
is used to build menus.
Menus are bound to windows and lines by
.CW Pad.menu() " and"
.CW Pad.insert() .
.B3
typedef void (*Action) (...);

class Menu {
public:
                Menu();
                ~Menu();
	void 	first( char*, Action, long=0 );	
	void 	last(  char*, Action, long=0 );	
	void 	sort(  char*, Action, long=0 );
};
.B4
.PP
Objects of type
.CW Menu
are usually declared as automatic (stack) variables.
The lifetime of
.DS
Menu m;
.DE
need only be long enough to bind it to a window by
.DS
pad->menu( m );
.DE
or to a line 
.DS
pad->insert( ... m ... );
.DE
.CW Pad.insert() " and"
.CW Pad.menu()
extract the current value of 
.CW m ;
they do not retain a reference to
.CW m .
Subsequent changes to
.CW m
are not reflected in the menu presented for those objects to which
it was bound earlier.
.PP
The void constructor initializes an empty menu.
.CW Menu.first() ,
.CW Menu.last() " and"
.CW Menu.sort()
add entries to the menu.
Their
.CW char*
argument is the string to appear as one entry in the menu presented to the
user.
The string can be temporary;
.CW Menu
takes a copy of it.
The
.CW Action
argument is the function to be invoked if the entry is selected.
The optional
.CW long
argument is to be passed on to that function as an argument.
The arguments
.DS
( "next year", (Action) &calendar, 1985 )
.DE
would result in the call
.DS
object->calendar(1985)
.DE
which is implemented as the equivalent of the C call
.DS
calendar(object,1985)
.DE
The
.CW object
is the
.CW PadRcv*
pointer passed to either
.CW Pad.Pad()
or
.CW Pad.insert() .
.PP
At any time, a
.CW Menu
is a sequence of such entries.
.CW Menu.first()
adds a new entry to the beginning of the sequence.
.CW Menu.last()
adds a new entry to the end.
.CW Menu.sort()
inserts before the first string lexically greater than its argument.
The
.I order
of a set of calls to these functions is important.
For example,
.DS
Menu m1, m2;

m1.first( "z" ... );	/* z */
m1.first( "x" ... );	/* x, z */
m1.last(  "a" ... );	/* x, z, a */
m1.sort(  "q" ... );	/* q, x, z, a */
m1.sort(  "p" ... );	/* p, q, x, z, a */

m2.sort(  "q" ... );	/* q */
m2.sort(  "p" ... );	/* p, q */
m1.first( "x" ... );	/* x, p, q */
m2.first( "z" ... );	/* z, x, p, q */
m2.last(  "a" ... );	/* z, x, p, q, a */
.DE
.PP
A second constructor initializes a menu to have a single entry:
.B3
class Menu {
public:
		Menu( char*, Action, long=0 );
};
.B4
.PP
If an entry string ends with a question mark, e.g.
.DS
m.first( "delete all?" ... );
.DE
the terminal makes the user confirm the selection before passing it
to the host.
.PP
The leftmost character of the string with bit
.CW 0200
set is treated as a ``padding'' character.
The padding character
.CW 0200|'c'
is replaced by sufficiently many copies of
.CW 'c'
to make the string as wide as the widest string in the menu.
For example, the strings
.DS
"a\\\\240reg"
"table\\\\240arg"
"\\\\255"
"main()\\\\25675"
"f()\\\\256123"
.DE
would appear in a menu as
.DS
a     reg
table arg
---------
main().75
f()...123
.DE
.CW 040 ,
.CW 055 " and"
.CW 056 " are"
.CW '\ ' ,
.CW '-' " and"
.CW '.' ,
respectively.
(The expansion is done by
.CW menuhit()
in the terminal .)
.NH
More about Menus
.PP
Skip this section on first reading.
.PP
Rebuilding a menu every time it is needed does not create a new copy
of the menu in the host or terminal.
For each distinct menu entry, the incremental memory used in the terminal
is the size of the string.
For each distinct menu, the incremental memory in the terminal is 2 bytes
per entry.
More memory is used for host data structures, but linear in distinct entries
and menus.
For most applications it should not be necessary to worry about the
way menus are implemented.
The application programmer's effort ought to go into composing good menus.
A built-in pad can be instantiated by
.CW NewPadSats()
to report how much memory is being consumed by menus.
.PP
If profiling indicates that too much time is consumed on the
host by repeatedly building the same menu, it is possible to obtain a menu's
.I index ,
the key by which it is known internally to Pads.
Once extracted, this key may be passed repeatedly to
.CW Pad.menu() " and"
.CW Pad.insert() .
.B3
class Index {
public:
	int	null();
};

class Menu {
public:
	Index	index(char* =0, Action=0, long=0);
};

class Pad {
public:
	void 	insert(long, Attrib, Univ*, Index, char* ...);
	void 	menu(Index);
};
.B4
.PP
The representation of the null
.CW Index
is zeros.
A statically allocated
.CW Index
is initially null,
tested by
.CW Index.null() .
This permits the following style of code, in which the menu is
built once, but used many times.
.DS
void Any.linereq( long k, Attrib a = 0 )
{
	static Index ix;

	if( ix.null() ){
		Menu m;

		m.first( ... );
		.
		.
		.
		ix = m.index();
	}
	pad->insert( k, a ... ix ... );
}
.DE
.PP
Reference to a pre-computed menu can also be inserted into another menu.
.B3
class Menu {
public:
	void 	first(Index);
	void 	last(Index);
};
.B4
A menu is more than a sequence of atomic entries, as stated above.
Each member of the sequence can also be another menu.
This recursive structure permits arbitrary trees.
.PP
If arguments are given to
.CW Menu.index()
the menu is treated differently when incorporated in another menu.
The string appears in the parent menu as a regular entry with an
associated action.
The sub-menu is only reached if the user explicitly selects it to be
displayed.
.PP
.CW NumericRange()
makes it easier to build numeric (sub-)menus.
.B3
Index NumericRange(short, short);

class PadRcv {
public:
virtual void    numeric(long);
};
.B4
.DS
Index ix = NumericRange(lo, hi);
.DE
is (almost) equivalent to
.DS
Menu m;
for( long i = lo; i <= hi, ++i ){
        char dec[BIG_ENOUGH];
        sprintf( dec, "%d", i );
        m.last( dec, &numeric, i );
}
Index ix = m.index();
.DE
Of course, no such menu is built; the numeric range is represented by the pair
of shorts.
Moreover, the binding of
.CW numeric()
occurs through the regular virtual function calling mechanism when the
operation comes back from the terminal, not in the menu construction.
.NH
Miscellany
.B3
void NewHelp();
void NewPadStats();
long UniqueKey();
void PadsWarn(char* ...);
.B4
.PP
.CW NewHelp()
instantiates an object that creates a ``help'' pad, presenting
a synopsis of the Pads manual page.
.PP
.CW NewPadStats()
creates a pad that shows consumption of host and terminal memory.
.PP
Each call of
.CW UniqueKey()
returns a distinct value, monotonically increasing (at least to begin with.)
.PP
.CW PadsWarn()
takes
.CW printf
arguments and writes a message to the user.
It use is discouraged for routine communication with the user.
.NH
References
.IP 1.
B.S. Stroustrup,
``C++ Reference Manual'',
Computing Science Technical Report CSTR-108,
AT&T Bell Laboratories, 1984.
