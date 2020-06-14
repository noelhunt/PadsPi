/* sccsid[] = "%W%\t%G%" */

enum {
	STT_NOTYPE=0,
	STT_OBJECT,
	STT_FUNC,
	STT_SECTION,
	STT_FILE,
	STT_COMMON,
	STT_TLS,
	STT_NUM,

	STB_GLOBAL=1,
	STB_WEAK
};

class DwarfTShare;

class DwarfSource : public Source {
public:
			DwarfSource(SymTab*,Source*,char*,long);
			~DwarfSource();
	DwarfType	*typeinfo;
};

class DwarfSymTab : public SymTab {
private:
	Elf		*e;
	Dwarf		*dwarf;
	DwarfType	*dwarftype;
	DwarfLine	*dwarfline;
	DwarfRec	r;
	MemLayout	memlayout;
	void		funcbody(DwarfRec*);
	void		gathervar(DwarfRec&, Var**, Block*, UDisc, DwarfType *dt);
	void		dosymtab(Var*, DwarfSource*);
	char		*gethdr();
	Source		*gathertypes(DwarfSource*, Var**, char*);
	Source		*tree();
	DwarfTShare	*share;
public:
			DwarfSymTab(Core*,int,SymTab* =0,long =0);
			~DwarfSymTab();
	Block		*gatherfunc(Func*);
	Var		*gatherutype(UType*);
};

class DwarfUType : public UType {
public:
			DwarfUType(SymTab*,DwarfRec&,char*);
			~DwarfUType();
	DwarfState	save;
	DType		type;		/* aggregate members */
	char		**name;		/* associated names */
	long		*val;		/* associated offsets or values */
};
