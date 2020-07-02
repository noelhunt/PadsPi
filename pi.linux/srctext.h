#ifndef SRCTEXT
#define SRCTEXT
#ifndef UNIV_H
#include "univ.h"
#endif
#include <stdio.h>

/* sccsid[] = "%W%\t%G%" */

class SrcText : public PadRcv {
	friend	class Stmt;
	friend	class SrcDir;
class	Pad	*pad;
class	Source	*source;
	int	lastline;
	char	**edge;
	char	*body;
	int	*funcnt;
	void	**stmts;
	int	current;
	char	*path;
	int	fd;
	time_t	compiletime;
	char	*prevpat;
	short	warned;
	short	promoted;

	void	DoNothing();
	void	addmulti(long);
	Index	multiindx(long,long,Bls&);
	void	promote();
	void	reopen();
class	Core	*core();
	char	*read(class Menu&);
class	Process	*process();
	void	go();
	void	currentstmt();
	void	stmtstep(long);
	void	stepinto();
	void	free();
	void	banner();
PUBLIC(SrcText,U_SRCTEXT)
		SrcText(Source*, long );
	void	open();
	void	linereq(int,Attrib=0);
	void	select(long,long=0);
	void	userclose();
	char	*kbd(char*);
	char	*help();
const	char	*enchiridion(long);
	char	*contextsearch(int,const char*,int);
	char	*srcline(long);
};

class MultiStmt : public PadRcv {
	friend	class SrcText;
	int	nstmt;
	Stmt	**stmt;
	void	req(long);
public:
		MultiStmt(int);
		~MultiStmt();
const	char	*enchiridion(long);
	char	*kbd(char*);
};

#endif
