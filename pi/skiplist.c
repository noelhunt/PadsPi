#ifdef DUMPLIST
#include <stdio.h>
#endif
#include <stdlib.h>
#include "skiplist.h"

Node::~Node(){
	delete [] forw;
}

Node::Node(int r, ulong k, void *v){
	key = k;
	val = v;
	forw = new Node*[r+1];
}

Skiplist::~Skiplist(){
	Node *p,*q;
	p = head;
	do {
		q = (Node *)p->forw[0];
		delete p;
		p = q; 
	} while( p != Sentinel );
};

Skiplist::Skiplist(float p){
	Sentinel = new Node(0, 0x7fffffff, 0);
#ifndef RANDOMBITS
	prob = p;
#else
	randomBits = random();
	randomsLeft = BitsInRandom/2;
#endif
	level = 0;
	head = new Node(MAXLEVEL,0,0);
	for(int i=0; i<MAXLEVEL; i++)
		head->forw[i] = Sentinel;
#ifndef RANDOMBITS
	srand48( time(0) );
#endif
};

int Skiplist::randomLevel(){
	int b, rlevel = 0;
#ifdef RANDOMBITS
	do {
		b = randomBits&3;
		if (!b) rlevel++;
		randomBits>>=2;
		if (--randomsLeft == 0) {
			randomBits = random();
			randomsLeft = BitsInRandom/2;
		};
	} while (!b);
	return(rlevel>MaxLevel ? MaxLevel : rlevel);
#else
	while( (drand48() < prob) && (rlevel < MaxLevel) )
		rlevel++;
	return rlevel;
#endif
};

VOIDINT Skiplist::insert(ulong key, void* val){
	int k;
	Node *update[MAXLEVEL];
	Node *p,*q;

	p = head;
	k = level;
	do {
		while (q = p->forw[k], q->key < key) p = q;
		update[k] = p;
	} while(--k>=0);
#ifndef ALLOWDUP
	if (q->key == key) {
		q->value = value;
		return(0);
	};
#endif
	k = randomLevel();
	if (k>level) {
		k = ++level;
		update[k] = head;
	};
	q = new Node(k,key,val);
	do {
		p = update[k];
		q->forw[k] = p->forw[k];
		p->forw[k] = q;
	} while(--k>=0);
#ifndef ALLOWDUP
	return(1);
#endif
}

int Skiplist::remove(ulong key){
	int k,m;
	Node *update[MAXLEVEL];
	Node *p,*q;

	p = head;
	k = m = level;
	do {
		while (q = p->forw[k], q->key < key) p = q;
		update[k] = p;
	} while(--k>=0);

	if (q->key == key) {
		for(k=0; k<=m && (p=update[k])->forw[k] == q; k++)
			p->forw[k] = q->forw[k];
		free(q);
		while( head->forw[m] == Sentinel && m > 0 )
			m--;
		level = m;
		return 1;
	}
	else return 0;
}

void *Skiplist::search(ulong key){
	int k = level;
	Node *p = head,*q;
	do 
		while (q = p->forw[k], q->key < key) p = q;
	while (--k>=0);
	if (q->key != key) return 0;
	return q->val;
};

#ifdef DUMPLIST
#include <stdio.h>
void Skiplist::dump(){
	int ntypes = 0;
	int wide = 80, ncol = 7, i = 0;
	Node *p = head;
	while( p != Sentinel ){
# ifdef NOTDEF
		if( i++ == ncol ) i = 0, printf("\n");
		printf("%10d", p->key);
# endif
		ntypes++;
		p = p->forw[0];
	}
	printf("\n\n%d types at %d levels\n", ntypes, level);
}
#endif
