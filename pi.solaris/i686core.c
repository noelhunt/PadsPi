#include "process.h"
#include "frame.h"
#include "symtab.h"
#include "symbol.h"
#include "core.h"
#include "asm.h"
#include "i686asm.h"
#include "i686core.h"
#include "dwarftype.h"
#include "dwarfsymtab.h"
SRCFILE("i686core.c")

static char sccsid[ ] = "%W%\t%G%";

const uchar
	JMP_REL8=0xEB,		// jmp
	JMP_REL32=0xE9,		// jmp
	PUSHL_EAX=0x50,		// pushl %eax
	PUSHL_EBX=0x53,		// pushl %ebx
	PUSHL_EBP=0x55,		// pushl %ebp
	PUSHL_ESI=0x56,		// pushl %esi
	PUSHL_EDI=0x57,		// pushl %edi
	POPL_EAX=0x58,		// popl %eax
	CALL=0xE8,		// call
	RETURN=0xC3;		// ret
static uchar subl_imm8_esp[2]	= { 0x83, 0xEC };
static uchar subl_imm32_esp[2]	= { 0x81, 0xEC };
static uchar xchgl_eax_0esp[4]	= { 0x87, 0x44, 0x24, 0x0 };
static uchar movl_esp_ebp[2]	= { 0x8B, 0xEC };
static uchar call_eax[2]	= { 0xFF, 0xD0 };

int I686Core::REG_AP()			{ return 6; }	// EBP
int I686Core::REG_FP()			{ return 6; }
int I686Core::REG_SP()			{ return 17; }	// UESP
int I686Core::REG_PC()			{ return 14; }	// EIP
int I686Core::nregs()			{ return 19; }
int I686Core::atreturn(long pc)		{ return (peek(pc)->chr&0xFF)==RETURN;}
long I686Core::returnregloc()		{ return regloc(0); }
long I686Core::callingpc(long fp)	{ return peek(fp+4)->lng; }
long I686Core::callingfp(long fp)	{ return peek(fp)->lng; }
int I686Core::argstart()		{ return 8; }
Asm *I686Core::newAsm()			{ return new I686Asm(this); }

void I686Core::newSymTab(long reloc){
	_symtab = new DwarfSymTab(this, stabfd, _symtab, reloc);
}

I686Core::I686Core(){
	stackdir = GROWDOWN;
	memlayout = LSBFIRST;
	bptsize = 1;
}

char *I686Core::regname(int r){
	static char *regnames[] = {
		"gs", "fs", "es", "ds", "edi", "esi", "ebp", "esp",
		"ebx", "edx", "ecx", "eax", "trapno", "error", "eip",
		"cs", "eflags", "uesp", "ss",
	};

	if (r < nregs())
		return regnames[r];
	else
		return 0;
}

CallStk *I686Core::callstack(){
	long size;
	long fpcache[1000];
	long *fpp = fpcache;
	*fpp = fp();
	if( !fpvalid(*fpp))
		return (CallStk *)0;
	for( size = 1; size<1000; ++size ){
		long fpnext = callingfp(*fpp);
		if( !instack(fpnext, *fpp) )
			break;
		*++fpp = fpnext;
	}
	size--;		// off by one, throw away top
	if (!size)
		return (CallStk *)0;
	CallStk *c = new CallStk(size, this);
	long _pc = pc();
	long i = 0;
	for( fpp = fpcache;; fpp++ ){
		c->fpf[i].fp = *fpp;
		c->fpf[i].func = (Func*) _symtab->loctosym(U_FUNC, _pc);
		if (++i == size)
			break;
		_pc = callingpc(*fpp);
	}
	return c;
}

#define REGBIT(r) (1 << r)
long I686Core::saved(Frame *f, int r, int){	/* ignore size */
	int reg;
	switch(r) {
		case 3: r = 0; break;	/* ebx */
		case 6: r = 1; break;	/* esi */
		case 7: r = 2; break;	/* edi */
		default:
			return 0;
	}
	if (!(f->regsave & REGBIT(r)))
		return 0;
	long loc = f->regbase;
	for (reg = 0; reg < 3; reg++) {
		if (reg == r)
			return loc;
		if (f->regsave & REGBIT(reg))
			loc += 4;
	}
	/* Not reached */
	return 0;
}

Frame I686Core::frameabove(long _fp){
	Frame f(this);
	if( _fp ){
		f.pc = callingpc(_fp);
		f.fp = callingfp(_fp);
	} else {
		f.pc = pc();
		f.fp = fp();
	}
	f.ap = f.fp;
	f.regbase = f.fp;
	Func *funcp = (Func*)_symtab->loctosym(U_FUNC, f.pc);
	if (!funcp)
		return f;
	if (funcp->regsave == -1) {
	// compute the mask and offset once and cache it in funcp->regsave
	long a = funcp->range.lo;
	long mask = 0;
	long offset = 0;
	unsigned char instr[20];

	read(a, (char*)instr, sizeof(instr));
	if (instr[0] == JMP_REL8) {
		a += instr[1] + 2;
		read(a, (char*)instr, sizeof(instr));
	} else if (instr[0] == JMP_REL32) {
		a += (instr[1] | (instr[2] << 8) | (instr[3] << 16) |
		     (instr[4] << 24)) + 5;
		read(a, (char*)instr, sizeof(instr));
	}
	int i = 0;
	if (instr[0] == POPL_EAX &&
	    instr[1] == xchgl_eax_0esp[0] && instr[2] == xchgl_eax_0esp[1] &&
	    instr[3] == xchgl_eax_0esp[2] && instr[4] == xchgl_eax_0esp[3])
		i += 5;
	if (instr[i] == PUSHL_EBP &&
	    instr[i+1] == movl_esp_ebp[0] && instr[i+2] == movl_esp_ebp[1]) {
		i += 3;
		if (instr[i] == PUSHL_EAX) {
			offset = 4;
			i++;
		} else if (instr[i] == subl_imm8_esp[0] &&
			   instr[i+1] == subl_imm8_esp[1]) {
			offset = instr[i+2];
			i += 3;
		} else if (instr[i] == subl_imm32_esp[0] &&
			   instr[i+1] == subl_imm32_esp[1]) {
			i += 2;
			offset = instr[i] | (instr[i+1] << 8) |
				 (instr[i+2] << 16) | (instr[i+3] << 24);
			i += 4;
		}
		if (instr[i] == PUSHL_EDI) {
			mask |= REGBIT(2);
			offset += 4;
			i++;
		}
		if (instr[i] == PUSHL_ESI) {
			mask |= REGBIT(1);
			offset += 4;
			i++;
		}
		if (instr[i] == PUSHL_EBX) {
			mask |= REGBIT(0);
			offset += 4;
		}
		funcp->regsave = (offset << 4) | mask;
	 }
	 else
		funcp->regsave = 0;
	}
	f.regbase -= (funcp->regsave >> 4) & 0x0FFFFFFF;
	f.regsave = funcp->regsave & 0xF;
	return f;
}

long I686Core::instrafterjsr(){
	dostep(0,0,1);
	return peek(sp())->lng;
}

int I686Core::atjsr(long pc){
	unsigned char instr[2];

	if (read(pc, (char *)instr, 2))
		return 0;
	if (instr[0] == CALL ||
	    (instr[0] == call_eax[0] && instr[1] == call_eax[1]) )
		return 1;
	// Add other cases when needed
	return 0;
}

char *I686Core::stepprolog(){
	unsigned char instr[20];
	int jumpped = 0;
	int i = 0;
	long a;

	read(a = pc(), (char*)instr, sizeof(instr));
	if (instr[0] == JMP_REL8  || instr[0] == JMP_REL32) {
		jumpped++;
		step();
		read(a = pc(), (char*)instr, sizeof(instr));
	}
	if (instr[0] == POPL_EAX &&
	    instr[1] == xchgl_eax_0esp[0] && instr[2] == xchgl_eax_0esp[1] &&
	    instr[3] == xchgl_eax_0esp[2] && instr[4] == xchgl_eax_0esp[3])
		i += 5;
	if (instr[i] == PUSHL_EBP &&
	    instr[i+1] == movl_esp_ebp[0] && instr[i+2] == movl_esp_ebp[1]) {
		i += 3;
		if (instr[i] == PUSHL_EAX)
			i++;
		else if (instr[i] == subl_imm8_esp[0] &&
			   instr[i+1] == subl_imm8_esp[1])
			i += 3;
		else if (instr[i] == subl_imm32_esp[0] &&
			   instr[i+1] == subl_imm32_esp[1])
			i += 6;
		if (instr[i] == PUSHL_EDI)
			i++;
		if (instr[i] == PUSHL_ESI)
			i++;
		if (instr[i] == PUSHL_EBX)
			i++;
	}
	if (jumpped) {
		if (instr[i] == JMP_REL8)
			i += 2;
		else if (instr[i] == JMP_REL32)
			i += 5;
	}
	if (i)
		return dostep(a, a+i, 1);
	return 0;
}

char *I686Core::docall(long addr, int){
	const int CALL_SIZE=5;
	unsigned char save[CALL_SIZE], code[CALL_SIZE];
	char *error;

	if( behavetype() == ACTIVE )
		return "process not stopped";
	long callstart = scratchaddr();
	long offset = addr - callstart - CALL_SIZE;
	code[0] = CALL;
	code[1] = (unsigned char)(offset & 0xFF);
	code[2] = (unsigned char)((offset >> 8) & 0xFF);
	code[3] = (unsigned char)((offset >> 16) & 0xFF);
	code[4] = (unsigned char)((offset >> 24) & 0xFF);
	if ((error = read(callstart, (char*)save, CALL_SIZE))
	 || (error = write(callstart, (char*)code, CALL_SIZE)))
		return error;
	if ((error = regpoke(REG_PC(), callstart))
	 || (error = step(callstart, callstart+CALL_SIZE)))
		write(callstart, (char*)save, CALL_SIZE);
	else
		error = write(callstart, (char*)save, CALL_SIZE);
	return error;
}

long I686Core::apforcall(int argbytes){
	regpoke(REG_SP(), sp() - argbytes);
	return sp() - 8;
}
