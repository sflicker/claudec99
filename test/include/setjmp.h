/* setjmp.h */
#ifndef CLAUDEC99_SETJMP_H
#define CLAUDEC99_SETJMP_H

typedef long jmp_buf[32];

int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif
