#ifndef DTYPE_H
#define DTYPE_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class DType : public PadRcv {
	friend class DwarfType;
	friend class DwarfSymTab;
	friend class SymTab;
	friend class Block;
	friend class Func;
	friend class Var;
	DType	*univ;		/* type: FTN, PTR, ARY. UType for Aggr. PadRcv*? */
	int	formatset();
	void	free();
PUBLIC(DType,U_DTYPE)
		DType(const DType&);
		DType();
	int	dim;
	short	pcc;
	ulong	over;		/* to be resolved */
class	UType	*utype();
	DType	*ref();
	DType	incref();
	DType	*decref();
const	char	*text();
	int	format();
struct	Index	carte();
	int	size_of();
	int	isary();
	int	isaryorptr();
	int	isftn();
	int	isintegral();
	int	isptr();
	int	iscnst();
	int	isreal();
	int	isscalar();
	int	isstrun();
	void	reformat(int,int=0);
};

char *PccName(int);
#endif
