#include <stdlib.h>
#include "univ.h"
#include "audit.h"
#include "core.h"
#include "master.h"
#include "process.h"
#include "hostcore.h"
SRCFILE("pi.c")

static char sccsid[ ] = "%W%\t%G%";

void ErrExit(const char *e){
	fprintf(stderr, "%s\n", e);
	exit(1);
}

char *TAPTO = ".tape";
char *PATH;
char *SRCPATH;
char version[] = VERSION;

#ifndef PI_FUNC
Hostfunc *hfn;
#endif
#ifdef HASHSTATS
int TypeGathered;
int HashStats[MAGIC2];
#endif

void PadsRemInit();
const char *PadsTermInit(int, char**, char*);

void LoadTerm(int argc, char **av){
	if (av[1] && !strcmp(av[1],"-R")) {
		PadsRemInit();
		return;
	}
	if (av[1] && !strcmp(av[1],"-r") && av[2])
		ErrExit(PadsTermInit(argc, av, av[2]));
	if( PadsInit(argc, av) )
		ErrExit("cannot load terminal");
}

void DoBatch(int argc, char **av){
	const char *core = "core", *aout = "a.out";
	if (argc--) core = *av++;
	if (argc) aout = *av++;
#ifdef __GNUC__
	HostProcess *hp = new HostProcess(0, core, aout, 0);
	hp->batch();
#else
	new HostProcess(0, core, aout, 0)->batch();
#endif
	exit(0);
}

int main(int argc, char **av){
	PATH = getenv("PATH");
	SRCPATH = getenv("PI_SRCPATH");
	if (argc == 2 && !strcmp(av[1],"-V")) {
		printf(version);
		exit(0);
	}
	if (argc >= 2 && !strcmp(av[1],"-t"))
		DoBatch(argc - 2, av + 2);
	::signal(SIGCHLD, SIG_IGN);
	LoadTerm(argc, av);
	extern char *TapTo;
	TapTo = TAPTO;
	NewHelp();
	NewWd();
	NewPadStats();
	new Audit;
#ifndef PI_FUNC
	hfn = new Hostfunc;
#endif
	new HostMaster;
	PadsServe();
}
