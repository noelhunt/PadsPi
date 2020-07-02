/*ident	"@(#)cls4:tools/demangler/dem.c	1.9" */
#ifdef __SUNPRO_C
#include <demangle.h>
#else
#include "univ.h"
#include <ctype.h>
#endif
SRCFILE("demangle.c")

static char sccsid[ ] = "%W%\t%G%";

#ifdef __SUNPRO_C

int ccdemangle(char **text, char *classname, int doargs){
	char* s;
	char buf2[MAXLINE];

	trace("demangle text.%s classname.%s",*text,(!classname)? "<null>": classname);
	s = *text;
	switch( cplus_demangle(s, buf2, MAXLINE) ){
	case 0:
		*text = sf("%s", buf2);
		return 1;
	case DEMANGLE_ENAME:
		return 0;
	case DEMANGLE_ESPACE:
		return 0;
	}
}

#else

#define SP_ALIGN 0x4			/* alignment of dynamic space blocks */

struct DEMCL;

struct DEMARG {
	char* mods;		/* modifiers and declarators (page 123 in */
				/* ARM), e.g. "CP" */

	long* arr;		/* dimension if mod[i] == 'A' else NULL */

	DEMARG* func;		/* list of arguments if base == 'F' */
				/* else NULL */

	DEMARG* ret;		/* return type if base == 'F' else NULL */

	DEMCL* clname;		/* class/enum name if base == "C" */

	DEMCL** mname;		/* class name if mod[i] == "M" */
				/* in argument list (pointers to members) */

	DEMARG* next;		/* next argument or NULL */

	char* lit;		/* literal value for PT arguments */
				/* e.g. "59" in A<59> */

	char base;		/* base type of argument, */
				/* 'C' for class/enum types */
};

struct DEMCL {
	char* name;		/* name of class or enum without PT args */
				/* e.g. "Vector" */

	DEMARG* clargs;		/* arguments to class, NULL if not PT */

	char* rname;		/* raw class name with __pt__ if PT */
				/* e.g. "A__pt__2_i" */

	DEMCL* next;		/* next class name, NULL if not nested */
};

struct DEM {
	char* f;		/* function or data name;  NULL if type name */
				/* see page 125 of ARM for predefined list */

	char* vtname;		/* if != NULL name of source file for vtbl */

	DEMARG* fargs;		/* arguments of function name if __opargs__ */
				/* else NULL */

	DEMCL* cl;		/* name of relevant class or enum or NULL */
				/* used also for type-name-only input */

	DEMARG* args;		/* args to function, NULL if data or type */

	short slev;		/* scope level for local variables or -1 */

	char sc;		/* storage class type 'S' or 'C' or: */
				/* i -> __sti   d --> __std */
				/* b -> __ptbl_vec */
};

#define MAXDBUF 4096

#define MAXLINE 1024			/* general buffer use */

#define MAXARG 100			/* max arguments in a function */

#define STRCMP(s, t) ((s)[0] != (t)[0] || strcmp((s), (t)) != 0)

static char* spbase;
static char cc;
static char* base;
static int baselen;
#define gc() {cc = baselen >= 1 ? *base++ : 0, baselen--;}

static int waserror = 0;

#define MAXSTACK 100
static char* stackp[MAXSTACK];
static int stackl[MAXSTACK];
static char stackc[MAXSTACK];
static int sp = -1;

#define ERROR() {waserror = 1; return NULL;}

static DEMARG*	getarglist();
static void	dem_printarglist(DEMARG*,char*,int);
static void	dem_printarg(DEMARG*,char*,int);
static void	dem_printcl(DEMCL*,char*);

/************************* UTILITIES *************************/

/* fatal errors */
static void fatal(const char* msg, const char* arg1, const char* arg2){
	char buf[MAXLINE];

	sprintf(buf, msg, arg1, arg2);
	PadsError("demangle fatal error: %s\n", buf);
}

/* get space */
static char* gs(int s){
	char* p;

	if (s < 1)
		fatal("bad argument to gs()", (char*)0, (char*)0);

	/* align space on SP_ALIGN boundary */

	while ((unsigned long)spbase & (SP_ALIGN - 1))
		spbase++;

	p = spbase;
	spbase += s;

	return p;
}

/* copy a string */
static char* copy(char* s){
	char* p;

	if (s == NULL || !*s)
		fatal("bad argument to copy()", (char*)0, (char*)0);

	p = gs(strlen(s) + 1);
	strcpy(p, s);
	return p;
}

/************************* DEMANGLE UTILITIES *************************/

/* push a string to scan */
static void push(char* s, int n){
	if (s == NULL || !*s || n < 1)
		fatal("bad argument to push()", (char*)0, (char*)0);
	if (sp + 1 >= MAXSTACK)
		fatal("overflow of stack in push()", (char*)0, (char*)0);

	sp++;
	stackp[sp] = base;
	stackl[sp] = baselen;
	stackc[sp] = cc;
	base = s;
	baselen = n;
	gc();
}

static void pop(){
	if (sp < 0)
		fatal("bad argument to pop()", (char*)0, (char*)0);

	base = stackp[sp];
	baselen = stackl[sp];
	cc = stackc[sp];
	sp--;
}

/************************* DEMANGLER *************************/

/* get a class name */
static DEMCL* getclass(){
	int n;
	char nbuf[MAXLINE];
	int i;
	int j;
	int iter;
	DEMCL* p;
	DEMCL* clhead;
	DEMCL* curr;
	DEMARG* ap;

	iter = 1;
	clhead = NULL;
	curr = NULL;

	/* fix for ambiguity in encoding */

	i = 0;
	if (isdigit(base[0])) {
		i = 1;
		if (isdigit(base[1]))
			i = 2;
	}
	if (isdigit(cc) && base[i] == 'Q' && isdigit(base[i + 1]) &&
	    base[i + 2] == '_') {
		gc();
		if (i)
			gc();
		if (i == 2)
			gc();
	}

	/* might be nested class */

	if (cc == 'Q') {
		gc();
		if (!isdigit(cc))
			ERROR();
		iter = cc - '0';
		if (iter < 1)
			ERROR();
		gc();
		if (cc != '_')
			ERROR();
		gc();
	}

	/* grab number of classes expected */

	while (iter-- > 0) {

		/* get a class */

		if (!isdigit(cc))
			ERROR();
		n = cc - '0';
		gc();
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (n < 1)
			ERROR();
		for (i = 0; i < n; i++) {
			if (!isalnum(cc) && cc != '_')
				ERROR();
			nbuf[i] = cc;
			gc();
		}
		nbuf[i] = 0;
		p = (DEMCL*)gs(sizeof(DEMCL));
		p->rname = copy(nbuf);
		p->clargs = NULL;

		/* might be a template class */

		for (j = 0; j < i; j++) {
			if (nbuf[j] == '_' && nbuf[j + 1] == '_' &&
			    nbuf[j + 2] == 'p' && nbuf[j + 3] == 't')
				break;
		}
		if (j == 0)
			ERROR();
		if (j == i) {
			p->name = copy(nbuf);
		}
		else {
			if (nbuf[j + 4] != '_' || nbuf[j + 5] != '_')
				ERROR();
			nbuf[j] = 0;
			p->name = copy(nbuf);
			j += 6;
			if (!isdigit(nbuf[j]))
				ERROR();
			n = nbuf[j] - '0';
			j++;
			if (isdigit(nbuf[j])) {
				n = n * 10 + nbuf[j] - '0';
				j++;
			}
			if (isdigit(nbuf[j])) {
				n = n * 10 + nbuf[j] - '0';
				j++;
			}
			if (n < 2)
				ERROR();
			if (nbuf[j] != '_')
				ERROR();
			j++;
			n--;
			if (!nbuf[j])
				ERROR();

			/* get arguments for template class */

			push(nbuf + j, n);
			if ((ap = getarglist()) == NULL || cc)
				ERROR();
			pop();
			p->clargs = ap;
		}
		p->next = NULL;

		/* link in to list */

		if (clhead != NULL) {
			curr->next = p;
			curr = p;
		}
		else {
			clhead = p;
			curr = clhead;
		}
	}

	return clhead;
}

/* copy an argument */
static DEMARG* arg_copy(DEMARG* p){
	DEMARG* p2;

	if (p == NULL)
		fatal("bad argument to arg_copy()", (char*)0, (char*)0);

	p2 = (DEMARG*)gs(sizeof(DEMARG));
	p2->mods = p->mods;
	p2->base = p->base;
	p2->arr = p->arr;
	p2->func = p->func;
	p2->clname = p->clname;
	p2->mname = p->mname;
	p2->lit = p->lit;
	p2->ret = p->ret;
	p2->next = NULL;

	return p2;
}

/* get an argument */
static DEMARG* getarg(int acmax, DEMARG* arg_cache[], int* ncount){
	char mods[100];
	int mc;
	int type;
	static DEMARG* p;
	DEMCL* clp;
	long n;
	DEMARG* farg;
	DEMARG* fret;
	char litbuf[MAXLINE];
	int lp;
	int foundx;
	long arrdim[100];
	int arrp;
	int i;
	int wasm;
	int waslm;
	char buf[MAXLINE];
	char buf2[MAXLINE];
	DEMCL* clist[100];
	int clc;
	int ic;

	/* might be stuff remaining from Nnn */

	if (ncount != NULL && *ncount > 0) {
		(*ncount)--;
		return arg_copy(p);
	}

	mc = 0;
	type = 0;
	clp = NULL;
	farg = NULL;
	fret = NULL;
	lp = 0;
	foundx = 0;
	arrp = 0;
	wasm = 0;
	clc = 0;

	/* get type */

	while (!type) {
		switch (cc) {

			/* modifiers and declarators */

			case 'X':
				gc();
				foundx = 1;
				break;
			case 'U':
			case 'C':
			case 'V':
			case 'S':
			case 'P':
			case 'R':
				mods[mc++] = cc;
				gc();
				break;

			/* fundamental types */

			case 'v':
			case 'c':
			case 's':
			case 'i':
			case 'l':
			case 'f':
			case 'd':
			case 'r':
			case 'e':
				type = cc;
				gc();
				break;

			/* arrays */

			case 'A':
				mods[mc++] = cc;
				gc();
				if (!isdigit(cc))
					ERROR();
				n = cc - '0';
				gc();
				while (isdigit(cc)) {
					n = n * 10 + cc - '0';
					gc();
				}
				if (cc != '_')
					ERROR();
				gc();
				arrdim[arrp++] = n;
				break;

			/* functions */

			case 'F':
				type = cc;
				gc();
				if ((farg = getarglist()) == NULL)
					ERROR();
				if (cc != '_')
					ERROR();
				gc();
				if ((fret = getarg(-1, (DEMARG**)0, (int*)0)) == NULL)
					ERROR();
				break;

			/* pointers to member */

			case 'M':
				mods[mc++] = cc;
				wasm = 1;
				gc();
				if ((clist[clc++] = getclass()) == NULL)
					ERROR();
				break;

			/* repeat previous argument */

			case 'T':
				gc();
tcase:
				if (!isdigit(cc))
					ERROR();
				n = cc - '0';
				gc();
#if 0
				if (isdigit(cc)) {
					n = n * 10 + cc - '0';
					gc();
				}
#endif
				if (n < 1)
					ERROR();
				if (arg_cache == NULL || n - 1 > acmax)
					ERROR();
				p = arg_copy(arg_cache[n - 1]);
				return p;

			/* repeat previous argument N times */

			case 'N':
				gc();
				if (!isdigit(cc))
					ERROR();
				if (ncount == NULL)
					ERROR();
				*ncount = cc - '0' - 1;
				if (*ncount < 0)
					ERROR();
				gc();
				goto tcase;

			/* class, struct, union, enum */

			case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9': case 'Q':
				if ((clp = getclass()) == NULL)
					ERROR();
				type = 'C';
				break;

			default:
				return NULL;
		}
	}

	/* template literals */

	if (type && foundx) {
		n = 0;
		waslm = 0;
		if (cc == 'L' && base[0] == 'M') {
			gc();
			gc();
			while (cc != '_' && cc)
				gc();
			if (!cc)
				ERROR();
			gc();
			while (cc != '_' && cc)
				gc();
			if (!cc)
				ERROR();
			gc();
			n = cc - '0';
			gc();
			if (isdigit(cc)) {
				n = n * 10 + cc - '0';
				gc();
			}
			if (isdigit(cc)) {
				n = n * 10 + cc - '0';
				gc();
			}
			waslm = 1;
		}
		else if (cc == 'L') {
			gc();
			if (!isdigit(cc))
				ERROR();
			n = cc - '0';
			gc();
			if (isdigit(cc) && base[0] == '_') {
				n = n * 10 + cc - '0';
				gc();
				gc();
			}
			if (cc == 'n') {
				gc();
				n--;
				litbuf[lp++] = '-';
			}
		}
		else if (cc == '0') {
			n = 1;
		}
		else if (isdigit(cc)) {
			n = cc - '0';
			gc();
			if (isdigit(cc)) {
				n = n * 10 + cc - '0';
				gc();
			}
		}
		else {
			ERROR();
		}
		if (!n && waslm) {
			strcpy(litbuf, "0");
			lp = 1;
		}
		else {
			ic = -1;
			while (n-- > 0) {
				if (!isalnum(cc) && cc != '_')
					ERROR();
				litbuf[lp++] = cc;
				gc();
				if (n > 0 && lp >= 2 &&
				    litbuf[lp - 1] == '_' && litbuf[lp - 2] == '_') {
					if ((clist[ic = clc++] = getclass()) == NULL)
						ERROR();
					litbuf[lp - 1] = 0;
					litbuf[lp - 2] = 0;
					lp -= 2;
					break;
				}	
			}
			litbuf[lp] = 0;
			if ((wasm && waslm) || ic >= 0) {
				dem_printcl(clist[ic >= 0 ? ic : 0], buf2);
				sprintf(buf, "%s::%s", buf2, litbuf);
				strcpy(litbuf, buf);
				lp = strlen(litbuf);
			}
		}
	}

	mods[mc] = 0;
	litbuf[lp] = 0;
	p = (DEMARG*)gs(sizeof(DEMARG));
	p->mods = mc ? copy(mods) : NULL;
	p->lit = lp ? copy(litbuf) : NULL;
	if (arrp > 0) {
		p->arr = (long*)gs(sizeof(long) * arrp);
		for (i = 0; i < arrp; i++)
			p->arr[i] = arrdim[i];
	}
	else {
		p->arr = NULL;
	}
	p->base = type;
	p->func = farg;
	p->ret = fret;
	p->clname = clp;
	if (clc > 0) {
		p->mname = (DEMCL**)gs(sizeof(DEMCL*) * (clc + 1));
		for (i = 0; i < clc; i++)
			p->mname[i] = clist[i];
		p->mname[clc] = NULL;
	}
	else {
		p->mname = NULL;
	}
	p->next = NULL;

	return p;
}

/* get list of arguments */
static DEMARG* getarglist(){
	DEMARG* p;
	DEMARG* head;
	DEMARG* curr;
	DEMARG* arg_cache[MAXARG];
	int acmax;
	int ncount;

	head = NULL;
	curr = NULL;

	acmax = -1;
	ncount = 0;

	for (;;) {

		/* get the argument */

		p = getarg(acmax, arg_cache, &ncount);
		if (p == NULL) {
			if (waserror)
				return NULL;
			return head;
		}

		/* cache it for Tn and Nnn */

		arg_cache[++acmax] = p;
		if (curr == NULL) {
			head = p;
			curr = head;
		}
		else {
			curr->next = p;
			curr = p;
		}
	}
}

/* entry point for demangling */
static int dem(char* s, DEM* p, char* buf){
	char nbuf[MAXLINE];
	int nc;
	long n;
	char* t;
	char* t2;
	char* ob;
	int flag;
	char buf2[MAXLINE];

	if (s == NULL || p == NULL || buf == NULL)
		return -1;

	if (!*s)
		return -1;

	/* set up space and input buffer management */

	spbase = buf;
	sp = -1;
	waserror = 0;

	p->fargs = NULL;
	p->cl = NULL;
	p->sc = 0;
	p->args = NULL;
	p->f = NULL;
	p->vtname = NULL;
	p->slev = -1;

	/* special case local variables */

	if (s[0] == '_' && s[1] == '_' && isdigit(s[2])) {
		t = s + 2;
		n = 0;
		while (isdigit(*t)) {
			n = n * 10 + *t - '0';
			t++;
		}
		if (*t) {
			p->f = copy(t);
			p->slev = (short)n;
			goto done2;
		}
	}

	/* special case sti/sti/ptbl */

	if (s[0] == '_' && s[1] == '_' &&
	    (!strncmp(s, "__sti__", 7) || !strncmp(s, "__std__", 7) ||
	    !strncmp(s, "__ptbl_vec__", 12))) {
		p->sc = s[4];
		t = (s[2] == 's' ? s + 7 : s + 12);
		while (*t == '_')
			t++;
		t2 = t;
		while (t2[0] && (t2[0] != '_' || t2[1] != 'c' || t2[2] != '_'))
			t2++;
		*t2 = 0;
		p->f = copy(t);
		cc = 0;
		goto done2;
	}

	/* special case type names */

	t = s;
	flag = 0;
	while (t[0] && (t[0] != '_' || t == s || t[-1] != '_'))
		t++;
	if (t[0] == '_' && t[1] == 'p' && t[2] == 't' &&
	    t[3] == '_' && t[4] == '_')
		flag = 1;
	if (t[0] == '_' && t[1] == '_' && t[2] == 'p' && t[3] == 't' &&
	    t[4] == '_' && t[5] == '_')
		flag = 1;
	if (!flag) {
		t = s;
		if ((t[0] == '_' && t[1] == '_' && t[2] == 'Q' &&
		    isdigit(t[3]) && t[4] == '_'))
			flag = 2;
	}
	if (flag) {
		sp = -1;
		waserror = 0;
		if (flag == 1) {
			sprintf(buf2, "%d%s", strlen(s), s);
			push(buf2, 9999);
		}
		else {
			push(s + 2, 9999);
		}
		if ((p->cl = getclass()) == NULL)
			return -1;
		cc = 0;
		goto done2;
	}

	sp = -1;
	push(s, 9999);
	waserror = 0;

	/* get function name */

	nc = 0;
	nbuf[0] = 0;
	while (isalnum(cc) || cc == '_') {
		nbuf[nc++] = cc;
		nbuf[nc] = 0;
		if (!base[0] ||
		    (base[0] == '_' && base[1] == '_' && base[2] != '_')) {
			gc();
			break;
		}
		gc();

		/* conversion operators */

		if (!STRCMP(nbuf, "__op")) {
			ob = base - 1;
			if ((p->fargs = getarg(-1, (DEMARG**)0, (int*)0)) == NULL)
				return -1;
			while (ob < base - 1)
				nbuf[nc++] = *ob++;
			nbuf[nc] = 0;
			break;
		}
	}
	if (!cc || (!isalpha(nbuf[0]) && nbuf[0] != '_'))
		return -1;

	/* pick off delimiter */

	if (cc == '_' && base[0] == '_') {
		gc();
		gc();
		if (!cc)
			return -1;
	}

	/* get class name */

	if (isdigit(cc) || cc == 'Q') {
		if ((p->cl = getclass()) == NULL)
			return -1;
	}

	/* a function template */

	else if (cc == 'p' && !strncmp(base, "t__F", 4)) {
		gc();
		gc();
		gc();
		gc();
		gc();
		if (!isdigit(cc))
			return -1;
		n = cc - '0';
		gc();
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (n < 1)
			return -1;
		while (n-- > 0) {
			if (!isalnum(cc) && cc != '_')
				return -1;
			gc();
		}
		if (cc != '_' || base[0] != '_')
			return -1;
		gc();
		gc();
	}

	if (!STRCMP(nbuf, "__vtbl")) {
		if (cc == '_' && base[0] == '_' && base[1])
			p->vtname = copy(base + 1);
		goto done;
	}

	/* const/static member functions */

	if ((cc == 'C' || cc == 'S') && base[0] == 'F') {
		p->sc = cc;
		gc();
	}

	/* get arg list for function */

	if (cc == 'F') {
		gc();
		if ((p->args = getarglist()) == NULL)
			return -1;
	}

done:
	if ((cc && STRCMP(nbuf, "__vtbl")) || waserror)
		return -1;

	p->f = copy(nbuf);

done2:
	return 0;
}

/************************* PRINT AN UNMANGLED NAME *************************/

/* format a class name */
static void dem_printcl(DEMCL* p, char* buf){
	int i;
	char buf2[MAXLINE];

	if (p == NULL || buf == NULL)
		fatal("bad argument to dem_printcl()", (char*)0, (char*)0);

	buf[0] = 0;
	i = 0;
	while (p != NULL) {
		i++;

		/* handle nested */

		if (i > 1)
			strcat(buf, "::");
		strcat(buf, p->name);

		/* template class */

		if (p->clargs != NULL) {
			if (buf[strlen(buf) - 1] == '<')
				strcat(buf, " ");
			strcat(buf, "<");
			dem_printarglist(p->clargs, buf2, 0);
			strcat(buf, buf2);
			if (buf[strlen(buf) - 1] == '>')
				strcat(buf, " ");
			strcat(buf, ">");
		}
		p = p->next;
	}
}

/* format an argument list */
static void dem_printarglist(DEMARG* p, char* buf, int sv){
	int i;
	char buf2[MAXLINE];

	if (p == NULL || buf == NULL || sv < 0 || sv > 1)
		fatal("bad argument to dem_printarglist()", (char*)0, (char*)0);

	/* special case single "..." argument */

	if (p->base == 'v' && p->mods == NULL && p->next != NULL &&
	    p->next->base == 'e' && p->next->next == NULL) {
		strcpy(buf, "...");
		return;
	}

	/* special case single "void" argument */

	if (p->base == 'v' && p->mods == NULL) {
		buf[0] = 0;
		return;
	}

	buf[0] = 0;
	i = 0;
	while (p != NULL) {
		i++;
		if (i > 1)
			strcat(buf, p->base == 'e' ? " " : ",");
		dem_printarg(p, buf2, sv);
		strcat(buf, buf2);
		p = p->next;
	}
}

/* format a single argument */
static void dem_printarg(DEMARG* p, char* buf, int f){
	const char* t;
	char bufc[MAXLINE];
	char bufc2[MAXLINE];
	char farg[MAXLINE];
	char fret[MAXLINE];
	const char* m;
	const char* mm;
	char pref[MAXLINE];
	int arrindx;
	long dim;
	char scr[MAXLINE];
	char ptrs[MAXLINE];
	int i;
	int sv;
	const char* s;
	const char* trail;
	int clc;

	if (p == NULL || buf == NULL || f < 0 || f > 1)
		fatal("bad argument to dem_printarg()", (char*)0, (char*)0);

	/* format the underlying type */

	sv = !f;

	switch (p->base) {

		/* fundamental types */

		case 'v':
			t = "void";
			break;
		case 'c':
			t = "char";
			break;
		case 's':
			t = "short";
			break;
		case 'i':
			t = "int";
			break;
		case 'l':
			t = "long";
			break;
		case 'f':
			t = "float";
			break;
		case 'd':
			t = "double";
			break;
		case 'r':
			t = "long double";
			break;
		case 'e':
			t = "...";
			sv = 1;
			break;

		/* functions */

		case 'F':
			dem_printarg(p->ret, fret, 0);
			dem_printarglist(p->func, farg, 0);
			break;

		/* class, struct, union, enum */

		case 'C':
			dem_printcl(p->clname, bufc);
			t = bufc;
			break;

		default:
			fatal("bad base type in dem_printarg()", (char*)0, (char*)0);
			break;
	}

	/* handle modifiers and declarators */

	pref[0] = 0;
	m = p->mods;
	if (m == NULL)
		m = "";

	/* const and unsigned */

	mm = m;
	while (*mm) {
		if (mm[0] == 'C' && (mm[1] != 'P' && mm[1] != 'R' && mm[1] != 'M') && (mm[1] || p->base != 'F')) {
			strcat(pref, "const ");
			break;
		}
		mm++;
	}
	mm = m;
	while (*mm) {
		if (*mm == 'U') {
			strcat(pref, "unsigned ");
			break;
		}
		mm++;
	}

	/* go through modifier list */

	mm = m;
	ptrs[0] = 0;
	arrindx = 0;
	clc = 0;
	while (*mm) {
		if (mm[0] == 'P') {
			sprintf(scr, "*%s", ptrs);
			strcpy(ptrs, scr);
		}
		else if (mm[0] == 'R') {
			sprintf(scr, "&%s", ptrs);
			strcpy(ptrs, scr);
		}
		else if (mm[0] == 'M') {
			dem_printcl(p->mname[clc++], bufc2);
			sprintf(scr, "%s::*%s", bufc2, ptrs);
			strcpy(ptrs, scr);
		}
		else if (mm[0] == 'C' && mm[1] == 'P') {
			sprintf(scr, " *const%s%s", isalnum(ptrs[0]) || ptrs[0] == '_' ? " " : "", ptrs);
			strcpy(ptrs, scr);
			mm++;
		}
		else if (mm[0] == 'C' && mm[1] == 'R') {
			sprintf(scr, " &const%s%s", isalnum(ptrs[0]) || ptrs[0] == '_' ? " " : "", ptrs);
			strcpy(ptrs, scr);
			mm++;
		}
		else if (mm[0] == 'C' && mm[1] == 'M') {
			dem_printcl(p->mname[clc++], bufc2);
			sprintf(scr, "%s::*const%s%s", bufc2, isalnum(ptrs[0]) || ptrs[0] == '_' ? " " : "", ptrs);
			strcpy(ptrs, scr);
			mm++;
		}
		else if (mm[0] == 'A') {
			dim = p->arr[arrindx++];
			s = sv ? "" : "@";
			if (!ptrs[0]) {
				sprintf(scr, "%s[%ld]", s, dim);
				sv = 1;
			}
			else if (ptrs[0] == '(' || ptrs[0] == '[') {
				sprintf(scr, "%s[%ld]", ptrs, dim);
			}
			else {
				sprintf(scr, "(%s%s)[%ld]", ptrs, s, dim);
				sv = 1;
			}
			strcpy(ptrs, scr);
		}
		else if (mm[0] == 'U' || mm[0] == 'C' || mm[0] == 'S') {
			/* ignore */
		}
		else {
			fatal("bad value in modifier list", (char*)0, (char*)0);
		}
		mm++;
	}

	/* put it together */

	s = sv ? "" : "@";
	if (p->base == 'F') {
		i = 0;
		if (ptrs[0] == ' ')
			i = 1;
		trail = "";
		if (p->mods != NULL && p->mods[strlen(p->mods) - 1] == 'C')
			trail = " const";
		if (ptrs[i])
			sprintf(buf, "%s%s (%s%s)(%s)%s", pref, fret, ptrs + i,
			    s, farg, trail);
		else
			sprintf(buf, "%s%s %s(%s)%s", pref, fret, s, farg, trail);
	}
	else {
		sprintf(buf, "%s%s%s%s%s", pref, t, ptrs[0] == '(' || isalnum(ptrs[0]) || ptrs[0] == '_' ? " " : "", ptrs, s);
	}
	if (p->lit != NULL) {
		if (isdigit(p->lit[0]) || p->lit[0] == '-')
			sprintf(scr, "(%s)%s", buf, p->lit);
		else
			sprintf(scr, "&%s", p->lit);
		strcpy(buf, scr);
	}
}

struct Ops {
	const char* encode;
	const char* name;
};

static struct Ops ops[] = {
	"__pp",		"operator++",
	"__as",		"operator=",
	"__vc",		"operator[]",
	"__nw",		"operator new",
	"__dl",		"operator delete",
	"__rf",		"operator->",
	"__ml",		"operator*",
	"__mm",		"operator--",
	"__oo",		"operator||",
	"__md",		"operator%",
	"__mi",		"operator-",
	"__rs",		"operator>>",
	"__ne",		"operator!=",
	"__gt",		"operator>",
	"__ge",		"operator>=",
	"__or",		"operator|",
	"__aa",		"operator&&",
	"__nt",		"operator!",
	"__apl",	"operator+=",
	"__amu",	"operator*=",
	"__amd",	"operator%=",
	"__ars",	"operator>>=",
	"__aor",	"operator|=",
	"__cm",		"operator,",
	"__dv",		"operator/",
	"__pl",		"operator+",
	"__ls",		"operator<<",
	"__eq",		"operator==",
	"__lt",		"operator<",
	"__le",		"operator<=",
	"__ad",		"operator&",
	"__er",		"operator^",
	"__co",		"operator~",
	"__ami",	"operator-=",
	"__adv",	"operator/=",
	"__als",	"operator<<=",
	"__aad",	"operator&=",
	"__aer",	"operator^=",
	"__rm",		"operator->*",
	"__cl",		"operator()",
	NULL,		NULL
};

/* format a function name */
static void dem_printfunc(DEM* dp, char* buf){
	int i;
	char buf2[MAXLINE];

	if (dp == NULL || buf == NULL)
		fatal("bad argument to dem_printfunc()", (char*)0, (char*)0);

	if (dp->f[0] == '_' && dp->f[1] == '_') {

		/* conversion operators */

		if (!strncmp(dp->f, "__op", 4) && dp->fargs != NULL) {
			dem_printarg(dp->fargs, buf2, 0);
			sprintf(buf, "operator %s", buf2);		
		}

		/* might be overloaded operator */

		else {
			i = 0;
			while (ops[i].encode != NULL && strcmp(ops[i].encode, dp->f))
				i++;
			if (ops[i].encode != NULL)
				strcpy(buf, ops[i].name);
			else
				strcpy(buf, dp->f);
		}
	}
	else {
		strcpy(buf, dp->f);
	}
}

/* entry point to formatting functions */
static int dem_print(DEM* p, char* buf, int doargs){
	char buf2[MAXLINE];
	char* s;
	int t;

	if (p == NULL || buf == NULL)
		return -1;

	buf[0] = 0;

	/* type names */

	if (p->f == NULL && p->cl != NULL) {
		dem_printcl(p->cl, buf);
		return 0;
	}

	/* sti/std */

	if (p->sc == 'i' || p->sc == 'd') {
		sprintf(buf, "%s:__st%c", p->f, p->sc);
		return 0;
	}
	if (p->sc == 'b') {
		sprintf(buf, "%s:__ptbl_vec", p->f);
		return 0;
	}

	/* format class name */

	buf2[0] = 0;
	if (p->cl != NULL) {
		dem_printcl(p->cl, buf2);
		strcat(buf, buf2);
		strcat(buf, "::");
	}

	/* special case constructors and destructors */

	s = buf2 + strlen(buf2) - 1;
	t = 0;
	while (s >= buf2) {
		if (*s == '>')
			t++;
		else if (*s == '<')
			t--;
		else if (*s == ':' && !t)
			break;
		s--;
	}
	if (!STRCMP(p->f, "__ct")) {
		strcat(buf, s + 1);
	}
	else if (!STRCMP(p->f, "__dt")) {
		strcat(buf, "~");
		strcat(buf, s + 1);
	}
	else {
		dem_printfunc(p, buf2);
		strcat(buf, buf2);
	}

	/* format argument list */

	if (doargs && p->args != NULL) {
		strcat(buf, "(");
		dem_printarglist(p->args, buf2, 0);
		strcat(buf, buf2);
		strcat(buf, ")");
	}

	/* const member functions */

	if (p->sc == 'C')
		strcat(buf, " const");

	return 0;
}
int ccdemangle(char **text, char *classname, int doargs){
	char sbuf[MAXDBUF];
	char buf2[MAXLINE];
	DEM da;
	const char* s;
	int l;

	trace("demangle text.%s classname.%s",*text,(!classname)? "<null>": classname);
	s = *text;
	if (dem((char *)s, &da, sbuf))
		return 0;
	s = buf2;
	dem_print(&da, (char *)s, doargs);
	if (classname) {
		l = strlen(classname);
		if (!strncmp(classname, s, l) && !strncmp("::", &s[l], 2))
			s += l + 2;
	}
	*text = (char *)sf("%s", s);
	return 1;
}

#endif
