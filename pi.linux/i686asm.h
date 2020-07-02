/* <MESA:01:@(#):Mdruhi9uNl0g:bug:1.4:900605160212:druhi:1 35 48428:MESA> */

/* sccsid[] = "%W%\t%G%" */

class I686Asm : public Asm {
public:
const	char    *literaldelimiter();
	Instr   *newInstr(long);
	I686Asm(Core *);
};

class I686Instr : public Instr {
	void		getbyte();
	void		get_opcode(unsigned*,unsigned*);
	void		get_operand(unsigned,unsigned,int,int);
	void 		getbytes(int,char*,long*);
	void		check_override(int);
	void		displacement(int,int,long*);
	void 		imm_data(int,int);
	void		get_modrm_byte(unsigned*,unsigned*,unsigned*);
public:
const	char		*arg(int);
const	char		*mnemonic();
	int		argtype(int);
	int		nargs();
			I686Instr(Asm*, long);
};

#define		WBIT(x)	(x & 0x1)		/* to get w bit	*/
#define		REGNO(x) (x & 0x7)		/* to get 3 bit register */
#define		VBIT(x)	((x)>>1 & 0x1)		/* to get 'v' bit */
#define		OPSIZE(data16,wbit) ((wbit) ? ((data16) ? 2:4) : 1 )
#define		REG_ONLY 3	/* mode indicates a single register with*/
				/* no displacement is an operand	*/
#define		LONGOPERAND 1	/* value of the w-bit indicating a long	*/
				/* operand (2-bytes or 4-bytes)		*/
#define		NCPS	8	/* number of chars per symbol	*/
#define		TRUE	1
#define		FALSE	0
#define		TERM 0		/* indicates indirect' field terminates	*/

struct	instable {
	char		name[NCPS];
	struct instable *indirect;	/* for decode op codes */
	unsigned	adr_mode;
	int		suffix;		/* for instructions which may
					   have a 'w' or 'l' suffix */
};

/*
 *	These are the instruction formats as they appear in
 *	'tables.c'.  Here they are given numerical values
 *	for use in the actual disassembly of an object file.
 */
#define UNKNOWN	0
#define MRw	2
#define IMlw	3
#define IMw	4
#define IR	5
#define OA	6
#define AO	7
#define MS	8
#define SM	9
#define Mv	10
#define Mw	11
#define M	12
#define R	13
#define RA	14
#define SEG	15
#define MR	16
#define IA	17
#define MA	18
#define SD	19
#define AD	20
#define SA	21
#define D	22
#define INM	23
#define SO	24
#define BD	25
#define I	26
#define P	27
#define V	28
#define DSHIFT	29 /* for double shift that has an 8-bit immediate */
#define U	30
#define OVERRIDE 31
#define GO_ON	32
#define	O	33	/* for call	*/
#define JTAB	34	/* jump table 	*/
#define IMUL	35	/* for 186 iimul instr  */
#define CBW 36 /* so that data16 can be evaluated for cbw and its variants */
#define MvI	37	/* for 186 logicals */
#define	ENTER	38	/* for 186 enter instr  */
#define RMw	39	/* for 286 arpl instr */
#define Ib	40	/* for push immediate byte */
#define	F	41	/* for 287 instructions */
#define	FF	42	/* for 287 instructions */
#define DM	43	/* 16-bit data */
#define AM	44	/* 16-bit addr */
#define LSEG	45	/* for 3-bit seg reg encoding */
#define	MIb	46	/* for 686 logicals */
#define	SREG	47	/* for 686 special registers */
#define PREFIX 48 /* an instruction prefix like REP, LOCK */
#define INT3 49   /* The int 3 instruction, which has a fake operand */
#define DSHIFTcl 50 /* for double shift that implicitly uses %cl */
#define CWD 51    /* so that data16 can be evaluated for cwd and variants */
#define RET 52    /* single immediate 16-bit operand */
#define MOVZ 53   /* for movs and movz, with different size operands */

#define	FILL	0x90	/* Fill byte used for alignment (nop)	*/
