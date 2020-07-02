#ifndef EXPR_H
#define EXPR_H
#ifndef UNIV_H
#include "univ.h"
#endif
#include "dtype.h"
#include "format.h"

/* sccsid[] = "%W%\t%G%" */

const char *OpName(Op);

class Spy{
	friend	class Expr;
	friend	class Phrase;
	Bls	b;
		Spy()	{}
};

class Expr : private PadRcv {
	friend	class Phrase;
	EDisc	edisc;
const	char	*under(Expr*);
const	char	*left(Expr*);
const	char	*evalunary(Frame*);
const	char	*evalbinary(Frame*);
const	char	*evalindex(Frame*,Expr*,long);
const	char	*evalcast();
const	char	*ascii(Frame*,int limit);
const	char	*enumformat();
const	char	*utypeformat(Frame*, Bls&);
const	char	*getval(Frame*);
const	char	*evaldotarrow(Frame*);
const	char	*evalcall(Frame*);
const	char	*evalassign(Frame*);
const	char	*invalidoperands(const char* =0);
const	char	*evaltextcomma(Frame*, Bls&);
const	char	*doevaltext(Frame*, Bls&);
	Expr	*actual(int i);
const	char	*eval(Frame*);
const	char	*evalenv(Frame*);
	Index	castcarte();
const	char	*textunary();
const	char	*textbinary();
const	char	*evalrange();
const	char	*floaterror();
const	char	*evalflop();
const	char	*enumid(Frame*);
PUBLIC(Expr,U_EXPR)
		Expr();	
		Expr(EDisc, Op, Expr*, Expr*, Cslfd, const char*, int, Symbol*);
const	char	*id;
	Symbol	*sym;
	Expr	*sub1;
	Expr	*sub2;
	DType	type;
	Cslfd	val;
	long	addr;
	Spy	*spy;
	short	bitaddr;
	char	op;
	char	evalerr;
const	char	*evaltext(Frame*, Bls&);
const	char	*text();
	Index	carte(Frame*);
	int	format();
	void	setspy(long);
	void	reformat(int,int=0);
	void	catchfpe();
};

Expr	*E_Id(const char*),
	*E_Sym(Symbol*),
	*E_Unary(Op,Expr*),
	*E_Binary(Expr*,Op,Expr*),
	*E_IConst(long);
#endif
