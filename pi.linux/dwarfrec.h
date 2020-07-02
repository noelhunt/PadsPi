#ifndef DWARFREC_H
#define DWARFREC_H

/* sccsid[] = "%W%\t%G%" */

#define OFFSET(x) offsetof(DwarfAttrs, x), offsetof(DwarfAttrs, have.x)

typedef struct AParse AParse;

struct AParse {
	int name;
	int off;
	int haveoff;
	int type;
};

struct DwarfState {
	DwarfBuf 	b;
	ulong		unit;
	uint		uoff;
	ulong		aoff;
	int		allunits;
	ulong		nextunit;
	int		depth;
	uchar		children;
};

class DwarfRec {
	friend class Dwarf;
	DwarfBuf	b;
	ulong		_unit;
	uint		_uoff;
	ulong		_aoff;
	int		_depth;
	int		allunits;
	ulong		nextunit;
public:
			DwarfRec(const DwarfRec&);
			DwarfRec();
	ulong		tag;
	int		children;
	DwarfAttrs	attrs;
	ulong		unit();
	uint		uoff();
	uint		aoff();
	int		depth();
	DwarfState	getstat();
	void		setstat(DwarfState);
	void		clear();
};
#endif
