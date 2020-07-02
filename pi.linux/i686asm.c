/* <MESA:01:@(#):Mdruhi9uMwBw:bug:1.8:901012121507:druhi:1 35 7411:MESA> */


/*       Copyright (c) 1987,1988,1989,1990 AT&T
 *       All Rights Reserved
 *
 *       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *       The copyright notice above does not evidence any
 *       actual or intended publication of such source code.
 */
#include "univ.h"
#include "asm.h"
#include "i686asm.h"
#include "core.h"
#include "format.h"
SRCFILE("i686asm.c")

static char sccsid[ ] = "%W%\t%G%";

#include "i686table.h"

#define	EBP	6
#define	ESP	7

/* maximum length of a single operand   */
#define         OPLEN   35

static	unsigned short	curbyte;
static	unsigned	mode, r_m;
static	unsigned	opcode2, opcode5;
static	int		got_modrm_byte;
static	struct instable	*dp;
static  char    operand[3][OPLEN];      /* operands as encountered */
static	char	*overreg;    /* save the segment override register if any    */
static	int	data16;      /* 16- or 32-bit data */
static	int	addr16;      /* 16- or 32-bit addressing */

Instr *I686Asm::newInstr(long a) { return (Instr*) new I686Instr(this,a); }
const char *I686Asm::literaldelimiter()	{ return ""; }
I686Asm::I686Asm(Core *c):Asm(c)	{ }
int I686Instr::argtype(int)		{ return 0; }
int I686Instr::nargs()			{ return 1; }
I686Instr::I686Instr(Asm*a,long l):Instr(a,l)	{ display(); }

// The following code is from dis_dot() in dis source.
const char *I686Instr::mnemonic(){
	trace("%d.mnemonic()", this);
	static char mnemonic[OPLEN];
	unsigned opcode1, opcode3, opcode4;

	mnemonic[0] = '\0';
	operand[0][0] = '\0';
	operand[1][0] = '\0';
	operand[2][0] = '\0';
	addr16 = data16 = 0;
	overreg = (char *) 0;
	/*
	 * Asm::display() is too machine dependent. It reads the opcode
	 * in the first byte, and then increments "next" on us.
	 * Turn it back here so we can use it.
	 */
	next--;


	/*
	** As long as there is a prefix, the default segment register,
	** addressing-mode, or data-mode in the instruction will be overridden.
	** This may be more general than the chip actually is.
	*/
	for(;;) {
		get_opcode(&opcode1, &opcode2);
		dp = distable + opcode1 * 16 + opcode2;
		if ( dp->adr_mode == PREFIX )
			strcat(mnemonic,dp->name);
		else if ( dp->adr_mode == AM )
			addr16 = !addr16;
		else if ( dp->adr_mode == DM )
			data16 = !data16;
		else if ( dp->adr_mode == OVERRIDE )
			overreg = dp->name;
		else
			break;
	}

	/* some 686 instructions have 2 bytes of opcode before the mod_r/m */
	/* byte so we need to perform a table indirection.	      */
	if (dp->indirect == (struct instable *) op0F) {
		get_opcode(&opcode4,&opcode5);
		if (opcode4>11)
			return "bad opcode";
		dp = &op0F[opcode4][opcode5];
	}

	got_modrm_byte = 0;
	if (dp->indirect != TERM) {
		/* This must have been an opcode for which several
		 * instructions exist.  The opcode3 field further decodes
		 * the instruction.
		 */
		got_modrm_byte = 1;
		get_modrm_byte(&mode, &opcode3, &r_m);
		/*
		 * decode 287 instructions (D8-DF) from opcodeN
		 */
		if (opcode1 == 0xD && opcode2 >= 0x8) {
			/* instruction form 5 */
			if (opcode2 == 0xB && mode == 0x3 && opcode3 == 4)
				dp = &opFP5[r_m];
			else if (opcode2 == 0xB && mode == 0x3 && opcode3 > 4) 				return "bad opcode";
			/* instruction form 4 */
			else if (opcode2==0x9 && mode==0x3 && opcode3 >= 4)
				dp = &opFP4[opcode3-4][r_m];
			/* instruction form 3 */
			else if (mode == 0x3)
				dp = &opFP3[opcode2-8][opcode3];
			/* instruction form 1 and 2 */
			else
				dp = &opFP1n2[opcode2-8][opcode3];
		}
		else
			dp = dp -> indirect + opcode3;
			/* now dp points the proper subdecode table entry */
	}

	if (dp->indirect != TERM)
		return "bad opcode";
	/* print the mnemonic */
	if ( dp->adr_mode != CBW  && dp->adr_mode != CWD ) {
		(void) strcat(mnemonic,dp->name);  /* print the mnemonic */
		if (dp->suffix)
			(void) strcat(mnemonic, (data16? "w" : "l") );
	}
	return mnemonic;
}

//	get_opcode (high, low)
//	Get the next byte and separate the op code into the high and
//	low nibbles.
void I686Instr::get_opcode(unsigned *high, unsigned *low){
	getbyte();
	*low = curbyte & 0xf;  /* ----xxxx low 4 bits */
	*high = curbyte >> 4 & 0xf;  /* xxxx---- bits 7 to 4 */
}

void I686Instr::getbyte(){
	curbyte = (unsigned short)((unsigned char)
			_asm->core->peekcode(next++)->chr);
}

//	Get the byte following the op code and separate it into the
//	mode, register, and r/m fields.
//	Scale-Index-Bytes have a similar format.
void I686Instr::get_modrm_byte(unsigned *mode, unsigned *reg, unsigned *r_m){
	getbyte();
	*r_m = curbyte & 0x7; /* r/m field from curbyte */
	*reg = curbyte >> 3 & 0x7; /* register field from curbyte */
	*mode = curbyte >> 6 & 0x3; /* mode field from curbyte */
}

// getbytes() reads no_bytes from a file and converts them into destbuf.
// A sign-extended value is placed into destvalue if it is non-null.
void I686Instr::getbytes(int no_bytes, char *destbuf, long *destvalue){
	int i;
	long f;
	unsigned long shiftbuf = 0;

	for (i=0; i<no_bytes; i++) {
		getbyte();
		shiftbuf |= (long) curbyte << (8*i);
	}
	switch(no_bytes) {
		case 1:
			if (destvalue)
				*destvalue = (shiftbuf & 0x80) ?
					shiftbuf | ~0xffL : shiftbuf & 0xffL;
			f = F_MASK8|F_EXT8|fmt;
			break;
		case 2:
			if (destvalue) *destvalue = (short) shiftbuf;
			f = F_MASK16|F_EXT16|fmt;
			break;
		case 4:
			if (destvalue) *destvalue = shiftbuf;
			f = fmt;
			break;
	}
	if (!(f & (F_OCTAL|F_SIGNED|F_HEX)))
		f |= F_HEX;
	m.lng = shiftbuf;
	sprintf(destbuf, literal(f));
}

//	Determine if 1, 2 or 4 bytes of immediate data are needed, then
//	get and print them.
void I686Instr::imm_data(int no_bytes, int opindex){
	int len = strlen(operand[opindex]);
	operand[opindex][len] = '$';
	getbytes(no_bytes, &operand[opindex][len+1], 0);
}

//	Check to see if there is a segment override prefix pending.
//	If so, print it in the current 'operand' location and set
//	the override flag back to false.
void I686Instr::check_override(int opindex){
	if (overreg)
		(void) sprintf(operand[opindex],"%s",overreg);
	overreg = (char *) 0;
}

//	Get and print in the 'operand' array a one, two or four
//	byte displacement from a register.
void I686Instr::displacement(int no_bytes, int opindex, long *value){
	char	temp[(NCPS*2)+1];
	getbytes(no_bytes, temp, value);
	check_override(opindex);
	(void) sprintf(operand[opindex],"%s%s",operand[opindex],temp);
}

void I686Instr::get_operand(unsigned mode, unsigned r_m, int wbit, int opindex){
	int dispsize;		/* size of displacement in bytes */
	int dispvalue;		/* value of the displacement */
	const char *resultreg;	/* representation of index(es) */
	const char *format;	/* output format of result */
	int s_i_b;		/* flag presence of scale-index-byte */
	unsigned ss;		/* scale-factor from opcode */
	unsigned index;		/* index register number */
	unsigned base;		/* base register number */
	char indexbuffer[16];	/* char representation of index(es) */

	/* if symbolic representation, skip override prefix, if any */
	check_override(opindex);

	/* check for the presence of the s-i-b byte */
	if (r_m==ESP && mode!=REG_ONLY && !addr16) {
		s_i_b = TRUE;
		get_modrm_byte(&ss, &index, &base);
	}
	else
		s_i_b = FALSE;

	if (addr16)
		dispsize = dispsize16[r_m][mode];
	else
		dispsize = dispsize32[r_m][mode];

	if (s_i_b && mode==0 && base==EBP) dispsize = 4;

	if (dispsize != 0)
		displacement(dispsize, opindex, (long *)&dispvalue);

	if (s_i_b) {
		const char *basereg = regname32[mode][base];
		(void) sprintf(indexbuffer, "%s%s,%s", basereg,
			indexname[index], scale_factor[ss]);
		resultreg = indexbuffer;
		format = "%s(%s)";
	}
	else { /* no s-i-b */
		if (mode == REG_ONLY) {
			format = "%s%s";
			if (data16)
				resultreg = REG16[r_m][wbit] ;
			else
				resultreg = REG32[r_m][wbit] ;
		}
		else { /* Modes 00, 01, or 10 */
			if (addr16)
				resultreg = regname16[mode][r_m];
			else
				resultreg = regname32[mode][r_m];
			if (r_m ==EBP && mode == 0) { /* displacement only */
				format = "%s";
			}
			else { /* Modes 00, 01, or 10, not displacement only, and no s-i-b */
			if (r_m == 5) { // ebp
				long f = fmt;
				if (dispsize == 1)
					f |= F_MASK8|F_EXT8;
				else if (dispsize == 2)
					f |= F_MASK16|F_EXT16;
				if (!(f & (F_OCTAL|F_SIGNED|F_HEX)))
					f |= F_HEX;
				m.lng = dispvalue;
				reg = r_m;
				(void) sprintf(operand[opindex], "%s", regarg("%s(%%%s)", f));
				return;
			  }
			  format = "%s(%s)";
			}
		}
	}
	(void) sprintf(operand[opindex],format,operand[opindex], resultreg);
}

// Each instruction has a particular instruction syntax  format
// stored in the disassembly tables.  The assignment of formats
// to instructions was made by the author.  Individual  formats
// are  explained  as  they  are  encountered  in the following
// switch construct.

const char *I686Instr::arg(int /*x*/){
	long	lngval;
	int	wbit, vbit;
	char	temp[NCPS+1];
	unsigned	reg;
	const char	*reg_name;
	static char	mneu[OPLEN * 3];

	if (!dp)
		return "686::arg w/o dp";
	switch(dp->adr_mode){
	/* movsbl movsbw (0x0FBE) or movswl (0x0FBF) */
	/* movzbl movzbw (0x0FB6) or mobzwl (0x0FB7) */
	/* wbit lives in 2nd byte, note that operands are different sized */
	case MOVZ:
		/* Get second operand first so data16 can be destroyed */
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];

		wbit = WBIT(opcode5);
		data16 = 1;
		get_operand(mode, r_m, wbit, 0);
		(void) sprintf(mneu,"%s,%s",operand[0],reg_name);
		return mneu;

	/* imul instruction, with either 8-bit or longer immediate */
	case IMUL:
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 1);
		/* opcode 0x6B for byte, sign-extended displacement, 0x69 for word(s)*/
		imm_data( OPSIZE(data16,opcode2 == 0x9), 0);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		(void) sprintf(mneu,"%s,%s,%s",operand[0],operand[1],reg_name);
		return mneu;

	/* memory or register operand to register, with 'w' bit	*/
	case MRw:
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, 0);
		if (data16)
			reg_name = REG16[reg][wbit];
		else
			reg_name = REG32[reg][wbit];
		(void) sprintf(mneu,"%s,%s",operand[0],reg_name);
		return sf("%s",mneu);

	/* register to memory or register operand, with 'w' bit	*/
	/* arpl happens to fit here also because it is odd */
	case RMw:
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, 0);
		if (data16)
			reg_name = REG16[reg][wbit];
		else
			reg_name = REG32[reg][wbit];
		(void) sprintf(mneu,"%s,%s",reg_name,operand[0]);
		return mneu;

	/* Double shift. Has immediate operand specifying the shift. */
	case DSHIFT:
		get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 1);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		imm_data(1, 0);
		sprintf(mneu,"%s,%s,%s",operand[0],reg_name,operand[1]);
		return mneu;

	/* Double shift. With no immediate operand, specifies using %cl. */
	case DSHIFTcl:
		get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		sprintf(mneu,"%s,%s",reg_name,operand[0]);
		return mneu;

	/* immediate to memory or register operand */
	case IMlw:
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 1);
		/* A long immediate is expected for opcode 0x81, not 0x80 nor 0x83 */
		imm_data(OPSIZE(data16,opcode2 == 1), 0);
		sprintf(mneu,"%s,%s",operand[0],operand[1]);
		return mneu;
	default:
		break;
	}

/* broke up switch PFM */

	switch(dp -> adr_mode){

	/* immediate to memory or register operand with the	*/
	/* 'w' bit present					*/
	case IMw:
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, 1);
		imm_data(OPSIZE(data16,wbit), 0);
		sprintf(mneu,"%s,%s",operand[0],operand[1]);
		return mneu;

	/* immediate to register with register in low 3 bits	*/
	/* of op code						*/
	case IR:
		wbit = opcode2 >>3 & 0x1; /* w-bit here (with regs) is bit 3 */
		reg = REGNO(opcode2);
		imm_data( OPSIZE(data16,wbit), 0);
		if (data16)
			reg_name = REG16[reg][wbit];
		else
			reg_name = REG32[reg][wbit];
		(void) sprintf(mneu,"%s,%s",operand[0],reg_name);
		return mneu;

	/* memory operand to accumulator			*/
	case OA:
		wbit = WBIT(opcode2);
		displacement(OPSIZE(addr16,LONGOPERAND), 0,&lngval);
		reg_name = ( data16 ? REG16 : REG32 )[0][wbit];
		(void) sprintf(mneu,"%s,%s",operand[0],reg_name);
		return mneu;

	/* accumulator to memory operand			*/
	case AO:
		wbit = WBIT(opcode2);
		
		displacement(OPSIZE(addr16,LONGOPERAND), 0,&lngval);
		reg_name = ( addr16 ? REG16 : REG32 )[0][wbit];
		(void) sprintf(mneu,"%s,%s",reg_name, operand[0]);
		return mneu;

	/* memory or register operand to segment register	*/
	case MS:
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0);
		(void) sprintf(mneu,"%s,%s",operand[0],SEGREG[reg]);
		return mneu;

	/* segment register to memory or register operand	*/
	case SM:
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0);
		(void) sprintf(mneu,"%s,%s",SEGREG[reg],operand[0]);
		return mneu;

	/* rotate or shift instrutions, which may shift by 1 or */
	/* consult the cl register, depending on the 'v' bit	*/
	case Mv:
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0);
		/* When vbit is set, register is an operand, otherwise just $0x1 */
		reg_name = vbit ? "%cl," : "" ;
		(void) sprintf(mneu,"%s%s",reg_name, operand[0]);
		return mneu;

	/* immediate rotate or shift instrutions, which may or */
	/* may not consult the cl register, depending on the 'v' bit	*/
	case MvI:
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0);
		imm_data(1,1);
		/* When vbit is set, register is an operand, otherwise just $0x1 */
		reg_name = vbit ? "%cl," : "" ;
		(void) sprintf(mneu,"%s,%s%s",operand[1], reg_name, operand[0]);
		return mneu;

	case MIb:
		get_operand(mode, r_m, LONGOPERAND, 0);
		imm_data(1,1);
		(void) sprintf(mneu,"%s,%s",operand[1], operand[0]);
		return mneu;

	/* single memory or register operand with 'w' bit present*/
	case Mw:
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0);
		(void) sprintf(mneu,"%s",operand[0]);
		return mneu;

	/* single memory or register operand			*/
	case M:
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0);
		(void) sprintf(mneu,"%s",operand[0]);
		return mneu;

	case SREG: /* special register */
		get_modrm_byte(&mode, &reg, &r_m);
		vbit = 0;
		switch (opcode5) {
		case 2:
			vbit = 1;
			/* fall thru */
		case 0: 
			reg_name = CONTROLREG[reg];
			break;
		case 3:
			vbit = 1;
			/* fall thru */
		case 1:
			reg_name = DEBUGREG[reg];
			break;
		case 6:
			vbit = 1;
			/* fall thru */
		case 4:
			reg_name = TESTREG[reg];
			break;
		}
		strcpy(operand[0], REG32[r_m][1]);

		if (vbit)
		{
			strcpy(operand[0], reg_name);
			reg_name = REG32[r_m][1];
		}
		
		(void) sprintf(mneu, "%s,%s", reg_name, operand[0]);
		return mneu;

	/* single register operand with register in the low 3	*/
	/* bits of op code					*/
	case R:
		reg = REGNO(opcode2);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		(void) sprintf(mneu,"%s",reg_name);
		return mneu;

	/* register to accumulator with register in the low 3	*/
	/* bits of op code, xchg instructions                   */
	case RA: {
		const char *eprefix;
		reg = REGNO(opcode2);
		if (data16) {
			eprefix = "";
			reg_name = REG16[reg][LONGOPERAND];
		}
		else {
			eprefix = "e";
			reg_name = REG32[reg][LONGOPERAND];
		}
		(void) sprintf(mneu,"%s,%%%sax",reg_name,eprefix);
		return mneu;
	}

	/* single segment register operand, with register in	*/
	/* bits 3-4 of op code					*/
	case SEG:
		reg = curbyte >> 3 & 0x3; /* segment register */
		(void) sprintf(mneu,"%s",SEGREG[reg]);
		return mneu;

	/* single segment register operand, with register in	*/
	/* bits 3-5 of op code					*/
	case LSEG:
		reg = curbyte >> 3 & 0x7; /* long seg reg from opcode */
		(void) sprintf(mneu,"%s",SEGREG[reg]);
		return mneu;

	/* memory or register operand to register		*/
	case MR:
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		(void) sprintf(mneu,"%s,%s",operand[0],reg_name);
		return mneu;

	default:
		break;

	}
/* break up switch statement so that the 4.1.6 CPLU can generate the code -g
   PFM 10-6-90 */
	switch(dp -> adr_mode){

	/* immediate operand to accumulator			*/
	case IA: {
		int no_bytes = OPSIZE(data16,WBIT(opcode2));
		switch(no_bytes) {
			case 1: reg_name = "%al"; break;
			case 2: reg_name = "%ax"; break;
			case 4: reg_name = "%eax"; break;
		}
		imm_data(no_bytes, 0);
		(void) sprintf(mneu,"%s,%s",operand[0], reg_name) ;
		return mneu;
	}
	/* memory or register operand to accumulator		*/
	case MA:
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0);
		reg_name = ( data16 ? REG16 : REG32) [0][wbit];
		(void) sprintf(mneu,"%s,%s", operand[0], reg_name );
		return mneu;

	/* si register to di register				*/
	case SD:
		check_override(0);
		(void) sprintf(mneu,"%s(%%%ssi),(%%%sdi)",operand[0],
			addr16? "" : "e" , addr16? "" : "e");
		return mneu;

	/* accumulator to di register				*/
	case AD:
		wbit = WBIT(opcode2);
		check_override(0);
		reg_name = (data16 ? REG16 : REG32) [0][wbit] ;
		(void) sprintf(mneu,"%s,%s(%%%sdi)", reg_name, operand[0],
			addr16? "" : "e");
		return mneu;

	/* si register to accumulator				*/
	case SA:
		wbit = WBIT(opcode2);
		check_override(0);
		reg_name = (addr16 ? REG16 : REG32) [0][wbit] ;
		(void) sprintf(mneu,"%s(%%%ssi),%s",operand[0],
			addr16? "" : "e", reg_name);
		return mneu;

	/* single operand, a 16/32 bit displacement		*/
	/* added to current offset by 'compoff'			*/
	case D:
		displacement(OPSIZE(data16,LONGOPERAND), 0, &lngval);
		m.lng = lngval + next;
		return symbolic();

	/* indirect to memory or register operand		*/
	case INM:
		get_operand(mode, r_m, LONGOPERAND, 0);
		(void) sprintf(mneu,"*%s",operand[0]);
		return mneu;

	/* for long jumps and long calls -- a new code segment   */
	/* register and an offset in IP -- stored in object      */
	/* code in reverse order                                 */
	case SO:
		displacement(OPSIZE(addr16,LONGOPERAND), 1,&lngval);
		/* will now get segment operand*/
		displacement(2, 0,&lngval);
		(void) sprintf(mneu,"%s,%s",operand[0],operand[1]);
		return mneu;

	/* jmp/call. single operand, 8 bit displacement.	*/
	/* added to current EIP in 'compoff'			*/
	case BD:
		displacement(1, 0, &lngval);
		m.lng = lngval + next;
		return symbolic();

	/* single 32/16 bit immediate operand			*/
	case I:
		imm_data(OPSIZE(data16,LONGOPERAND), 0);
		(void) sprintf(mneu,"%s",operand[0]);
		return mneu;

	/* single 8 bit immediate operand			*/
	case Ib:
		imm_data(1, 0);
		(void) sprintf(mneu,"%s", operand[0]);
		return mneu;

	case ENTER:
		imm_data(2,0);
		imm_data(1,1);
		(void) sprintf(mneu,"%s,%s",operand[0],operand[1]);
		return mneu;
	default:
		break;
	}

/* broke up switch PFM */

	switch(dp -> adr_mode){

	/* 16-bit immediate operand */
	case RET:
		imm_data(2,0);
		(void) sprintf(mneu,"%s",operand[0]);
		return mneu;

	/* single 8 bit port operand				*/
	case P:
		check_override(0);
		imm_data(1, 0);
		(void) sprintf(mneu,"%s",operand[0]);
		return mneu;

	/* single operand, dx register (variable port instruction)*/
	case V:
		check_override(0);
		(void) sprintf(mneu,"%s(%%dx)",operand[0]);
		return mneu;

	/* The int instruction, which has two forms: int 3 (breakpoint) or  */
	/* int n, where n is indicated in the subsequent byte (format Ib).  */
	/* The int 3 instruction (opcode 0xCC), where, although the 3 looks */
	/* like an operand, it is implied by the opcode. It must be converted */
	/* to the correct base and output. */
	case INT3:
		return "$3";

	/* an unused byte must be discarded			*/
	case U:
		getbyte();
		return "";

	case CBW:
		if (data16)
			 return "cbtw";
		else
			 return "cwtl";

	case CWD:
		if (data16)
			 return "cwtd";
		else
			 return "cltd";

	/* no disassembly, the mnemonic was all there was	*/
	/* so go on						*/
	case GO_ON:
		return "";

	/* Special byte indicating a the beginning of a 	*/
	/* jump table has been seen. The jump table addresses	*/
	/* will be printed until the address 0xffff which	*/
	/* indicates the end of the jump table is read.		*/
	case JTAB:
		return "***JUMP TABLE BEGINNING***";

	/* float reg */
	case F:
		(void) sprintf(mneu,"%%st(%1.1d)", r_m);
		return mneu;

	/* float reg to float reg, with ret bit present */
	case FF:
		if ( opcode2 >> 2 & 0x1 ) {
			/* return result bit for 287 instructions	*/
			/* st -> st(i) */
			(void) sprintf(mneu,"%%st,%%st(%1.1d)",r_m);
		}
		else {
			/* st(i) -> st */
			(void) sprintf(mneu,"%%st(%1.1d),%%st",r_m);
		}
		return mneu;

	/* an invalid op code */
	case AM:
	case DM:
	case OVERRIDE:
	case PREFIX:
	case UNKNOWN:
		return "Error - bad opcode";

	default:
		return "Error - dis case not found";

	} /* end switch */
}
