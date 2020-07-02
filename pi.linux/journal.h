#ifndef HISTORY_H
#define HISTORY_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class Journal : private PadRcv {
	int	length;
	long	key;
	long	lastreq;
	Bls	*bls;
	int	*ct;	
const	char	*proc;
	Pad	*pad;
	void	banner();
	void	linereq(int,Attrib=0);
PUBLIC(Journal,U_JOURNAL)
	void	open();
	void	hostclose();
		Journal(const char*);
	void	insert(const char* ...);
const 	char	*enchiridion(long);
};
#endif /* HISTORY_H */
