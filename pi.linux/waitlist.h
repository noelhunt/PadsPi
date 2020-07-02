class WaitMem {
	friend WaitList;
	HostCore	*core;
	WaitMem		*next;
	int		changed;
	_wait		status;
};

class WaitList {
	WaitMem		*head;
public:
	void		add(HostCore*);
	void		remove(HostCore*);
	int		wait(HostCore*, int);
			WaitList()		{ head = 0; }
};
