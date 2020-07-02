#ifndef ELF_H
#define ELF_H
/*
 * Copyright (c) 2004 Russ Cox.  See LICENSE.
 *
 *    sccsid[] = "%W%\t%G%"
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "format.h"

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef long long		vlong;
typedef unsigned long long	uvlong;

#ifdef ERRMAX
# undef ERRMAX
#endif
#define	ERRMAX	1024

#define ELFNOTE_NAME(n) ((unsigned char*)(n) + sizeof(*(n)))
#define ELFNOTE_DESC(n) (ELFNOTE_NAME(n) + (((n)->namesz+3)&~3))
#define ELFNOTE_NEXT(n) (ELFNOTE_DESC(n) + (((n)->descsz+3)&~3))

typedef struct ElfEhdr ElfEhdr;
typedef struct ElfSect ElfSect;
typedef struct ElfProg ElfProg;
typedef struct ElfNhdr ElfNhdr;
typedef struct ElfNote ElfNote;
typedef struct ElfSym ElfSym;

enum {
	ElfClassNone = 0,
	ElfClass32,
	ElfClass64,

	ElfDataNone = 0,
	ElfDataLsb,
	ElfDataMsb,

	ElfTypeNone = 0,
	ElfTypeRelocatable,
	ElfTypeExecutable,
	ElfTypeSharedObject,
	ElfTypeCore,
	/* 0xFF00 - 0xFFFF reserved for processor-specific types */

	ElfMachNone = 0,
	ElfMach32100,		/* AT&T WE 32100 */
	ElfMachSparc,		/* SPARC */
	ElfMach386,		/* Intel 80386 */
	ElfMach68000,		/* Motorola 68000 */
	ElfMach88000,		/* Motorola 88000 */
	ElfMach486,		/* Intel 80486, no longer used */
	ElfMach860,		/* Intel 80860 */
	ElfMachMips,		/* MIPS RS3000 */
	ElfMachS370,		/* IBM System/370 */
	ElfMachMipsLe,	/* MIPS RS3000 LE */
	ElfMachParisc = 15,		/* HP PA RISC */
	ElfMachVpp500 = 17,	/* Fujitsu VPP500 */
	ElfMachSparc32Plus,	/* SPARC V8+ */
	ElfMach960,		/* Intel 80960 */
	ElfMachPower,		/* PowerPC */
	ElfMachPower64,	/* PowerPC 64 */
	ElfMachS390,		/* IBM System/390 */
	ElfMachV800 = 36,	/* NEC V800 */
	ElfMachFr20,		/* Fujitsu FR20 */
	ElfMachRh32,		/* TRW RH-32 */
	ElfMachRce,		/* Motorola RCE */
	ElfMachArm,		/* ARM */
	ElfMachAlpha,		/* Digital Alpha */
	ElfMachSH,		/* Hitachi SH */
	ElfMachSparc9,		/* SPARC V9 */
	ElfMachAmd64 = 62,	/* x86-64 */
	/* and the list goes on... */

	ElfAbiNone = 0,
	ElfAbiSystemV = 0,	/* [sic] */
	ElfAbiHPUX,
	ElfAbiNetBSD,
	ElfAbiLinux,
	ElfAbiSolaris = 6,
	ElfAbiAix,
	ElfAbiIrix,
	ElfAbiFreeBSD,
	ElfAbiTru64,
	ElfAbiModesto,
	ElfAbiOpenBSD,
	ElfAbiARM = 97,
	ElfAbiEmbedded = 255,

	/* some of sections 0xFF00 - 0xFFFF reserved for various things */
	ElfSectNone = 0,
	ElfSectProgbits,
	ElfSectSymtab,
	ElfSectStrtab,
	ElfSectRela,
	ElfSectHash,
	ElfSectDynamic,
	ElfSectNote,
	ElfSectNobits,
	ElfSectRel,
	ElfSectShlib,
	ElfSectDynsym,

	ElfSectFlagWrite = 0x1,
	ElfSectFlagAlloc = 0x2,
	ElfSectFlagExec = 0x4,
	/* 0xF0000000 are reserved for processor specific */

	ElfSymBindLocal = 0,
	ElfSymBindGlobal,
	ElfSymBindWeak,
	/* 13-15 reserved */

	STBLocal = 0,
	STBGlobal,
	STBWeak,
	/* 13-15 reserved */

	ElfSymTypeNone = 0,
	ElfSymTypeObject,
	ElfSymTypeFunc,
	ElfSymTypeSection,
	ElfSymTypeFile,
	/* 13-15 reserved */

	STTNone = 0,
	STTObject,
	STTFunc,
	STTSection,
	STTFile,
	/* 13-15 reserved */

	ElfSymShnNone = 0,
	ElfSymShnAbs = 0xFFF1,
	ElfSymShnCommon = 0xFFF2,
	/* 0xFF00-0xFF1F reserved for processors */
	/* 0xFF20-0xFF3F reserved for operating systems */

	ElfProgNone = 0,
	ElfProgLoad,
	ElfProgDynamic,
	ElfProgInterp,
	ElfProgNote,
	ElfProgShlib,
	ElfProgPhdr,
	ElfProgTls,

	PTNULL = 0,
	PTLOAD,
	PTDYNAMIC,
	PTINTERP,
	PTNOTE,
	PTSHLIB,
	PTPHDR,
	PTTLS,

	ElfProgFlagExec = 0x1,
	ElfProgFlagWrite = 0x2,
	ElfProgFlagRead = 0x4,

	PFEXEC = 0x1,
	PFWRITE = 0x2,
	PFREAD = 0x4,

	/*
	 *  Known values for note entry types (e_type == ElfTypeCore)
	 */

	NotePrstatus=1,		/* obsolete                             */
	NotePrfpreg=2,		/* prfpregset_t <sys/procfs_isa.h>      */
	NotePrpsinfo=3,		/* obsolete                             */
	NotePrxreg=4,		/* prxregset_t  <sys/procfs.h>          */
	NotePlatform=5,		/* string from sysinfo(SI_PLATFORM)     */
	NoteAuxv=6,		/* auxv_t array <sys/auxv.h>            */
	NoteGwindows=7,		/* gwindows_t   SPARC only              */
	NoteAsrs=8,		/* asrset_t     SPARC V9 only           */
	NoteLdt=9,		/* ssd array    <sys/sysi86.h> IA32 only */
	NotePstatus=10,		/* pstatus_t    <sys/procfs.h>          */
	NotePsinfo=13,		/* psinfo_t     <sys/procfs.h>          */
	NotePrcred=14,		/* prcred_t     <sys/procfs.h>          */
	NoteUtsname=15,		/* struct utsname <sys/utsname.h>       */
	NoteLwpstatus=16,	/* lwpstatus_t  <sys/procfs.h>          */
	NoteLwpsinfo=17,	/* lwpsinfo_t   <sys/procfs.h>          */
	NotePrpriv=18,		/* prpriv_t     <sys/procfs.h>          */
	NotePrprivinfo=19,	/* priv_impl_info_t <sys/priv.h>        */
	NoteContent=20,		/* core_content_t <sys/corectl.h>       */
	NoteZonename=21,	/* string from getzonenamebyid(3C)      */
	NotePrcpuxreg=22,	/* prcpuxregset_t <sys/procfs.h>        */
	NoteSigaction=23,	/* struct sigaction array               */
	NoteFdinfo=24,		/* struct prfdinfo array                */
	NoteNum=24,
};

struct ElfEhdr {
	uchar	magic[4];
	uchar	_class;
	uchar	encoding;
	uchar	version;
	uchar	abi;
	uchar	abiversion;
	uint	type;
	uint	machine;
	uvlong	entry;
	uvlong	phoff;
	uvlong	shoff;
	uint	flags;
	uint	ehsize;
	uint	phentsize;
	uint	phnum;
	uint	shentsize;
	uint	shnum;
	uint	shstrndx;
};

struct ElfSect {
	char	*name;
	uint	type;
	uvlong	flags;
	uvlong	addr;
	uvlong	offset;
	uvlong	size;
	uint	link;
	uint	info;
	uvlong	align;
	uvlong	entsize;
	uchar	*base;
};

struct ElfProg {
	uint	type;
	uvlong	offset;
	uvlong	vaddr;
	uvlong	paddr;
	uvlong	filesz;
	uvlong	memsz;
	uint	flags;
	uvlong	align;
	uchar	*base;
};

struct ElfNhdr {
	uint	namesz;
	uint	descsz;
	uint	type;
};

struct ElfNote {
	uint	namesz;
	uint	descsz;
	uint	type;
	uchar	*name;
	uchar	*desc;
	uint	offset;	/* in-memory only */
};

struct ElfSym {
	char*	name;
	uvlong	value;
	uvlong	size;
	uchar	bind;
	uchar	type;
	uchar	other;
	ushort	shndx;
};

class Elf {
	friend	class Dwarf;
	friend	class SymTab;
	friend	class DwarfSymTab;
	friend	class Hostfunc;
	int	fd;
	ElfEhdr	ehdr;
	uchar	magic[4];
	ElfSect	*symtab;
	ElfSect	*symstr;
	ElfSect	*dynsym;
	ElfSect	*dynstr;
	ElfSect	*bss;
	ulong	dynamic;
	Bls	error;
#ifdef MMAP
struct	stat	st;
	void	*maddr;
#endif
	void	unpackehdr(void*);
	void	unpackphdr(int, void*);
	void	unpackshdr(int, void*);
	int	init();
	int	map(ElfSect*);
const	char	*namety(int);
static	ushort	leload2(uchar*);
static	uint	leload4(uchar*);
static	uvlong	leload8(uchar*);
static	ushort	beload2(uchar*);
static	uint	beload4(uchar*);
static	uvlong	beload8(uchar*);
	void	errstr(const char *fmt, ...);
	char	errbuf[ERRMAX];
public:
		~Elf();
		Elf();
	ElfSect	*sect;
	uint	nsect;
	ElfProg	*prog;
	uint	nprog;
	int	nsymtab;
	int	ndynsym;
	ushort	e2(uchar*);
	uint	e4(uchar*);
	uvlong	e8(uchar*);
	int	open(const char*, int =0);	// O_RDONLY
const	char	*fdopen(int);
	ElfSect *findsect(const char*);
	ElfProg *findprog(int);
	int	map(ElfProg*);
	int	nseg(int);
	int	sym(int, ElfSym*);
	MemLayout memlayout();
	uint	machine()	{ return ehdr.machine;	}
	uchar	encoding()	{ return ehdr.encoding;	}
	void	close()		{ ::close(fd);		}
	char	*perror()	{ return errbuf;	}
};
#endif
