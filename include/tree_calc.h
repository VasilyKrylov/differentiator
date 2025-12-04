#ifndef K_TREE_CALC_H
#define K_TREE_CALC_H

#include <stdio.h>

#include "tree.h"

typedef size_t variable_idx_t; // TODO think

enum keywordIdxes_t : variable_idx_t
{
    OP_UNKNOWN,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_LOG,
    OP_LN,
    OP_SIN,
    OP_COS,
    OP_TG,
    OP_CTG,
    OP_ARCSIN,
    OP_ARCCOS,
    OP_ARCTG,
    OP_ARCCTG,
    OP_SH,
    OP_CH,
    OP_TH,
    OP_CTH,
};

struct variable_t
{
    char *name   = NULL;
    size_t len   = 0;

    variable_idx_t idx   = 0;
    double value = 0;
};

struct differentiator_t
{
    treeLog_t log = {};

    tree_t expression   = {};
    tree_t taylor       = {};
    tree_t *diffTrees   = NULL;
    size_t diffTreesCnt = 0;

    variable_t *variables    = NULL;
    size_t variablesCapacity = 0;
    size_t variablesSize     = 0;

    variable_t *varToDiff = NULL;

    char *buffer = NULL;
};

struct keyword_t
{
    const char *name        = NULL;
    size_t nameLen          = 0;
    keywordIdxes_t idx      = OP_UNKNOWN;
    bool isFunction         = 0;
    size_t numberOfArgs     = 0;
    const char *latexFormat = NULL;
};

#define KEYWORD(nameStr, idxKey, isFunctionKey, numberOfArgsKey, texForm)   \
        {.name = nameStr,                                                   \
         .nameLen = sizeof (nameStr) - 1,                                   \
         .idx = idxKey,                                                     \
         .isFunction = isFunctionKey,                                       \
         .numberOfArgs = numberOfArgsKey,                                   \
         .latexFormat = texForm}

const keyword_t keywords[] = 
{
    KEYWORD ("unknown_keyword", OP_UNKNOWN, 0,  0, "unknown%e"          ),
    KEYWORD ("+",               OP_ADD,     0,  0, "%l \n\t+ %r%e"      ),
    KEYWORD ("-",               OP_SUB,     0,  0, "%l \n\t- %r%e"      ),
    KEYWORD ("*",               OP_MUL,     0,  0, "%l \n\t \\cdot %r%e"),
    KEYWORD ("/",               OP_DIV,     0,  0, "\\frac {%l}{%r}%e"  ),
    KEYWORD ("^",               OP_POW,     0,  0, "{(%l)} ^ {%r}%e"    ),
    KEYWORD ("log",             OP_LOG,     1,  2, "\\log_{%l} (%r)%e"  ),
    KEYWORD ("ln",              OP_LN,      1,  1, "\\ln {(%r)}%e"      ),
    KEYWORD ("sin",             OP_SIN,     1,  1, "\\sin (%r)%e"       ),
    KEYWORD ("cos",             OP_COS,     1,  1, "\\cos (%r)%e"       ),
    KEYWORD ("tg",              OP_TG,      1,  1, "\\tan (%r)%e"       ),
    KEYWORD ("ctg",             OP_CTG,     1,  1, "\\ctan (%r)%e"      ),
    KEYWORD ("arcsin",          OP_ARCSIN,  1,  1, "\\arcsin (%r)%e"    ),
    KEYWORD ("arccos",          OP_ARCCOS,  1,  1, "\\arccos (%r)%e"    ),
    KEYWORD ("arctg",           OP_ARCTG,   1,  1, "\\arctg (%r)%e"     ),
    KEYWORD ("arcctg",          OP_ARCCTG,  1,  1, "\\arcctg (%r)%e"    ),
    KEYWORD ("sh",              OP_SH,      1,  1, "\\sh (%r)%e"        ),
    KEYWORD ("ch",              OP_CH,      1,  1, "\\ch (%r)%e"        ),
    KEYWORD ("th",              OP_TH,      1,  1, "\\th (%r)%e"        ),
    KEYWORD ("cth",             OP_CTH,     1,  1, "\\cth (%r)%e"       ),
};
const size_t kNumberOfKeywords = sizeof(keywords) / sizeof(keyword_t);


int DifferentiatorCtor              (differentiator_t *diff, size_t variablesCapacity);
void DifferentiatorDtor             (differentiator_t *diff);

const char *GetTypeName             (type_t type);

variable_t *FindVariableByIdx       (differentiator_t *diff, size_t idx);
variable_t *FindVariableByName      (differentiator_t *diff, char *varName, size_t varNameLen);
const keyword_t *FindKeywordByIdx   (size_t idx);

int CheckForReallocVariables        (differentiator_t *diff);
int FindOrAddVariable               (differentiator_t *diff, char **curPos, size_t len, 
                                     type_t *type, treeDataType *value);
void TryToFindOperator              (char *str, int len, type_t *type, treeDataType *value);

// int TreeSaveToFile               (tree_t *tree, const char *fileName);
// int NodeSaveToFile               (node_t *node, FILE *file);
bool NodeFindVariable               (node_t *node, variable_t *argument);

int TreeCalculate                   (differentiator_t *diff, tree_t *expression);
double NodeCalculate                (differentiator_t *diff, node_t *node);

void TreeSimplify                   (differentiator_t *diff, tree_t *tree);

int TreesDiff                       (differentiator_t *diff, tree_t *expression);
node_t *NodeDiff                    (differentiator_t *diff, node_t *expression, tree_t *tree,
                                     variable_t *argument);

#endif // K_TREE_CALC_H