#ifndef LIB_H
#define LIB_H

/* sccsid[] = "%W%\t%G%" */

typedef unsigned char	uchar;
char *basename(char*);
char *slashname(char*);
char *pathexpand(char*, char*, int);
char *Name(char*,int);
char *SysErr(char* = "");
char *strcatfmt(char*, const char*, ... );
int alldigits(const char*);
int ReadOK(int, char*, int);
int WriteOK(int, char*, int);
long readn(int, uchar*, long);
long modified(int);
#endif
