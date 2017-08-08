/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: stdio.h
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
# error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_STDIO__
#define __CMIX_STDIO__

#pragma cmix header: <stdio.h>

typedef __CMIX(iu) size_t ;
typedef __CMIX() FILE ;
typedef __CMIX() fpos_t ;

#ifndef NULL
# pragma cmix taboo: NULL
# define NULL ((void*)0)
#endif

extern const int _IOFBF, _IOLBF, _IONBF ;
extern const size_t BUFSIZ ;
extern const int EOF, FOPEN_MAX, FILENAME_MAX, L_tmpnam ;
extern const int SEEK_CUR, SEEK_END, SEEK_SET ;
extern const int TMP_MAX ;
extern FILE *const stderr, *const stdin, *const stdout ;

#pragma cmix well-known constant: _IOFBF _IOLBF _IONBF BUFSIZ EOF
#pragma cmix well-known constant: FOPEN_MAX FILENAME_MAX L_tmpnam
#pragma cmix well-known constant: SEEK_CUR SEEK_END SEEK_SET TMP_MAX
#pragma cmix well-known constant: stderr stdin stdout

/* ISO C 7.9.4 Operations on files */
int remove(const char*);
int rename(const char*,const char*);
FILE *tmpfile(void);
char *tmpnam(char *);
#pragma cmix well-known: remove rename tmpfile tmpnam

/* ISO C 7.9.5 File access functions */
int fclose(FILE*);
int fflush(FILE*);
FILE *fopen(const char*,const char*);
FILE *freopen(const char *,const char *,FILE *);
void setbuf(FILE*,char*);
void setvbuf(FILE*,char *,int,size_t);
#pragma cmix well-known: fclose fflush fopen freopen setbuf setvbuf

/* ISO C 7.9.6 Formatted input/output functions */
int fprintf(FILE*,const char*,...);
int fscanf(FILE*,const char*,...);
int printf(const char*,...);
int scanf(const char*,...);
int sprintf(char*,const char*,...);
int sscanf(const char*,const char*,...);
/* the v* versions are omitted as C-Mix/II doesn't handle varargs yet */
#pragma cmix well-known: fprintf fscanf printf scanf sprintf sscanf

/* ISO C 7.9.7 Character input/output functions */
int fgetc(FILE *);
char *fgets(char*,int,FILE*);
int fputc(int,FILE*);
int fputs(const char*,FILE*);
int getc(FILE *);
int getchar(void);
char *gets(char *s);
int putc(int c,FILE *);
int putchar(int);
int puts(const char*);
int ungetc(int,FILE *);
#pragma cmix well-known: fgetc fgets fputc fputs getc getchar
#pragma cmix well-known: gets putc putchar puts ungetc

/* ISO C 7.9.8 Direct input/output functions */
size_t fread(void*,size_t,size_t,FILE*);
size_t fwrite(const void*,size_t,size_t,FILE*);
#pragma cmix well-known: fread fwrite

/* ISO C 7.9.9 File positioning functions */
int fgetpos(FILE*,fpos_t);
int fseek(FILE*,long,int);
int fsetpos(FILE*,fpos_t);
long ftell(FILE*);
void rewind(FILE*);
#pragma cmix well-known: fgetpos fseek fsetpos ftell rewind

/* ISO C 7.9.10 Error-handling functions */
void clearerr(FILE*);
int feof(FILE*);
int ferror(FILE*);
void perror(const char*);
#pragma cmix well-known: clearerr feof ferror perror

#endif
