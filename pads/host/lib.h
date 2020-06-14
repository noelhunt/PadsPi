#define A_LARGE	256
#define A_USER	0x55000000

enum Event { POOL=0, ALLOC=1, FREE=2, EVENT=3 };

class Allocator {
	union M {
		long size;
		M    *link;
	};
	M	*freelist[A_LARGE];
	long	req[A_LARGE];
	long	event[A_LARGE][EVENT];
public:
		Allocator();
	void	*alloc(size_t);
	void	free(void*);
	char	*profile(long);
};

extern Allocator NewDel;
