#include <string.h>

static int *_mcbase;

void monitor(long lopc, long hipc, int *buf, long bufsiz, long cntsiz){
	int o;
	static int *sbuf, ssiz;
	struct phdr {
		int *lpc;
		int *hpc;
		int ncnt;
	};
	struct cnt {
		int *pc;
		long ncall;
	};

	if (lopc == 0) {
# ifdef PROFIL
		profil((short *)0, 0, 0, 0);
		o = creat("mon.out", 0666);
		write(o, sbuf, ssiz);
		close(o);
# endif
		return;
	}
	sbuf = buf;
	ssiz = bufsiz;
	buf[0] = (int)lopc;
	buf[1] = (int)hipc;
	buf[2] = cntsiz;
	_mcbase = (int *) (((int)buf) + sizeof(struct phdr));
	buf = (int *) (((int)_mcbase) + (o = cntsiz*sizeof(struct cnt)));
	bufsiz -= o + sizeof(struct phdr);
	memset((char *)_mcbase, 0, o);		/* clear counts */
	if (bufsiz<=0)
		return;
	o = bufsiz*65536.0/(hipc - lopc);
# ifdef PROFIL
	profil(buf, bufsiz, lopc, o);
# endif
}
