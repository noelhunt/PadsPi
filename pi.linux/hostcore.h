#include "hostfunc.h"

/* sccsid[] = "%W%\t%G%" */

class HostMaster : public Master {
protected:
	char	**pscmds;
	char	*dopscmd(int);
	void	exit();
	char	*kbd(char*);
	char	*help();
	char	*enchiridion(long);
	void	open();
	void	openhelp();
	void	refresh(int);
	Process	*newProcess(Process*,const char*,const char*,const char*);
public:
		HostMaster();
};

class HostProcess : public Process {
	friend class HostCore;
protected:
	SigMask	*sigmsk;

	int	accept(Action);
	Index	carte();
	void	destroy();
	int	fixsymtab();
	void	hang();
	void	hangopen();
	void	hangtakeover();
	void	imprint();
	void	opensigmask();
	void	stop();
	void	substitute(HostProcess*);
	void	takeover();
	void	userclose();
	Core	*newCore(Master*);
public:
		HostProcess(Process*,const char*,const char*,const char*);
	void	open(long);
	void	batch();
};

class HostCore : virtual public Core {
	friend class HostProcess;
protected:
	Unixstate	state;
	Localproc	*localp;

	int	atsyscall();
	Behavs	behavetype();
	char	*dbreq(int, char* =0, int =0, int =0);
	char	*dostep(long,long,int);
	char	*exechang(long);
	int	exechangsupported();
	int	nsig();
	char	*readwrite(long,char*,int,int);
	long	regaddr();
	char	*resources();	
	long	scratchaddr();
	char	*signalclear();
	char	*signalmask(long);
	long	signalmaskinit();
	char	*signalname(long);
	char	*signalsend(long);
public:
	Behavs	behavs();
	char	*behavsnam();
	void	close();
	char	*destroy();
	char	*eventname();
	char	*liftbpt(Trap*);
	char	*laybpt(Trap*);
	char	*open();
	char	*problem();
	char	*reopen(char*,char*);
	char	*run();
	char	*stop();
};
