/* sccsid[] = "%W%\t%G%" */
# define TNULL PTR    /* pointer to UNDEF */
# define FARG 1
# define CHAR 2
# define SHORT 3
# define INT 4
# define LONG 5
# define FLOAT 6
# define DOUBLE 7
# define CLASSTY 8
# define STRTY 9
# define UNIONTY 10
# define ENUMTY 11
# define MOETY 12
# define REFTY 13
# define SUBRTY 14
# define UCHAR 15
# define USHORT 16
# define UNSIGNED 17
# define ULONG 18
# define UNDEF 19
# define BITS 20
# define UBITS 21
# define VOID 22

# define PTR  0100
# define FTN  0200
# define ARY  0300

/* qualifiers */

# define CNST 01000
# define VLTL 02000
# define RSTR 03000

# define BTMASK 077
# define BTSHIFT 6
# define TSHIFT 2
# define TMASK 0300
# define QMASK 03000

# define BTYPE(x)   (x&BTMASK)		/* basic type of x */

# define ISPTR(x)   ((x&TMASK)==PTR)
# define ISFTN(x)   ((x&TMASK)==FTN)	/* is x a function type */
# define ISARY(x)   ((x&TMASK)==ARY)	/* is x an array type */

# define ISCNST(x)  ((x&QMASK)==CNST)
# define ISVLTL(x)  ((x&QMASK)==VLTL)
# define ISRSTR(x)  ((x&QMASK)==RSTR)

# define INCREF(x)  (((x&~BTMASK)<<TSHIFT)|PTR|(x&BTMASK))
# define DECREF(x)  (((x>>TSHIFT)&~BTMASK)|(x&BTMASK))
