#ifndef MASTER_H
#define MASTER_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

void NewWd();

class Master : public PadRcv {
protected:
	Process	*child;
virtual	Process	*newProcess(Process*, const char*, const char* =0, const char* =0);
virtual	void	open();
PUBLIC(Master, U_MASTER)
	Core	*core;
	Pad	*pad;
	void	insert(Process*);
	Process	*search(const char*);
	Process	*makeproc(const char*, const char* =0, const char* =0);
		Master();
	char	*kbd(char *);
	char	*help();
};
#endif
