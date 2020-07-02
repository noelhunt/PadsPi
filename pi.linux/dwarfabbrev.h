#define	INITHEAD(h, t) do {	\
	(&h)->front = (t *)(&h);\
	(&h)->back = (t *)(&h);	\
} while (0)

/* Back-linked list */

#define	LINKIN(h, t, e) do {		\
	(e)->rsib = (t *)(&h);		\
	if((&h)->front == (t *)(&h))	\
		(&h)->front = (e);	\
	else				\
		(&h)->back->rsib = (e);	\
	(&h)->back = (e);		\
} while (0)

#define ISABBR(p)	((p) != (DwarfAbbrev*)&abbrlh)
#define ISATTR(p)	((p) != (DwarfAttr*)&attrlh)

class DwarfAbbrev;
class DwarfAttr;

typedef struct DwarfAbbrevLh DwarfAbbrevLh;
typedef struct DwarfAttrLh DwarfAttrLh;

struct DwarfAbbrevLh {
	DwarfAbbrev *front, *back;
};

struct DwarfAttrLh {
	DwarfAttr *front, *back;
};

class DwarfAbbrevs {
	Dwarf		*dwarf;
	int		size;
	char		*_error;
	DwarfAbbrevLh	abbrlh;
public:
			~DwarfAbbrevs();
			DwarfAbbrevs(Dwarf*);	
	char		*parseabbrev(ulong);
	DwarfAbbrev	*findabbrev(ulong, ulong);
	DwarfAbbrev	*getabbrev(ulong, ulong);
	char		*error(){ return _error; }
};

class DwarfAbbrev {
	friend class Dwarf;
	friend class DwarfAbbrevs;
	ulong		aoff;
	ulong		num;
	ulong		tag;
	int		children;
	int		nattr;
	DwarfAttr	*ptr;
	DwarfAttrLh	attrlh;
	DwarfAbbrev	*rsib;
public:
			~DwarfAbbrev();
			DwarfAbbrev();
	DwarfAbbrev&	operator=(const DwarfAbbrev&);
	DwarfAttr	*next();
	DwarfAttr	*first();
	void		linkin(DwarfAttr*);
	void		done(int,int,int,int,int);
	void		dump();
};

class DwarfAttr {
friend	class		DwarfAbbrev;
	ulong		_name;
	ulong		_form;
	DwarfAttr	*rsib;
public:
			~DwarfAttr();
			DwarfAttr(ulong, ulong);
	ulong		name();
	ulong		form();
	void		dump();
};
