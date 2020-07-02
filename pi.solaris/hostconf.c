#include "core.h"
#include "process.h"
#include "master.h"
#include "frame.h"
#include "i686core.h"
#include "hostcore.h"
SRCFILE("hostconf.c")

static char sccsid[ ] = "%W%\t%G%";

class I686HostCore : public I686Core, public HostCore {
public:
	I686HostCore(Process *pr, Master *m): Core(pr,m) {}
};

Core *HostProcess::newCore(Master *m){
	return new I686HostCore(this, m);
}
