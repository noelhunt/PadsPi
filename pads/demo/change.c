# include <stdio.h>
# include <stdlib.h>

int Needs(int i){ return 1; }

int main(int argc, char **argv){
	int p_lo, p_hi;
	int d, lo, hi, middle, key = 1;
	int capacity, lines, change;

	if(argc!=6){
		fprintf(stderr, "change lo hi capacity lines middle\n");
		return 1;
	}

	p_lo = atoi(*++argv);
	p_hi = atoi(*++argv);
	capacity = atoi(*++argv);
	lines = atoi(*++argv);
	middle = (p_lo+p_hi)/2;
	if( middle<1 || middle>lines )
		middle = key ? 1 : lines;
	lo = middle+1;
	hi = middle;
	printf(">> capacity = %d\n", capacity);
	printf(">>    lines = %d\n", lines);
	printf(">>       lo = %d\n", lo);
	printf(">>   middle = %d\n", middle);
	printf(">>       hi = %d\n", hi);

	do {
		int d;
		change = 0;
		if( lo>0 && capacity>=(d=Needs(lo-1)) ){
			capacity -= d, --lo;
			change = 1;
		//	printf(">>	lo %d\n", lo);
		}
		if( hi<lines && capacity>=(d=Needs(hi+1)) ){
			capacity -= d, ++hi;
			change = 1;
		//	printf(">>	hi %d\n", hi);
		}
	} while( change );

	printf("lo %d\nhi %d\n", lo, hi);
	return 0;
}
