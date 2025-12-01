#ifndef K_TREE_LOAD_INFIX
#define K_TREE_LOAD_INFIX

#include "tree.h"
#include "tree_calc.h"

int TreeLoadInfixFromFile (differentiator_t *diff, tree_t *tree,
                           const char *fileName, char **buffer, size_t *bufferLen);

#endif // K_TREE_LOAD_INFIX