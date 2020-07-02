#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "dtype.h"
#include "avl.h"
SRCFILE("avl.c")

static char sccsid[ ] = "%W%\t%G%";

/*
 * In-memory database stored as self-balancing AVL tree.
 * See Lewis & Denenberg, Data Structures and Their Algorithms.
 */

Avltree::Avltree(){
	root = 0;
	walks = 0;
};

void Avltree::singleleft(Avl **tp, Avl *p){
	int l, r2;
	Avl *a, *c;

	a = *tp;
	c = a->n[1];

	r2 = c->bal;
	l = (r2 > 0? r2: 0)+1 - a->bal;

	if((a->n[1] = c->n[0]) != 0)
		a->n[1]->p = a;

	if((c->n[0] = a) != 0)
		c->n[0]->p = c;

	if((*tp = c) != 0)
		(*tp)->p = p;

	a->bal = -l;
	c->bal = r2 - ((l > 0? l: 0)+1);

}

void Avltree::singleright(Avl **tp, Avl *p){
	int l2, r;
	Avl *a, *c;

	a = *tp;
	c = a->n[0];
	l2 = - c->bal;
	r = a->bal + ((l2 > 0? l2: 0)+1);

	if((a->n[0] = c->n[1]) != 0)
		a->n[0]->p = a;

	if((c->n[1] = a) != 0)
		c->n[1]->p = c;

	if((*tp = c) != 0)
		(*tp)->p = p;

	a->bal = r;
	c->bal = ((r > 0? r: 0)+1) - l2;
}

void Avltree::doublerightleft(Avl **tp, Avl *p){
	singleright(&(*tp)->n[1], *tp);
	singleleft(tp, p);
}

void Avltree::doubleleftright(Avl **tp, Avl *p){
	singleleft(&(*tp)->n[0], *tp);
	singleright(tp, p);
}

void Avltree::balance(Avl **tp, Avl *p){
	switch((*tp)->bal){
	case -2:
		if((*tp)->n[0]->bal <= 0)
			singleright(tp, p);
		else if((*tp)->n[0]->bal == 1)
			doubleleftright(tp, p);
		else
			assert(0);
		break;

	case 2:
		if((*tp)->n[1]->bal >= 0)
			singleleft(tp, p);
		else if((*tp)->n[1]->bal == -1)
			doublerightleft(tp, p);
		else
			assert(0);
		break;
	}
}


int Avltree::cmp(Avl *p, Avl *q){
	int r = strcmp( ((Node *)p)->key, ((Node *)q)->key );
	return r > 0 ? 1 : r < 0 ? -1 : 0;
}

void Avltree::insert(Avl *newp, Avl **oldp){
	*oldp = 0;
	_insert(&root, 0, newp, oldp);
}

int Avltree::_insert(Avl **tp, Avl *p, Avl *r, Avl **rfree){
	int i, ob;

	if(*tp == 0){
		r->bal = 0;
		r->n[0] = 0;
		r->n[1] = 0;
		r->p = p;
		*tp = r;
		return 1;
	}
	ob = (*tp)->bal;
	if((i = cmp(r, *tp)) != 0){
		(*tp)->bal += i * _insert(&(*tp)->n[(i+1)/2], *tp, r, rfree);
		balance(tp, p);
		return ob == 0 && (*tp)->bal != 0;
	}

	/* install new entry */
	*rfree = *tp;		/* save old node for freeing */
	*tp = r;		/* insert new node */
	**tp = **rfree;		/* copy old node's Avl contents */
	if(r->n[0])		/* fix node's children's parent pointers */
		r->n[0]->p = r;
	if(r->n[1])
		r->n[1]->p = r;

	return 0;
}

int Avltree::successor(Avl **tp, Avl *p, Avl **r){
	int ob;

	if((*tp)->n[0] == 0){
		*r = *tp;
		*tp = (*r)->n[1];
		if(*tp)
			(*tp)->p = p;
		return -1;
	}
	ob = (*tp)->bal;
	(*tp)->bal -= successor(&(*tp)->n[0], *tp, r);
	balance(tp, p);
	return -(ob != 0 && (*tp)->bal == 0);
}

void Avltree::remove(Avl *key, Avl **oldp){
	*oldp = 0;
	_remove(&root, 0, key, oldp, 1);
}

int Avltree::_remove(Avl **tp, Avl *p, Avl *rx, Avl **del, int flag){
	int i, ob;
	Avl *r, *pr;

	if(*tp == 0)
		return 0;

	ob = (*tp)->bal;
	if((i=cmp(rx, *tp)) != 0){
		(*tp)->bal += i * _remove(&(*tp)->n[(i+1)/2], *tp, rx, del, flag);
		balance(tp, p);
		return -(ob != 0 && (*tp)->bal == 0);
	}

	if(flag)
		walkdel(*tp);

	pr = *tp;
	if(pr->n[i=0] == 0 || pr->n[i=1] == 0){
		*tp = pr->n[1-i];
		if(*tp)
			(*tp)->p = p;
		*del = pr;
		return -1;
	}

	/* deleting node with two kids, find successpr */
	pr->bal += successor(&pr->n[1], pr, &r);
	r->bal = pr->bal;
	r->n[0] = pr->n[0];
	r->n[1] = pr->n[1];
	*tp = r;
	(*tp)->p = p;
	/* node has changed; fix children's parent pointers */
	if(r->n[0])
		r->n[0]->p = r;
	if(r->n[1])
		r->n[1]->p = r;
	*del = pr;
	balance(tp, p);
	return -(ob != 0 && (*tp)->bal == 0);
}

void fatal(char *s){}

void Avltree::checkparents(Avl *a, Avl *p){
	if(a == 0)
		return;
	if(a->p != p)
		fatal("bad parent\n");
	checkparents(a->n[0], a);
	checkparents(a->n[1], a);
}

Avl* Avltree::findpredecessor(Avl *a){
	if(a == 0) return 0;

	if(a->n[0] != 0){
		/* predecessor is rightmost descendant of left child */
		for(a = a->n[0]; a->n[1]; a = a->n[1]) ;
		return a;
	}else{
		/* we're at a leaf, successor is a parent we enter from the right */
		while(a->p && a->p->n[0] == a)
			a = a->p;
		return a->p;
	}
}

Avl* Avltree::findsuccessor(Avl *a){
	if(a == 0)
		return 0;

	if(a->n[1] != 0){
		/* successor is leftmost descendant of right child */
		for(a = a->n[1]; a->n[0]; a = a->n[0])
			;
		return a;
	}else{
		/* we're at a leaf, successor is a parent we enter from the left going up */
		while(a->p && a->p->n[1] == a)
			a = a->p;
		return a->p;
	}
}

Avl* Avltree::_lookup(Avl *t, Avl *r, int neighbor){
	int i;
	Avl *p;

	p = 0;
	if(t == 0)
		return 0;
	do{
		assert(t->p == p);
		if((i = cmp(r, t)) == 0)
			return t;
		p = t;
		t = t->n[(i+1)/2];
	}while(t);
	if(neighbor == 0)
		return 0;
	if(neighbor < 0)
		return i > 0 ? p : findpredecessor(p);
	return i < 0 ? p : findsuccessor(p);
}

Avl* Avltree::search(Avl *key, int neighbor){
	return _lookup(root, key, neighbor);
}

Avl* Avltree::lookup(Avl *key){
	return _lookup(root, key, 0);
}

#ifdef NOTDEF
struct Avlwalk {
	int	started;
	int	moved;
	Avlwalk	*next;
	Avltree	*tree;
	Avl	*node;
};
#endif

Avlwalk::Avlwalk(Avltree *t){
	started = moved = 0;
	tree = t;
	next = t->walks;
	t->walks = this;
}

void Avltree::walkdel(Avl *a){
	if(a == 0) return;

	Avl *p = findpredecessor(a);
	for(Avlwalk *w = walks; w; w = w->next){
		if(w->node == a){
			/* back pointer to predecessor; not perfect but adequate */
			w->moved = 1;
			w->node = p;
			if(p == 0)
				w->started = 0;
		}
	}
}

Avl* Avlwalk::avlnext(Avlwalk *w){
	Avl *a;

	if(w->started==0){
		for(a = w->tree->root; a && a->n[0]; a = a->n[0])
			;
		w->node = a;
		w->started = 1;
	}else{
		a = tree->findsuccessor(w->node);
		if(a == w->node)
			abort();
		w->node = a;
	}
	return w->node;
}

Avl* Avlwalk::avlprev(Avlwalk *w){
	Avl *a;

	if(w->started == 0){
		for(a = w->tree->root; a && a->n[1]; a = a->n[1])
			;
		w->node = a;
		w->started = 1;
	}else if(w->moved){
		w->moved = 0;
		return w->node;
	}else{
		a = tree->findpredecessor(w->node);
		if(a == w->node)
			abort();
		w->node = a;
	}
	return w->node;
}

void Avlwalk::endwalk(Avlwalk *w){
	Avltree *t;
	Avlwalk **l;

	t = w->tree;
	for(l = &t->walks; *l; l = &(*l)->next){
		if(*l == w){
			*l = w->next;
			break;
		}
	}
	delete w;
}

#ifdef MAIN
#include <stdio.h>

static int depth = 0;
static char dbuf[256];

void Avltree::dump(Avl *t){
	if(t == 0)
		return;
	depth += 2;
	dump(t->n[0]);
	printf("%*s%s\n", depth, " ", ((Node*)t)->key);
	dump(t->n[1]);
	depth -= 2;
}

void insert(Avltree *t, char* key, void* val, ulong addr){
	Avl *old;
	Node *n = new Node;
	n->avl.p = 0;
	n->avl.n[0] = n->avl.n[1] = 0;
	n->avl.bal = 0;
	n->key = strdup( key );
	n->val = val;
	n->addr = addr;

	t->insert(&n->avl, &old);
}

void remove(Avltree *t, char* key){
	Avl *old = 0;
	Node n;
	n.avl.p = 0;
	n.avl.n[0] = n.avl.n[1] = 0;
	n.avl.bal = 0;
	n.val = 0;
	n.addr = 0;
	n.key = key;

	t->remove(&n.avl, &old);
	if( old )
		delete old;
}
#endif
