#ifndef SIGMASK_H
#define SIGMASK_H

/* sccsid[] = "%W%\t%G%" */

class SigMask : public PadRcv {
	friend class SigBit;
	friend class NrtxProcess;
	friend class UnixProcess;
	friend class HostProcess;
	Core	*core;
	Pad	*pad;
	long	mask;
	long	exechang;
	long	numlines;

	long	bit(long s)		{ return 1<<(s-1); }
	void	signalmask(long);
	void	setsig(long);
	void	clrsig(long);
	void	clrcurrsig(long);
	void	sendsig(long);
	void	open();
	void	execline(long);
	void	updatecore(const char* =0);
PUBLIC(SigMask,U_SIGMASK)
		SigMask(Core*);
	void	linereq(int,Attrib=0);
	void	hostclose();
	void	banner();
const	char	*enchiridion(long);
};

class SigBit : public PadRcv {
	friend class SigMask;
	int	bit;
	void	set(SigMask*);
	void	clr(SigMask*);
	void	send(SigMask*);
		SigBit() {}
};
#endif
