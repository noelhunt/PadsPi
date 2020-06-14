#include <stdio.h>
#include <stdlib.h>

#define MJR(i)	((i)>>8)
#define MNR(i)	((i)&0xFF)
#define CARTESIZE(s) (sizeof(Carte) + (s)*sizeof(Index))
typedef unsigned char uchar;
typedef unsigned int Index;
typedef struct Carte Carte;
struct Carte {
	int	size;		/* host.size != term.size */
	uchar	attrib;
	uchar	width;
	uchar	pad[2];
	Index	bin[];		/* C99: FLEXIBLE ARRAY MEMBER OF CLASS */
};

void main(){
	Carte *c = malloc(CARTESIZE(3));
	c->size = 3;
	c->attrib = 11;
	c->width = 20;
	c->bin[0] = 0x081fc918;
	c->bin[1] = 0x080d41dc;
	c->bin[2] = 0x081f794c;
}
