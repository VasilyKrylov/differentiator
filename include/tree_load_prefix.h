#ifndef K_TREE_LOAD_PREFIX
#define K_TREE_LOAD_PREFIX

#include "tree.h"
#include "tree_calc.h"

int TreeLoadPrefixFromFile      (differentiator_t *diff, tree_t *tree,
                                 const char *fileName, char **buffer, size_t *bufferLen);

#endif // K_TREE_LOAD_PREFIX