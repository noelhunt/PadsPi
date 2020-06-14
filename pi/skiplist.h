#ifndef SKIPLIST_H
#define SKIPLIST_H

/* sccsid[] = "%W%\t%G%" */

#define DUMPLIST
#define ALLOWDUP

#ifdef DUMPLIST
#include <stdio.h>
#endif

typedef unsigned long ulong;

enum {
	MAXLEVEL=16,
	BitsInRandom=31
};

#define MaxLevel (MAXLEVEL-1)

#ifdef ALLOWDUP
#define	VOIDINT void
#else
#define	VOIDINT int
#endif

class Node {
friend	class	Skiplist;
	ulong	key;
	void	*val;
	Node	**forw;		/* variable sized array of forw pointers */
public:
		~Node();
		Node(int,ulong,void*);
};

class Skiplist {
	Node	*Sentinel;
	int	level;		// Maximum level list = number of levels in the list + 1
	Node	*head;
#ifndef RANDOMBITS
	float	prob;
#else
	int	randomsLeft;
	int	randomBits;
#endif
	int	randomLevel();
public:
		~Skiplist();
		Skiplist(float);
	VOIDINT	insert(ulong, void*);
	int	remove(ulong);
	void	*search(ulong);
#ifdef DUMPLIST
	void	dump();
#endif
};
#endif
