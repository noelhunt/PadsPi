#ifndef PHRASE_H
#define PHRASE_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class Phrase : private PadRcv {
	friend	class Frame;
	long	key;
	Expr	*expr;
	Frame	*frame;
	Pad	*pad();
	Phrase	*sib;
	void	applybinary(enum Op, Expr*);
	void	derive(Expr*);
	void	plugqindex(Expr*);
	void	plugqtype(Expr*);
	char	*help();
const	char	*enchiridion(long);
	char	*kbd(char*);
	void	numeric(int);
PUBLIC(Phrase,U_PHRASE)
		Phrase(Frame*, Phrase*, Expr*,long);
	void	applyarrow(Var*);
	void	applydot(Var*);
	void	allstar(UType*);
	void	alleval(UType*);
	void	applyunary(enum Op);
	void	evaluate();
	int	changed(Bls&);
	void	memory();
	void	reformat(int);
	void	setspy(long);
	void	applycast(DType*);
	void	strcast(long);
	void	enumcast(long);
	void	soretycast(long,short);
	void	increfcast(long);
	int	iscast();
};
#endif
