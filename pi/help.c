#include <pads.h>
SRCFILE("help.c")

static char sccsid[ ] = "%W%\t%G%";

struct HelpTopicMap {
	const char *topic;
	long line;
};

#if RTPI
#include "help.rtpi"
#else
#include "help.pi"
#endif

class Help : public PadRcv {
	Pad	*pad;
	int	lines;
	void	linereq(long,Attrib);
	void	select(long);
public:
		Help();
	int	topic(const char*);
const	char	*enchiridion(long);
};

static Help *shelp;

int helptopic(const char *s)	{ return shelp->topic(s); }

const char *Help::enchiridion(long l){
	switch(l) {
		case HELP_OVERVIEW:	return "Help Window";
		case HELP_MENU:		return "Help Menu Bar";
		default:		return 0;
	}
}

Help::Help(){
	Menu m;
	int i;

	lines = helplines;
	pad = new Pad((PadRcv*) this);
	pad->lines(lines);
	pad->options(TRUNCATE);
	pad->banner("Help:");
	pad->name("help");
	for(i = 0; i < ntopics; i++)
	  m.sort(topicmap[i].topic, (Action)&Help::select, topicmap[i].line);
	pad->menu(m.index("topics"));
}


int Help::topic(const char *t){
	int i;
	int n = strlen(t);
	for(i = 0; i < ntopics; i++)
		if (!strncmp(t, topicmap[i].topic, n)) {
			pad->makecurrent();
			linereq(topicmap[i].line, DONT_DIRTY);
			break;
		}
	return i == ntopics ? 0 : 1;
}

void Help::linereq(long l, Attrib a){
	if (l>=1 && l<=lines)
		pad->insert(l, a, "%s", helptext[l-1]);
}

void Help::select(long l)	{ linereq(l, SELECTLINE); }
void NewHelp()			{ shelp = new Help; }
