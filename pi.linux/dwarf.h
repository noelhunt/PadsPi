#ifndef DWARF_H
#define DWARF_H

/* sccsid[] = "%W%\t%G%" */

#define NAME(x)	(x->attrs.name)
#define TYPE(x) (x->attrs.type)
#define UNIT(x) (x->unit)
#define UOFF(x) (x->uoff)
#define HAVE(x,y) (x->attrs.have.y)

typedef struct DwarfAttr   	DwarfAttr;
typedef struct DwarfAttrs  	DwarfAttrs;
typedef struct DwarfBlock	DwarfBlock;
typedef struct DwarfBuf		DwarfBuf;
typedef struct DwarfExpr	DwarfExpr;
typedef union  DwarfVal		DwarfVal;
typedef struct DwarfState	DwarfState;
typedef struct DwarfSrcs	DwarfSrcs;

enum {
	T_ARRAYTYPE = 0x01,
	T_CLASSTYPE = 0x02,
	T_ENTRYPOINT = 0x03,
	T_ENUMTYPE = 0x04,
	T_FORMALPARAM = 0x05,
	T_IMPORTEDDECL = 0x08,
	T_LABEL = 0x0A,
	T_LEXICALBLK = 0x0B,
	T_MEMBER = 0x0D,
	T_PTRTYPE = 0x0F,
	T_REFTYPE = 0x10,
	T_COMPILEUNIT = 0x11,
	T_STRING = 0x12,
	T_STRUCTTYPE = 0x13,
	T_SUBRTYPE = 0x15,
	T_TYPEDEF = 0x16,
	T_UNIONTYPE = 0x17,
	T_UNSPECPARAMS = 0x18,
	T_VARIANT = 0x19,
	T_COMMONBLK = 0x1A,
	T_COMMONINCL = 0x1B,
	T_INHERITANCE = 0x1C,
	T_INLINEDSUBR = 0x1D,
	T_MODULE = 0x1E,
	T_PTRTOMEMB = 0x1F,
	T_SET = 0x20,
	T_SUBRANGETYPE = 0x21,
	T_WITHSTMT = 0x22,
	T_ACCESSDECL = 0x23,
	T_BASETYPE = 0x24,
	T_CATCHBLK = 0x25,
	T_CONSTTYPE = 0x26,
	T_CONSTANT = 0x27,
	T_ENUMERATOR = 0x28,
	T_FILE = 0x29,
	T_FRIEND = 0x2A,
	T_NAMLIST = 0x2B,
	T_NAMLISTITEM = 0x2C,
	T_PACKED = 0x2D,
	T_SUBPROGRAM = 0x2E,
	T_TEMPLTYPPARAM = 0x2F,
	T_TEMPLVALPARAM = 0x30,
	T_THROWN = 0x31,
	T_TRYBLK = 0x32,
	T_VARIANTPART = 0x33,
	T_VARIABLE = 0x34,
	T_VOLATILE = 0x35,
	T_DWARFPROC = 0x36,
	T_RESTRICT = 0x37,
	T_INTERFACE = 0x38,
	T_NAMESPACE = 0x39,
	T_IMPORTEDMODULE = 0x3A,
	T_UNSPEC = 0x3B,
	T_PARTIALUNIT = 0x3C,
	T_IMPORTEDUNIT = 0x3D,
	T_MUTABLE = 0x3E,
	T_CONDITION = 0x3F,
	T_SHARED = 0x40,
	T_TYPEUNIT = 0x41,
	T_RVALUEREF = 0x42,
	T_LOUSER = 0x4080,
	T_FMTLABEL = 0x4101,
	T_FUNCTEMPL = 0x4102,
	T_CLASSTEMPL = 0x4103,
	T_GNUBINCL = 0x4104,
	T_GNUEINCL = 0x4105,
	T_SUNFUNCTEMPL = 0x4201,
	T_SUNCLASSTEMPL = 0x4202,
	T_SUNSTRUCTTEMPL = 0x4203,
	T_SUNUNIONTEMPL = 0x4204,
	T_SUNINDIRECTINHERIT = 0x4205,
	T_SUNCODEFLAGS = 0x4206,
	T_SUNMEMOPINFO = 0x4207,
	T_SUNOMPCHILDFUNC = 0x4208,
	T_SUNRTTIDESCRIPTOR = 0x4209,
	T_SUNDTORINFO = 0x420A,
	T_SUNDTOR = 0x420B,
	T_SUNF90INTERFACE = 0x420C,
	T_SUNFORTRANVAXSTRUCT = 0x420D,
	T_SUNHI = 0x42FF,
	T_HIUSER = 0xFFFF,

	TypeAddress = 0x01,
	TypeBoolean = 0x02,
	TypeComplexFloat = 0x03,
	TypeFloat = 0x04,
	TypeSigned = 0x05,
	TypeSignedChar = 0x06,
	TypeUnsigned = 0x07,
	TypeUnsignedChar = 0x08,
	TypeImaginaryFloat = 0x09,

	AccessPublic = 0x01,
	AccessProtected = 0x02,
	AccessPrivate = 0x03,

	V_LOCAL = 0x01,
	V_EXPORTED = 0x02,
	V_QUALIFIED = 0x03,

	VirtNone = 0x00,
	VirtVirtual = 0x01,
	VirtPureVirtual = 0x02,

	LangC89 = 0x0001,
	LangC = 0x0002,
	LangAda83 = 0x0003,
	LangCplusplus = 0x0004,
	LangCobol74 = 0x0005,
	LangCobol85 = 0x0006,
	LangFortran77 = 0x0007,
	LangFortran90 = 0x0008,
	LangPascal83 = 0x0009,
	LangModula2 = 0x000A,
	LangJava = 0x000B,
	LangC99 = 0x000C,
	LangAda95 = 0x000D,
	LangFortran95 = 0x000E,
	LangPLI = 0x000F,
	/* 0x8000-0xFFFF reserved */

	IdCaseSensitive = 0x00,
	IdCaseUpper = 0x01,
	IdCaseLower = 0x02,
	IdCaseInsensitive = 0x03,

	CallingNormal = 0x01,
	CallingProgram = 0x02,
	CallingNocall = 0x03,
	/* 0x40-0xFF reserved */

	InNone = 0x00,
	InInlined = 0x01,
	InDeclaredNotInlined = 0x02,
	InDeclaredInlined = 0x03,

	OrderRowMajor = 0x00,
	OrderColumnMajor = 0x01,

	DiscLabel = 0x00,
	DiscRange = 0x01,

	TReference = 1<<0,
	TBlock = 1<<1,
	TConstant = 1<<2,
	TString = 1<<3,
	TFlag = 1<<4,
	TAddress = 1<<5,
	TUndef = 1<<6,

	OpAddr = 0x03,	/* 1 op, const addr */
	OpDeref = 0x06,
	OpConst1u = 0x08,	/* 1 op, 1 byte const */
	OpConst1s = 0x09,	/*	" signed */
	OpConst2u = 0x0A,	/* 1 op, 2 byte const  */
	OpConst2s = 0x0B,	/*	" signed */
	OpConst4u = 0x0C,	/* 1 op, 4 byte const */
	OpConst4s = 0x0D,	/*	" signed */
	OpConst8u = 0x0E,	/* 1 op, 8 byte const */
	OpConst8s = 0x0F,	/*	" signed */
	OpConstu = 0x10,	/* 1 op, LEB128 const */
	OpConsts = 0x11,	/*	" signed */
	OpDup = 0x12,
	OpDrop = 0x13,
	OpOver = 0x14,
	OpPick = 0x15,		/* 1 op, 1 byte stack index */
	OpSwap = 0x16,
	OpRot = 0x17,
	OpXderef = 0x18,
	OpAbs = 0x19,
	OpAnd = 0x1A,
	OpDiv = 0x1B,
	OpMinus = 0x1C,
	OpMod = 0x1D,
	OpMul = 0x1E,
	OpNeg = 0x1F,
	OpNot = 0x20,
	OpOr = 0x21,
	OpPlus = 0x22,
	OpPlusUconst = 0x23,	/* 1 op, ULEB128 addend */
	OpShl = 0x24,
	OpShr = 0x25,
	OpShra = 0x26,
	OpXor = 0x27,
	OpSkip = 0x2F,		/* 1 op, signed 2-byte constant */
	OpBra = 0x28,		/* 1 op, signed 2-byte constant */
	OpEq = 0x29,
	OpGe = 0x2A,
	OpGt = 0x2B,
	OpLe = 0x2C,
	OpLt = 0x2D,
	OpNe = 0x2E,
	OpLit0 = 0x30,
				/* OpLitN = OpLit0 + N for N = 0..31 */
	OpReg0 = 0x50,
				/* OpRegN = OpReg0 + N for N = 0..31 */
	OpBreg0 = 0x70,		/* 1 op, signed LEB128 constant */
				/* OpBregN = OpBreg0 + N for N = 0..31 */
	OpRegx = 0x90,		/* 1 op, ULEB128 register */
	OpFbreg = 0x91,		/* 1 op, SLEB128 offset */
	OpBregx = 0x92,		/* 2 op, ULEB128 reg, SLEB128 off */
	OpPiece = 0x93,		/* 1 op, ULEB128 size of piece */
	OpDerefSize = 0x94,	/* 1-byte size of data retrieved */
	OpXderefSize = 0x95,	/* 1-byte size of data retrieved */
	OpNop = 0x96,
	/* next four new in Dwarf v3 */
	OpPushObjAddr = 0x97,
	OpCall2 = 0x98,		/* 2-byte offset of DIE */
	OpCall4 = 0x99,		/* 4-byte offset of DIE */
	OpCallRef = 0x9A	/* 4- or 8- byte offset of DIE */
	/* 0xE0-0xFF reserved for user-specific */
};

enum {
	A_SIBLING = 0x01,
	A_LOCATION = 0x02,
	A_NAME = 0x03,
	A_ORDERING = 0x09,
	A_BYTESIZE = 0x0B,
	A_BITOFFSET = 0x0C,
	A_BITSIZE = 0x0D,
	A_STMTLIST = 0x10,
	A_LOWPC = 0x11,
	A_HIGHPC = 0x12,
	A_LANGUAGE = 0x13,
	A_DISCR = 0x15,
	A_DISCRVAL = 0x16,
	A_VISIBILITY = 0x17,
	A_IMPORT = 0x18,
	A_STRINGLEN = 0x19,
	A_COMMONREF = 0x1A,
	A_COMPDIR = 0x1B,
	A_CONSTVALUE = 0x1C,
	A_CONTAININGTYPE = 0x1D,
	A_DEFAULTVALUE = 0x1E,
	A_INLINE = 0x20,
	A_ISOPTIONAL = 0x21,
	A_LOWERBOUND = 0x22,
	A_PRODUCER = 0x25,
	A_PROTOTYPED = 0x27,
	A_RETURNADDR = 0x2A,
	A_STARTSCOPE = 0x2C,
	A_STRIDESIZE = 0x2E,
	A_UPPERBOUND = 0x2F,
	A_ABSTRACTORIGIN = 0x31,
	A_ACCESSIBILITY = 0x32,
	A_ADDRCLASS = 0x33,
	A_ARTIFICIAL = 0x34,
	A_BASETYPES = 0x35,
	A_CALLING = 0x36,
	A_COUNT = 0x37,
	A_DATAMEMBERLOC = 0x38,
	A_DECLCOL = 0x39,
	A_DECLFILE = 0x3A,
	A_DECLLINE = 0x3B,
	A_DECLARATION = 0x3C,
	A_DISCRLIST = 0x3D,
	A_ENCODING = 0x3E,
	A_EXTERNAL = 0x3F,
	A_FRAMEBASE = 0x40,
	A_FRIEND = 0x41,
	A_IDENTCASE = 0x42,
	A_MACROINFO = 0x43,
	A_NAMELISTITEM = 0x44,
	A_PRIORITY = 0x45,
	A_SEGMENT = 0x46,
	A_SPEC = 0x47,
	A_STATICLNK = 0x48,
	A_TYPE = 0x49,
	A_USELOC = 0x4A,
	A_VARPARAM = 0x4B,
	A_VIRTUALITY = 0x4C,
	A_VTABLEELEMLOC = 0x4D,
	A_DATALOC = 0x50,
	A_ENTRYPC = 0x52,
	A_RANGES = 0x55,
	A_CALLCOLUMN = 0x57,
	A_CALLFILE = 0x58,
	A_CALLLINE = 0x59,
	A_DECIMALSCALE = 0x5C,
	A_DECIMALSIGN = 0x5E,
	A_EXPLICIT = 0x63,
	A_OBJPTR = 0x64,
	A_ELEMENTAL = 0x66,
	A_DATABITOFF = 0x6B,
	A_CONSTEXPR = 0x6C,
	A_SUNVTABLE = 0x2203,
	A_SUNCOMMANDLINE = 0x2205,
	A_SUNVBASE = 0x2206,
	A_SUNCOMPILEOPTIONS = 0x2207,
	A_SUNLANGUAGE = 0x2208,
	A_SUNVTABLEABI = 0x2210,
	A_SUNFUNCOFFSETS = 0x2211,
	A_SUNCFKIND = 0x2212,
	A_SUNORIGINALNAME = 0x2222,
	A_SUNPARTLINKNAME = 0x2225,
	A_SUNLINKNAME = 0x2226,
	A_SUNPASSWITHCONST = 0x2227,
	A_SUNRETURNWITHCONST = 0x2228,
	A_SUNCOMDATFUNCTION = 0x223C,
	ATTRMAX,

	FormAddr           =  0x01,
	FormDwarfBlock2    =  0x03,
	FormDwarfBlock4    =  0x04,
	FormData2          =  0x05,
	FormData4          =  0x06,
	FormData8          =  0x07,
	FormString         =  0x08,
	FormDwarfBlock     =  0x09,
	FormDwarfBlock1    =  0x0A,
	FormData1          =  0x0B,
	FormFlag           =  0x0C,
	FormSdata          =  0x0D,
	FormStrp           =  0x0E,
	FormUdata          =  0x0F,
	FormRefAddr        =  0x10,
	FormRef1           =  0x11,
	FormRef2           =  0x12,
	FormRef4           =  0x13,
	FormRef8           =  0x14,
	FormRefUdata       =  0x15,
	FormIndirect       =  0x16,
	FormSecOffset      =  0x17,
	FormExprloc        =  0x18,
	FormFlagPresent    =  0x19,
	FormStrx           =  0x1A,
	FormAddrx          =  0x1B,
	FormRefSup         =  0x1C,
	FormStrpSup        =  0x1D,
	FormData16         =  0x1E,
	FormLineStrp       =  0x1F,
	FormRefSig8        =  0x20,
	FormImplicitConst  =  0x21,
	FormLoclistx       =  0x22,
	FormRnglistx       =  0x23,
	FormGNUAddrIndex   =  0x1F01,
	FormGNUStrIndex    =  0x1F02,
	FormGNURefAlt      =  0x1F20,
	FormGNUStrpAlt     =  0x1F21
};

class Place {
	int	_isreg;
public:
		Place(ulong,char*);
	char	*reg;
	long	lng;
	int	isreg(){ return _isreg; }
};

struct DwarfBlock {
	uchar *data;
	ulong len;
};

/* not for consumer use */
struct DwarfBuf {
	uchar *bp;
	uchar *ep;
	uint addrsz;
};

union DwarfVal {
	char *s;
	ulong c;
	ulong r;
	DwarfBlock b;
};

struct DwarfAttrs {
	struct {
		uchar sibling;
		uchar location;
		uchar name;
		uchar ordering;
		uchar bytesize;
		uchar bitoffset;
		uchar bitsize;
		uchar stmtlist;
		uchar lowpc;
		uchar highpc;
		uchar language;
		uchar discr;
		uchar discrval;
		uchar visibility;
		uchar import;
		uchar stringlen;
		uchar commonref;
		uchar compdir;
		uchar constvalue;
		uchar containingtype;
		uchar defaultvalue;
		uchar inlined;
		uchar isoptional;
		uchar lowerbound;
		uchar producer;
		uchar prototyped;
		uchar returnaddr;
		uchar startscope;
		uchar stridesize;
		uchar upperbound;
		uchar abstractorigin;
		uchar accessibility;
		uchar addrclass;
		uchar artificial;
		uchar basetypes;
		uchar calling;
		uchar count;
		uchar datamemberloc;
		uchar declcol;
		uchar declfile;
		uchar declline;
		uchar declaration;
		uchar discrlist;
		uchar encoding;
		uchar external;
		uchar framebase;
		uchar friend_r;
		uchar identcase;
		uchar macroinfo;
		uchar namelistitem;
		uchar priority;
		uchar segment;
		uchar spec;
		uchar staticlnk;
		uchar type;
		uchar uselocat;
		uchar varparam;
		uchar virtuality;
		uchar vtableelemloc;
		uchar dataloc;
		uchar entrypc;
		uchar ranges;
		uchar callcolumn;
		uchar callfile;
		uchar callline;
		uchar decimalscale;
		uchar decimalsign;
		uchar explicit_r;
		uchar objptr;
		uchar elemental;
		uchar databitoff;
		uchar cnstexpr;
		uchar SUNvtable;
		uchar SUNcommandline;
		uchar SUNvbase;
		uchar SUNcompileoptions;
		uchar SUNlanguage;
		uchar SUNvtableabi;
		uchar SUNfuncoffsets;
		uchar SUNcfkind;
		uchar SUNoriginalname;
		uchar SUNpartlinkname;
		uchar SUNlinkname;
		uchar SUNpasswithconst;
		uchar SUNreturnwithconst;
		uchar SUNcomdatfunction;
	} have;
	ulong		sibling;
	DwarfVal	location;
	char		*name;
	ulong		ordering;
	ulong		bytesize;
	ulong		bitoffset;
	ulong		bitsize;
	ulong		stmtlist;
	ulong		lowpc;
	ulong		highpc;
	ulong		language;
	ulong		discr;
	DwarfBlock	discrval;
	ulong		visibility;
	ulong		import;
	DwarfVal	stringlen;
	ulong		commonref;
	char		*compdir;
	DwarfVal	constvalue;
	ulong		containingtype;
	ulong		defaultvalue;
	ulong		inlined;
	uchar		isoptional;
	ulong		lowerbound;
	char		*producer;
	uchar		prototyped;
	DwarfVal	returnaddr;
	ulong		startscope;
	ulong		stridesize;
	ulong		upperbound;
	ulong		abstractorigin;
	ulong		accessibility;
	ulong		addrclass;
	uchar		artificial;
	ulong		basetypes;
	ulong		calling;
	ulong		count;
	DwarfVal	datamemberloc;
	ulong		declcol;
	ulong		declfile;
	ulong		declline;
	uchar		declaration;
	DwarfBlock	discrlist;
	ulong		encoding;
	uchar		external;
	DwarfVal	framebase;
	ulong		friend_r;
	ulong		identcase;
	ulong		macroinfo;
	DwarfBlock	namelistitem;
	ulong		priority;
	DwarfVal	segment;
	ulong		spec;
	DwarfVal	staticlnk;
	ulong		type;
	DwarfVal	uselocat;
	uchar		varparam;
	ulong		virtuality;
	DwarfVal	vtableelemloc;
	ulong		dataloc;
	ulong		entrypc;
	ulong		ranges;
	ulong		callcolumn;
	ulong		callfile;
	ulong		callline;
	ulong		decimalscale;
	ulong		decimalsign;
	uchar		explicit_r;
	ulong		objptr;
	ulong		elemental;
	ulong		databitoff;
	uchar		cnstexpr;
	ulong		SUNvtable;
	char		*SUNcommandline;
	uchar		*SUNvbase;
	char		*SUNcompileoptions;
	ulong		SUNlanguage;
	ulong		SUNvtableabi;
	uchar		*SUNfuncoffsets;
	uchar		*SUNcfkind;
	char		*SUNoriginalname;
	char		*SUNpartlinkname;
	char		*SUNlinkname;
	uchar		SUNpasswithconst;
	uchar		SUNreturnwithconst;
	uchar		SUNcomdatfunction;
};

enum {
	RuleUndef,
	RuleSame,
	RuleCfaOffset,
	RuleRegister,
	RuleRegOff,
	RuleLocation
};

struct DwarfExpr {
	int type;
	long offset;
	ulong reg;
	DwarfBlock loc;
};

struct DwarfSrcs {
	long	ndir;
	char	**dirs;
	long	nfile;
	char	**files;
};

#ifdef NOTDEF
enum {
	DebugAbbrev,
	DebugAranges,
	DebugFrame,
	DebugInfo,
	DebugLine,
	DebugPubnames,
	DebugPubtypes,
	DebugTypenames,
	DebugFuncnames,
	DebugVarnames,
	DebugRanges,
	DebugStrings,
	Nsect
};
#endif

enum {
	D_ABBREV,
	D_ARANGES,
	D_FRAME,
	D_INFO,
	D_LINE,
	D_PUBNAMES,
	D_PUBTYPES,
	D_TYPENAMES,
	D_FUNCNAMES,
	D_VARNAMES,
	D_RANGES,
	D_STRINGS,
	NSECT
};

class DwarfLine;

class Dwarf {
	friend	class DwarfAbbrevs;
	friend	class DwarfSymTab;
	friend	class DwarfLine;
	Elf	*elf;
	char	syserr[ERRMAX];
	int	addrsz;
	DwarfBlock sect[NSECT];
	/* */
	int	getulong(DwarfBuf*, int, ulong, ulong*, int*);
	int	getuchar(DwarfBuf*, int, uchar*);
	int	getfstr(DwarfBuf*, int, char**);
	int	getblock(DwarfBuf*, int, DwarfBlock*);
	int	constblock(DwarfBlock*, ulong*);
	int	skipform(DwarfBuf*, int);
	int	findsect(const char*, ulong*, ulong*);
	int	loadsect(const char*, DwarfBlock*);
	int	nametounit(char*, DwarfBlock*, DwarfRec*);
	int	readblock(DwarfBlock*, ulong, ulong);
	void	skip(DwarfBuf*, int);
	void	errstr(const char *fmt, ...);
public:
		Dwarf();
		~Dwarf();
	DwarfAbbrevs *abbrevs;
	int	start(DwarfRec&);
	int	startunit(DwarfRec&);
	ulong	get1(DwarfBuf*);
	ulong	get2(DwarfBuf*);
	ulong	get4(DwarfBuf*);
	uvlong	get8(DwarfBuf*);
	ulong	get128(DwarfBuf*);
	long	get128s(DwarfBuf*);
	ulong	getaddr(DwarfBuf*);
	int	getn(DwarfBuf*, uchar*, int);
	uchar	*getnref(DwarfBuf*, ulong);
	char	*getstring(DwarfBuf*);
	int	lines();
	Place	*location(DwarfRec&);
	int	nextsym(DwarfRec&);
	int	nextsymat(DwarfRec&, int);
	int	open(Elf*);
	int	parseattrs(DwarfRec&, DwarfAbbrev*);
	MemLayout memlayout();
	char	*error(){ return syserr; }
};
#endif
