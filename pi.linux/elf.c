/*
 * Parse 32-bit ELF files.
 * Copyright (c) 2004 Russ Cox.  See LICENSE.
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "elf.h"
#include <stdint.h>
SRCFILE("elf.c")

static char sccsid[ ] = "%W%\t%G%";

typedef struct Elf32HdrBytes	Elf32HdrBytes;
typedef struct Elf32SectBytes	Elf32SectBytes;
typedef struct Elf32ProgBytes	Elf32ProgBytes;
typedef struct Elf32SymBytes	Elf32SymBytes;

typedef struct Elf64HdrBytes	Elf64HdrBytes;
typedef struct Elf64SectBytes	Elf64SectBytes;
typedef struct Elf64ProgBytes	Elf64ProgBytes;
typedef struct Elf64SymBytes	Elf64SymBytes;

struct Elf32HdrBytes {
	uchar	ident[16];
	uchar	type[2];
	uchar	machine[2];
	uchar	version[4];
	uchar	entry[4];
	uchar	phoff[4];
	uchar	shoff[4];
	uchar	flags[4];
	uchar	ehsize[2];
	uchar	phentsize[2];
	uchar	phnum[2];
	uchar	shentsize[2];
	uchar	shnum[2];
	uchar	shstrndx[2];
};

struct Elf64HdrBytes {
	uchar	ident[16];
	uchar	type[2];
	uchar	machine[2];
	uchar	version[4];
	uchar	entry[8];
	uchar	phoff[8];
	uchar	shoff[8];
	uchar	flags[4];
	uchar	ehsize[2];
	uchar	phentsize[2];
	uchar	phnum[2];
	uchar	shentsize[2];
	uchar	shnum[2];
	uchar	shstrndx[2];
};

struct Elf32SectBytes {
	uchar	name[4];
	uchar	type[4];
	uchar	flags[4];
	uchar	addr[4];
	uchar	offset[4];
	uchar	size[4];
	uchar	link[4];
	uchar	info[4];
	uchar	align[4];
	uchar	entsize[4];
};

struct Elf64SectBytes {
	uchar	name[4];
	uchar	type[4];
	uchar	flags[8];
	uchar	addr[8];
	uchar	offset[8];
	uchar	size[8];
	uchar	link[4];
	uchar	info[4];
	uchar	align[8];
	uchar	entsize[8];
};

struct Elf32SymBytes {
	uchar	name[4];
	uchar	value[4];
	uchar	size[4];
	uchar	info;		/* top4: bind, bottom4: type */
	uchar	other;
	uchar	shndx[2];
};

struct Elf64SymBytes {
	uchar	name[4];
	uchar	info;
	uchar	other;
	uchar	shndx[2];
	uchar	value[8];
	uchar	size[8];
};

struct Elf32ProgBytes {
	uchar	type[4];
	uchar	offset[4];
	uchar	vaddr[4];
	uchar	paddr[4];
	uchar	filesz[4];
	uchar	memsz[4];
	uchar	flags[4];
	uchar	align[4];
};

struct Elf64ProgBytes {
	uchar	type[4];
	uchar	flags[4];
	uchar	offset[8];
	uchar	vaddr[8];
	uchar	paddr[8];
	uchar	filesz[8];
	uchar	memsz[8];
	uchar	align[8];
};

Elf::Elf(){
	static uchar ElfMagic[4] = { 0x7F, 'E', 'L', 'F' };
	memcpy(magic, ElfMagic, 4);
}

Elf::~Elf(){
	int i;
	for( i=0; i<nprog; i++ )
		delete [] prog[i].base;
	for( i=0; i<nsect; i++ )
		delete [] sect[i].base;
	delete [] prog;
	delete [] sect;
}

int Elf::open(const char *file, int mode){
	if((fd = ::open(file, mode)) < 0){
		errstr("%s: %s", file, strerror(errno));
		return -1;
	}
	return init();
}

const char *Elf::fdopen(int ofd){
	fd = ofd;
#ifdef MMAP
	if (fstat(fd, &st) < 0) {
		errstr("%s: %s", file, strerror(errno));
		return errubf;
	}

	if ((maddr=mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0))==MAP_FAILED) {
		errstr("mmap: %s", file, strerror(errno));
		return errbuf;
	}
#endif
	const char *s = "";
	if(init() < 0) s = "Elf::init fail.";
#ifdef MMAP
	munmap(maddr, st.st_size);
#endif
	return s;
}

int Elf::init(){
	int i;
	uint version;
	uchar *p;
	ElfSect *s;
	union {
		Elf32HdrBytes h32;
		Elf64HdrBytes h64;
	} hdrb;

	/*
	 * parse header
	 */
#ifdef MMAP
	unpackehdr(maddr);
#else
	if(lseek(fd, 0, 0) < 0){
		errstr("%s", strerror(errno));
		goto error;
	}
	if(readn(fd, (uchar*)&hdrb, sizeof hdrb) != sizeof hdrb){
		errstr("%s", strerror(errno));
		goto error;
	}
	unpackehdr(&hdrb);
#endif
	if(memcmp(ehdr.magic, magic, 4) != 0){
		errstr("not and ELF file");
		goto error;
	}
	if(ehdr._class != ElfClass32 && ehdr._class != ElfClass64){
		errstr("bad ELF class - not 32-bit, 64-bit");
		goto error;
	}
	if(ehdr.encoding != ElfDataLsb && ehdr.encoding != ElfDataMsb){
		errstr("bad ELF encoding - not LSB, MSB");
		goto error;
	}
	/*
	 * the prog+section info is almost always small - just load it into memory.
	 */
	nprog = ehdr.phnum;
	prog = new ElfProg[nprog];
#ifdef MMAP
	for(i=0; i<nprog; i++)
		unpackphdr(i, ((uchar*)maddr+h->phoff+i*h->phentsize));
#else
	p = new uchar[ehdr.phentsize];
	for(i=0; i<nprog; i++){
		if(lseek(fd, ehdr.phoff+i*ehdr.phentsize, 0) < 0
		  || readn(fd, p, ehdr.phentsize) != ehdr.phentsize){
			errstr("%s", strerror(errno));
			goto error;
		}
		unpackphdr(i, p);
	}
	delete (uchar*)p;
#endif
	if((nsect = ehdr.shnum) == 0)
		goto nosects;
	sect = new ElfSect[nsect];
#ifdef MMAP
	for(i=0; i<nsect; i++)
		unpackshdr(i, ((uchar*)maddr+h->shoff+i*h->shentsize));
#else
	p = new uchar[ehdr.shentsize];
	for(i=0; i<nsect; i++){
		if(lseek(fd, ehdr.shoff+i*ehdr.shentsize, 0) < 0
		  || readn(fd, p, ehdr.shentsize) != ehdr.shentsize){
			errstr("%s", strerror(errno));
			goto error;
		}
		unpackshdr(i, p);
	}
	delete (uchar*)p;
#endif
	if(ehdr.shstrndx >= nsect){
		fprintf(stderr, "warning: bad string section index %d >= %d", ehdr.shstrndx, nsect);
		ehdr.shnum = nsect = 0;
		goto nosects;
	}
	errstr("map strings");
	if(map((s = &sect[ehdr.shstrndx])) < 0)
		goto error;

	for(i=0; i<nsect; i++)
		if(sect[i].name)
			sect[i].name = (char*)s->base + (ulong)sect[i].name;

	symtab = findsect(".symtab");
	if(symtab){
		if(symtab->link >= nsect)
			symtab = (ElfSect*)0;
		else{
			symstr = &sect[symtab->link];
			nsymtab = symtab->size / sizeof(Elf32SymBytes);
		}
	}
	dynsym = findsect(".dynsym");
	if(dynsym){
		if(dynsym->link >= nsect)
			dynsym = (ElfSect*)0;
		else{
			dynstr = &sect[dynsym->link];
			ndynsym = dynsym->size / sizeof(Elf32SymBytes);
		}
	}
	return 0;

nosects:
	if(ehdr.type == ElfTypeCore)	/* Linux */
		return 0;

	errstr("no elf sections");
	return -1;

error:
	return -1;
}

void Elf::unpackehdr(void *v){
	Elf32HdrBytes *b32;
	Elf64HdrBytes *b64;

	b32 = (Elf32HdrBytes*)v;
	memmove(ehdr.magic, b32->ident, 4);
	ehdr._class = b32->ident[4];
	ehdr.encoding = b32->ident[5];
	ehdr.abi = b32->ident[7];
	ehdr.abiversion = b32->ident[8];
	if(ehdr._class == ElfClass32){
		ehdr.type = e2(b32->type);
		ehdr.machine = e2(b32->machine);
		ehdr.version = e4(b32->version);
		ehdr.entry = e4(b32->entry);
		ehdr.phoff = e4(b32->phoff);
		ehdr.shoff = e4(b32->shoff);
		ehdr.flags = e4(b32->flags);
		ehdr.ehsize = e2(b32->ehsize);
		ehdr.phentsize = e2(b32->phentsize);
		ehdr.phnum = e2(b32->phnum);
		ehdr.shentsize = e2(b32->shentsize);
		ehdr.shnum = e2(b32->shnum);
		ehdr.shstrndx = e2(b32->shstrndx);
	}else{
		b64 = (Elf64HdrBytes*)v;
		ehdr.type = e2(b64->type);
		ehdr.machine = e2(b64->machine);
		ehdr.version = e4(b64->version);
		ehdr.entry = e8(b64->entry);
		ehdr.phoff = e8(b64->phoff);
		ehdr.shoff = e8(b64->shoff);
		ehdr.flags = e4(b64->flags);
		ehdr.ehsize = e2(b64->ehsize);
		ehdr.phentsize = e2(b64->phentsize);
		ehdr.phnum = e2(b64->phnum);
		ehdr.shentsize = e2(b64->shentsize);
		ehdr.shnum = e2(b64->shnum);
		ehdr.shstrndx = e2(b64->shstrndx);
	}
}

void Elf::unpackphdr(int i, void *v){
	if(ehdr._class == ElfClass32) {
		Elf32ProgBytes *b = (Elf32ProgBytes*)v;
		prog[i].type = e4(b->type);
		prog[i].offset = e4(b->offset);
		prog[i].vaddr = e4(b->vaddr);
		prog[i].paddr = e4(b->paddr);
		prog[i].filesz = e4(b->filesz);
		prog[i].memsz = e4(b->memsz);
		prog[i].flags = e4(b->flags);
		prog[i].align = e4(b->align);
	} else {
		Elf64ProgBytes *b = (Elf64ProgBytes*)v;
		prog[i].type = e4(b->type);
		prog[i].offset = e8(b->offset);
		prog[i].vaddr = e8(b->vaddr);
		prog[i].paddr = e8(b->paddr);
		prog[i].filesz = e8(b->filesz);
		prog[i].memsz = e8(b->memsz);
		prog[i].flags = e4(b->flags);
		prog[i].align = e8(b->align);
	}
}

void Elf::unpackshdr(int i, void *v){
	if(ehdr._class == ElfClass32) {
		Elf32SectBytes *b = (Elf32SectBytes*)v;
		sect[i].name = (char*)(uintptr_t)e4(b->name);
		sect[i].type = e4(b->type);
		sect[i].flags = e4(b->flags);
		sect[i].addr = e4(b->addr);
		sect[i].offset = e4(b->offset);
		sect[i].size = e4(b->size);
		sect[i].link = e4(b->link);
		sect[i].info = e4(b->info);
		sect[i].align = e4(b->align);
		sect[i].entsize = e4(b->entsize);
	} else {
		Elf64SectBytes *b = (Elf64SectBytes*)v;
		sect[i].name = (char*)(uintptr_t)e4(b->name);
		sect[i].type = e4(b->type);
		sect[i].flags = e8(b->flags);
		sect[i].addr = e8(b->addr);
		sect[i].offset = e8(b->offset);
		sect[i].size = e8(b->size);
		sect[i].link = e4(b->link);
		sect[i].info = e4(b->info);
		sect[i].align = e8(b->align);
		sect[i].entsize = e8(b->entsize);
	}
}

ElfSect *Elf::findsect(const char *name){
	int i;

	for(i=0; i<nsect; i++){
		if(sect[i].name == name)
			return &sect[i];

		if(sect[i].name && name
		  && strcmp(sect[i].name, name) == 0)
			return &sect[i];
	}
	errstr("elf section '%s' not found", name);
	return (ElfSect*)0;
}

const char *Elf::namety(int type){
	switch(type){
	case ElfProgNone: return "ElfProgNone";
	case ElfProgLoad: return "ElfProgLoad";
	case ElfProgDynamic: return "ElfProgDynamic";
	case ElfProgInterp:  return "ElfProgInterp";
	case ElfProgNote: return "ElfProgNote";
	case ElfProgShlib: return "ElfProgShlib";
	case ElfProgPhdr: return "ElfProgPhdr";
	case ElfProgTls: return "ElfProgTls";
	default: return "ElfProgUnknown";
	}
}

ElfProg *Elf::findprog(int type){
	int i;

	for(i=0; i<nprog; i++)
		if(prog[i].type == type)
			return &prog[i];

	errstr("Elf prog '%s' not found", namety(type));
	return (ElfProg*)0;
}

int Elf::map(ElfSect *s){
	if(s->base)
		return 0;

	s->base = new uchar[s->size];
#ifdef MMAP
	memmove(s->base, (uchar*)maddr+s->offset, s->size);
#else
	errstr("short read");
	if(lseek(fd, s->offset, 0) < 0
	  || readn(fd, s->base, s->size) != s->size){
		delete s->base;
		sect->base = 0;
		return -1;
	}
#endif
	return 0;
}

int Elf::map(ElfProg *p){
	if(p->base)
		return 0;

	p->base = new uchar[p->filesz];
#ifdef MMAP
	memmove(p->base, (uchar*)maddr+p->offset, p->filesz);
#else
	errstr("short read");
	if(lseek(fd, p->offset, 0) < 0
	  || readn(fd, p->base, p->filesz) != p->filesz){
		delete p->base;
		p->base = 0;
		return -1;
	}
#endif
	return 0;
}

int Elf::nseg(int type){
	int i, n = 0;
	for(i = 0; i < nprog; i++)
		if(prog[i].type == type)
			++n;
	return n;
}

int Elf::sym(int i, ElfSym *sym){
	ElfSect *_symtab, *_strtab;
	uchar *p;
	char *s;
	ulong x;

	if(i < 0){
		errstr("bad index %d in elfsym", i);
		return -1;
	}

	if(i < nsymtab){
		_symtab = symtab;
		_strtab = symstr;
extract:
		if(map(_symtab) < 0 || map(_strtab) < 0)
			return -1;

		if(ehdr._class == ElfClass32) {
			p = _symtab->base + i * sizeof(Elf32SymBytes);
			s = (char*)_strtab->base;
			x = e4(p);
			if(x >= _strtab->size){
				errstr("bad symbol name offset 0x%lux", x);
				return -1;
			}
			sym->name = s + x;
			sym->value = e4(p+4);
			sym->size = e4(p+8);
			x = p[12];
			sym->bind = x>>4;
			sym->type = x & 0xF;
			sym->other = p[13];
			sym->shndx = e2(p+14);
		} else {
			p = _symtab->base + i * sizeof(Elf64SymBytes);
			s = (char*)_strtab->base;
			x = e4(p);
			if(x >= _strtab->size){
				errstr("bad symbol name offset 0x%lux", x);
				return -1;
			}
			sym->name = s + x;
			x = p[4];
			sym->bind = x>>4;
			sym->type = x & 0xF;
			sym->other = p[5];
			sym->shndx = e2(p+6);
			sym->value = e8(p+8);
			sym->size = e8(p+16);
		}
		return 0;
	}
	i -= nsymtab;
	if(i < ndynsym){
		_symtab = dynsym;
		_strtab = dynstr;
		goto extract;
	}
	/* i -= ndynsym */

	errstr("symbol index out of range");
	return -1;
}

MemLayout Elf::memlayout(){
	if( ehdr.encoding == ElfDataLsb )
		return LSBFIRST;
	return MSBFIRST;
}

ushort Elf::e2(uchar *b){
	switch(ehdr.encoding){
	case ElfDataLsb:
		return leload2(b);
	case ElfDataMsb:
		return beload2(b);
	}
}

uint Elf::e4(uchar *b){
	switch(ehdr.encoding){
	case ElfDataLsb:
		return leload4(b);
	case ElfDataMsb:
		return beload4(b);
	}
}

uvlong Elf::e8(uchar *b){
	switch(ehdr.encoding){
	case ElfDataLsb:
		return leload8(b);
	case ElfDataMsb:
		return beload8(b);
	}
}

void Elf::errstr(const char *fmt, ...){
	va_list arg;

	va_start(arg, fmt);
	vsnprintf(errbuf, ERRMAX, fmt, arg);
	va_end(arg);
}

ushort Elf::leload2(uchar *b){
	return b[0] | (b[1]<<8);
}

uint Elf::leload4(uchar *b){
	return b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24);
}

uvlong Elf::leload8(uchar *b){
	return Elf::leload4(b) | ((uvlong)leload4(b+4) << 32);
}

ushort Elf::beload2(uchar *b){
	return (b[0]<<8) | b[1];
}

uint Elf::beload4(uchar *b){
	return (b[0]<<24) | (b[1]<<16) | (b[2]<<8) | b[3];
}

uvlong Elf::beload8(uchar *b){
	return ((uvlong)beload4(b) << 32) | Elf::beload4(b+4);
}
