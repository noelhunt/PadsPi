#ifndef SKIPLIST_H
#define SKIPLIST_H

/* sccsid[] = "%W%\t%G%" */

typedef unsigned long ulong;

enum {
	MAXLEVEL=16
};

#define MaxLevel (MAXLEVEL-1)

class Node {
friend	class	Skiplist;
	char	*key;		/* keys only */
	Node	**forw;		/* variable sized array of forw pointers */
public:
		~Node();
		Node(int,char*);
};

class Skiplist {
	Node	*Sentinel;
	int	level;		// Maximum level list = number of levels in the list + 1
	Node	*head;
	float	prob;
	int	genlevel();
	int	compare(char*, char*);
public:
		~Skiplist();
		Skiplist(float);
	void	insert(char*);
	int	remove(char*);
	int	search(char*);
};
#endif
