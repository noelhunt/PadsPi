#include <pads.h>
#include <signal.h>
#ifdef i386
#define NOFILE 60
#else
#include <sys/param.h>
#endif /* i386 */
SRCFILE("popen.c")
static int popen_pid[NOFILE];

#define SIG_ARG_TYP SIG_TYP

FILE *Popen(const char *cmd, const char *mode){
	int parent = (*mode == 'r') ? 0 : 1;
	int child  = (*mode == 'r') ? 1 : 0;
	int p[2];

	if( pipe(p) < 0 ) return NULL;
	int pid = fork();
	if( pid == 0) {
		close(child);
		dup(p[child]);
		setuid(getuid());
		setgid(getgid());
		for( int i = 0; i < NOFILE; ++i )
			if( i != child ) close(i);
		execl("/bin/sh", "sh", "-c", cmd, 0);
		_exit(1);
	}
	if(pid == -1)
		return NULL;
	close(p[child]);
	popen_pid[p[parent]] = pid;
	return(fdopen(p[parent], mode));
}

#include <sys/types.h>
#include <sys/wait.h>

int Pclose(FILE *ptr){
	static SIG_TYP stat[4];
	static int sig[4] = { SIGINT, SIGQUIT, SIGHUP, SIGPIPE };
	int f, r, status, i;

	f = fileno(ptr);
	fclose(ptr);
	for( i = 0; i < 4; ++i )
		stat[i] = signal(sig[i], (SIG_ARG_TYP)SIG_IGN);
	while((r = wait(&status)) != popen_pid[f] && r != -1) {}
	if(r == -1)
		status = -1;
	for( i = 0; i < 4; ++i )
		signal(sig[i], (SIG_ARG_TYP)stat[i]);
	return(status);
}
