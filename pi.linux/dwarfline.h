#include <stdint.h>

enum {
	LNSExtendedOp = 0,
	LNSCopy,
	LNSAdvancePc,
	LNSAdvanceLine,
	LNSSetFile,
	LNSSetColumn,
	LNSNegateStmt,
	LNSSetBasicBlock,
	LNSConstAddPc,
	LNSFixedAdvancePc,
	LNSSetPrologueEnd,
	LNSSetEpilogueBegin,
	LNSSetIsa,

	LNEEndSequence = 1,
	LNESetAddress,
	LNEDefineFile
};

typedef struct LineInfo	LineInfo;
typedef struct LineData	LineData;
typedef struct LineAddr	LineAddr;

/* Information about a line.
   DIRECTORY is to be ignored if FILENAME is absolute.  
   PC will be relative to the file the debug_line section is in. */

struct LineInfo {
	ulong	file;
	long	line;
	ulong	col;
	ulong	pc;
	int	endseq;
};

/* Opaque status structure for the line readers. */
struct LineData;

/* The STOP parameter to next is one of line_stop_{file,line,col},
   perhaps ORed with StopPc; or StopAtend, or StopAlways. */

enum StopConstants {
	StopAtend	= 0,	/* Stop only at the end of a sequence. */
	StopFile	= 1,	/* Stop if DIRECTORY or FILENAME change. */
	StopLine	= 2,	/* Stop if LINE, DIRECTORY, or FILENAME change. */
	StopCol		= 3,	/* Stop if COL, LINE, DIRECTORY, or FILENAME change. */
	StopPosMask	= 3,
	StopPc		= 4,	/* Stop if PC changes. */
	StopAlways	= 8	/* Stop always. */
};

typedef enum StopConstants StopConstants;

struct LineData  {
	uchar		lsb;
	uint64_t	len;
	ushort		version;
	uint64_t	plen;		/* length of prolog */
	uint8_t		instrmin;	/* From the line number information header. */
	int8_t		isstmt;
	int8_t		lbase;
	uint8_t		lrange;
	uint8_t		opbase;
const	uint8_t		*stdlen;
	size_t		ndir;
const	uint8_t		**dirnames;
	size_t		finit;
	size_t		nfile;		/* As updated during execution of the table. */
const	uint8_t		**filenames;
const	uint8_t		*cpos;		/* Current position in the line table. */
const	uint8_t		*end;		/* End of this part of the line table. */
const	uint8_t		*init;		/* Start of the line table. */
	struct LineInfo	cur;
};

struct LineAddr {
	struct LineData	*ld;
	struct LineInfo	info;
};

/* Read in a word of fixed size, which may be
   unaligned,  in the appropriate endianness. */

#define read_16(p)(lnd->lsb		\
		    ?((p)[1] << 8 |(p)[0])	\
		    :((p)[0] << 8 |(p)[1]))

#define read_32(p)(lnd->lsb					    \
		    ?((p)[3] << 24 |(p)[2] << 16 |(p)[1] << 8 |(p)[0])  \
		    :((p)[0] << 24 |(p)[1] << 16 |(p)[2] << 8 |(p)[3]))

#define read_64(p) (lnd->lsb					    \
		    ? ((uint64_t) (p)[7] << 56 | (uint64_t) (p)[6] << 48    \
		       | (uint64_t) (p)[5] << 40 | (uint64_t) (p)[4] << 32  \
		       | (uint64_t) (p)[3] << 24 | (uint64_t) (p)[2] << 16u \
		       | (uint64_t) (p)[1] << 8 | (uint64_t) (p)[0])	    \
		    : ((uint64_t) (p)[0] << 56 | (uint64_t) (p)[1] << 48    \
		       | (uint64_t) (p)[2] << 40 | (uint64_t) (p)[3] << 32  \
		       | (uint64_t) (p)[4] << 24 | (uint64_t) (p)[5] << 16u \
		       | (uint64_t) (p)[6] << 8 | (uint64_t) (p)[7]))

typedef struct Pclu Pclu;

struct Pclu {
	ulong pc, line, unit;
};

class DwarfLine {
	int		lower;
	int		upper;
	uchar		*base;
	ulong		blen;
	ulong		addrsz;
	int		_size;
	int		_capacity;
	Pclu		*table;
	uchar		*data;
	uchar		*pend;
	int		lsb;
static	int64_t		readsleb128(LineData*);
static	uint64_t	readuleb128(LineData*);
static	void		skipleb128(LineData*);
	void		dealloc(LineData*);
	void		reset(LineData*);
	int		nextline(LineData*, LineInfo*, StopConstants);
	int		findaddr(LineData*, LineInfo*, LineInfo*, ulong);
	uchar		nextstate(LineData*);
	LineData	*lineopen();
	void		grow();
	void		push(Pclu&);
public:
			~DwarfLine();
			DwarfLine(uchar*,ulong,int,uint);
	int		findlo(ulong);
	int		findhi(ulong);
	char		*file(LineData*, long);
const	char		*newcu(ulong,ulong);
	Range		range(ulong);
	Range		range(ulong, int, int);
	void		bounds(ulong);
	void		bounds(ulong, int, int);
	Pclu		*line(int);
	Pclu		*sline();
	void		dump();
};
