// Everything in zlib is char even if the project is Unicode!

#ifndef _OSZDEBUG_H
#define _OSZDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

void zTrace(char *lpszFormat, ...);
void zTrace0(char *lpszFormat);
void zTracec(char c);

#ifdef __cplusplus
}
#endif

#endif /* _OSZDEBUG_H */
