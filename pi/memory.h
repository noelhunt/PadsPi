#ifndef MEMORY_H
#define MEMORY_H
#ifndef UNIV_H
#include "univ.h"
#endif

/* sccsid[] = "%W%\t%G%" */

class Cell : public PadRcv {
	friend	class Memory;
	friend	class Process;
	Memory	*memory;
	long	addr;
	int	fmt;
	int	size;
	Cell	*sib;
	Cslfd	*spy;

	char	*kbd(char*);
	char	*help();
const	char	*enchiridion(long);
	void	relative(long);
	void	indirect();
	void	reformat(long);
	void	resize(int);
	void	display(const char* = 0, int=0);
	void	dodisplay(Bls&);
	void	asmblr();
	Index	carte();
	void	setspy(long);
	int	changed();
	char	*search(int, char*);
PUBLIC(Cell,U_CELL)
		Cell(Memory *m) { memory = m; }
};

class Memory : public PadRcv {
	friend	class Cell;
	Pad	*pad;
	Cell	*cellset;
	Cell	*current;
	void	makecell(Cell*,long);
	char	*prevpat;
	Core	*core;
	char	*kbd(char*);
	char	*help();
const	char	*enchiridion(long);
PUBLIC(Memory,U_MEMORY)
		Memory(Core*);
	void	open(long=0);
	void	userclose();
	void	banner();
	int	changes(long=0);
};
#endif
