/*
 * State of Unix Process
 */
typedef struct Unixstate Unixstate;

struct Unixstate {
	short	state;
	short	code;
};

/* Possible states */
enum {
	UNIX_ERRORED,
	UNIX_BREAKED,
	UNIX_HALTED,
	UNIX_ACTIVE,
	UNIX_PENDING,
	UNIX_INCOHATE
};
