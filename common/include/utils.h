#ifndef K_UTILS_H
#define K_UTILS_H

#include <stdio.h>

int SafeMkdir       (const char *fileName);
void ClearBuffer    ();
char *SkipSpaces    (char *buffer);
char *ReadFile      (const char *inputFileName, size_t *bufferLen);
int SafeReadLine    (char **str, size_t *len);

#endif // K_UTILS_H