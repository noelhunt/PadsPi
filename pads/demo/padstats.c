#include <pads.h>

void PadsRemInit();

int main(int argc, char **argv){
	if (argc == 2 && !strcmp(argv[1],"-R"))
		PadsRemInit();
	else
		PadsInit( argc, argv );
	NewPadStats();
	PadsServe();
}
