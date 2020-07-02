#ifndef SRCDIR_H
#define SRCDIR_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class SrcDir : public PadRcv {
	Pad	*pad;
	Process *process;
	void	fileoption(long);
	void	update();
PUBLIC(SrcDir,U_SRCDIR)
		SrcDir(class Process *p);
	void	open();
	void	hostclose();
	void	banner();
	char	*kbd(char*);
	char	*help();
const	char	*enchiridion(long);
};
#endif
