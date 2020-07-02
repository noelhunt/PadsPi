#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "skiplist.h"

Node::~Node(){
	delete [] forw;
}

Node::Node(int r, char *k){
	key = k;
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
	Sentinel = new Node(0, ".NIL.");
	prob = p;
	level = 0;
	head = new Node(MAXLEVEL,0);
	for(int i=0; i<MAXLEVEL; i++)
		head->forw[i] = Sentinel;
	srand48( time(0) );
};

int Skiplist::genlevel(){
	int b, rlevel = 0;
	while( (drand48() < prob) && (rlevel < MaxLevel) )
		rlevel++;
	return rlevel;
};

void Skiplist::insert(char *key){
	int k;
	Node *update[MAXLEVEL];
	Node *p,*q;

	p = head;
	k = level;
	do {
		while ( q=p->forw[k], compare(q->key,key)<0 ) p = q;
		update[k] = p;
	} while(--k>=0);

	if ( !compare(q->key, key) )
		return;

	k = genlevel();
	if (k>level) {
		k = ++level;
		update[k] = head;
	};
	q = new Node(k,key);
	do {
		p = update[k];
		q->forw[k] = p->forw[k];
		p->forw[k] = q;
	} while(--k>=0);
}

int Skiplist::remove(char *key){
	int k,m;
	Node *update[MAXLEVEL];
	Node *p,*q;

	p = head;
	k = m = level;
	do {
		while ( q=p->forw[k], compare(q->key,key)<0 ) p = q;
		update[k] = p;
	} while(--k>=0);

	if ( !compare(q->key, key) ) {
		for(k=0; k<=m && (p=update[k])->forw[k] == q; k++)
			p->forw[k] = q->forw[k];
		delete q;
		while( head->forw[m] == Sentinel && m > 0 )
			m--;
		level = m;
		return 1;
	}
	else return 0;
}

int Skiplist::search(char *key){
	int k = level;
	Node *p = head,*q;
	do 
		while ( q=p->forw[k], compare(q->key,key)<0 ) p = q;
	while (--k>=0);
	if ( !q->key || strcmp(q->key, key) ) return 0;
	return 1;
};

int Skiplist::compare(char *s, char *t){
	if( strcmp(s, ".NIL.") )
		return strcmp(s, t);
	return 1;
}
