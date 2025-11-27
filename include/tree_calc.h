#ifndef K_TREE_CALC_H
#define K_TREE_CALC_H

#include <stdio.h>

#include "tree.h"

union value_t
{
    double number;
    int idx;
};

struct node_t
{
    type_t type = TYPE_UKNOWN;
    treeDataType value;

    // node_t *parent = NULL; FIXME: add parents
    node_t *left  = NULL;
    node_t *right = NULL;
};

enum operatorsIdxes
{
    OP_UKNOWN,
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
    char *name = NULL;
    size_t idx = 0;
};

struct differentiator_t
{
    treeLog_t log = {};

    tree_t expression = {};
    tree_t *diffTrees = NULL;
    size_t diffTreesCnt = 0;

    variable_t *variables = NULL;
    size_t variablesCapacity = 0;
    size_t variablesSize = 0;

    char *buffer = NULL;
};

struct operator_t
{
    const char *name     = NULL;
    operatorsIdxes idx   = OP_UKNOWN;
    ssize_t numberOfArgs = 0;
};

const operator_t operators[] = 
{
    {.name = "uknonwn", .idx = OP_UKNOWN,   },
    {.name = "+",       .idx = OP_ADD,      },
    {.name = "-",       .idx = OP_SUB,      },
    {.name = "*",       .idx = OP_MUL,      },
    {.name = "/",       .idx = OP_DIV,      },
    {.name = "^",       .idx = OP_POW,      },
    {.name = "log",     .idx = OP_LOG,      },
    {.name = "ln",      .idx = OP_LN,       },
    {.name = "sin",     .idx = OP_SIN,      }, 
    {.name = "cos",     .idx = OP_COS,      }, 
    {.name = "tg",      .idx = OP_TG,       }, 
};
const size_t operatorNumber = sizeof(operators) / sizeof(operator_t);


int DifferentiatorCtor          (differentiator_t *diff, size_t variablesCapacity);
void DifferentiatorDtor         (differentiator_t *diff);

const char * GetValueTypeName   (type_t type);
int TreeLoadFromFile            (differentiator_t *diff, tree_t *tree,
                                 const char *fileName, char **buffer, size_t *bufferLen);
int TreeSaveToFile              (tree_t *tree, const char *fileName);
int NodeSaveToFile              (node_t *node, FILE *file);

int TreeCalculate               (differentiator_t *diff, tree_t *expression);
double NodeCalculate            (differentiator_t *diff, node_t *node, double *values);

void TreeSimplify               (tree_t *tree);

int TreesDiff                    (differentiator_t *diff, tree_t *expression);
node_t *NodeDiff                (differentiator_t *diff, node_t *expression, tree_t *resTree,
                                 variable_t *argument);

#endif // K_TREE_CALC_H