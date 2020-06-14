#include "univ.h"

void DoCut(Line *l){
	if( l->attributes & USERCUT ){
		HostParent = Selected.pad;
		HostObject = (Pad *)l;
		ToHost( P_USERCUT, /* garbage */ 0 );
	}
	if( !(l->attributes&DONT_CUT) ) DelLine(l);
}

void CutLine(){
	if(!Selected.line) return;
	DoCut(Selected.line);
	Paint(Selected.pad);
}

void Sever(){
	register Line *l = Selected.line, *lu;

	if(!l) return;
	for( ; l != &Selected.pad->sentinel; l = lu ){	/* ISLINE */
		lu = l->up;
		DoCut(l);
	}
	Paint(Selected.pad);
}
