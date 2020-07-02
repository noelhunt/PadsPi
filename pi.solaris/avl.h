/* sccsid[] = "%W%\t%G%" */

#define nil	0

class MType {
	ulong	unit;
	int	uoff;
	DType	type;
public:
		MType(ulong,int,DType*);
	int	pcc(){ return type.pcc; }
};

class Avlwalk;

typedef struct Avl Avl;

struct Avl {
	Avl	*p;		/* parent	*/
	Avl	*n[2];		/* children	*/
	int	bal;		/* balance bits	*/
};

typedef struct Node Node;

struct Node {
	Avl	avl;
	char	*key;
	void	*val;
	ulong	addr;	
};

class Avltree {
	friend class Avlwalk;
	Avl	*root;
	int	_insert(Avl**, Avl*, Avl*, Avl**);
	int	_remove(Avl**, Avl*, Avl*, Avl**, int);
	Avl*	_lookup(Avl*, Avl*, int);
	void	singleleft(Avl**, Avl*);
	void	singleright(Avl**, Avl*);
	void	doublerightleft(Avl**, Avl*);
	void	doubleleftright(Avl**, Avl*);
	void	balance(Avl**, Avl*);
	int	successor(Avl**, Avl*, Avl**);
	int	cmp(Avl*, Avl*);
	void	checkparents(Avl*, Avl*);
	Avlwalk	*walks;
public:
		Avltree();
	void	remove(Avl*, Avl**);
	void	insert(Avl*, Avl**);
	Avl	*lookup(Avl*);
	Avl*	search(Avl*, int);
	Avl*	findsuccessor(Avl*);
	Avl*	findpredecessor(Avl*);
	void	walkdel(Avl*);
	void	show(){ dump( root ); }
	void	dump(Avl*);
};

class Avlwalk {
	friend class Avltree;
	int	started;
	int	moved;
	Avl	*node;
	Avlwalk	*next;
	Avltree	*tree;
public:
		Avlwalk(Avltree*);
	Avl*	avlnext(Avlwalk*);
	Avl*	avlprev(Avlwalk*);
	void	endwalk(Avlwalk*);
};
