#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "tree_calc.h"

#include "tree.h"
#include "tree_load_infix.h"
#include "utils.h"
#include "float_math.h"

static double NodeCalculateDoMath       (node_t *node, double leftVal, double rightVal);
static double NodeGetVariable           (differentiator_t *diff, node_t *node);
static void AskVariableValue            (differentiator_t *diff, node_t *node);

static node_t *NodeSimplifyCalc         (tree_t *tree, node_t *node, bool *modified);
static node_t *NodeSimplifyTrivial      (tree_t *tree, node_t *node, bool *modified);

static node_t *NodeDiffMathOperation    (differentiator_t *diff,node_t *expression, 
                                         tree_t *resTree, variable_t *argument);
static node_t *NodeDiffVariable         (node_t *expression, tree_t *resTree, 
                                         variable_t *argument);
static bool NodeFindVariable            (node_t *node, variable_t *argument);
static int AskUserAboutDifferentation   (differentiator_t *diff, size_t *diffTimes, 
                                         variable_t **var);

// NOTE: diffNumber - how many times to differentiate, idk better naming
int DifferentiatorCtor (differentiator_t *diff, size_t variablesCapacity)
{
    assert (diff);

    TREE_DO_AND_RETURN (LogCtor (&diff->log));
    
    diff->variablesCapacity = variablesCapacity;
    diff->variablesSize     = 0;

    diff->variables = (variable_t *) calloc (variablesCapacity, sizeof (variable_t));
    if (diff->variables == NULL)
    {
        ERROR_LOG ("Error allocating memory for diff->variables - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    diff->diffTrees         = NULL;
    diff->diffTreesCnt      = 0;
    diff->varToDiff         = NULL;
    diff->buffer            = NULL;

    TREE_DO_AND_RETURN (TREE_CTOR (&diff->expression, &diff->log));

    size_t len = 0;
    TREE_DO_AND_RETURN (TreeLoadInfixFromFile (diff, &diff->expression, 
                        ktreeSaveFileName, &diff->buffer, &len));

    return TREE_OK;
}

void DifferentiatorDtor (differentiator_t *diff)
{
    assert (diff);

    LogDtor (&diff->log);

    TreeDtor (&diff->expression);

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        TreeDtor (&diff->diffTrees[i]);
    }

    free (diff->variables);
    diff->variables = NULL;
    diff->variablesCapacity = 0;
    diff->variablesSize     = 0;

    free (diff->diffTrees);
    diff->diffTrees         = NULL;
    diff->diffTreesCnt      = 0;

    diff->varToDiff         = NULL;

    free (diff->buffer);
    diff->buffer = NULL;
}


const char *GetTypeName (type_t type)
{
    switch (type)
    {
        case TYPE_UKNOWN:           return "uknown";
        case TYPE_CONST_NUM:        return "number";
        case TYPE_MATH_OPERATION:   return "operation";
        case TYPE_VARIABLE:         return "variable";

        default:                    return "ERROR";
    }
}

void TryToFindOperator (char *str, int len, type_t *type, treeDataType *value)
{
    assert (str);

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (str, keywords[i].name, (size_t) len) == 0)
        {
            *type = TYPE_MATH_OPERATION;
            value->idx = keywords[i].idx;

            DEBUG_LOG ("FOUND \"%s\"", keywords[i].name);
        }
    }
}

variable_t *FindVariableByName (differentiator_t *diff, char *varName, size_t varNameLen)
{
    assert (diff);
    assert (varName);

    DEBUG_LOG ("varName = \"%.*s\"", (int) varNameLen, varName);
    DEBUG_VAR ("%lu", varNameLen);

    for (size_t i = 0; i < diff->variablesSize; i++)
    {
        DEBUG_VAR ("%lu", i);
        DEBUG_PTR (diff->variables[i].name);
        DEBUG_LOG ("diff->variables[i].name = \"%.*s\"", 
                   (int)diff->variables[i].len,
                   diff->variables[i].name);
        
        int res = 12;
        if ((res = strncmp (diff->variables[i].name, varName, varNameLen)) == 0)
        {
            return &diff->variables[i];
        }
        DEBUG_VAR ("%d", res);
    }

    return NULL;
}

variable_t *FindVariableByIdx (differentiator_t *diff, size_t idx)
{
    assert (diff);

    for (size_t i = 0; i < diff->variablesSize; i++)
    {
        if (diff->variables[i].idx == idx)
            return &diff->variables[i];
    }

    return NULL;
}

const keyword_t *FindKeywordByIdx (size_t idx)
{
    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (keywords[i].idx == idx)
            return &keywords[i];
    }

    return NULL;
}

int FindOrAddVariable (differentiator_t *diff, char **curPos, 
                       size_t len, type_t *type, treeDataType *value)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (type);
    assert (value);

    *type = TYPE_VARIABLE;
    char *varName = *curPos;
    // (*curPos)[len] = '\0'; // TODO: this can crash something

    variable_t *variable = FindVariableByName (diff, varName, len);

    if (variable == NULL)
    {
        // NOTE: New function - add new variable ?
        TREE_DO_AND_RETURN (CheckForReallocVariables (diff));

        size_t idx = (diff->variablesSize);
        value->idx = idx;

        diff->variables[idx].name = varName;
        diff->variables[idx].len = len;
        diff->variables[idx].idx = idx;
        diff->variables[idx].value = NAN;

        DEBUG_LOG ("%.*s", (int)diff->variables[idx].len, diff->variables[idx].name);

        diff->variablesSize++;
    }
    else
    {
        value->idx = variable->idx;
    }

    DEBUG_VAR ("%lu", diff->variablesSize);
    DEBUG_LOG ("variable name is '%.*s'", 
               (int)diff->variables[diff->variablesSize].len, varName);
    DEBUG_LOG ("(*value).idx = '%lu'", (*value).idx);

    return TREE_OK;
}


int CheckForReallocVariables (differentiator_t *diff)
{
    assert (diff);

    if (diff->variablesSize >= diff->variablesCapacity)
    {
        if (diff->variablesCapacity == 0) 
            diff->variablesCapacity = 1;

        diff->variablesCapacity *= 2;

        variable_t *newVariables = (variable_t *) realloc (diff->variables, 
                                                           diff->variablesCapacity * sizeof (variable_t));
        if (newVariables == NULL)
        {
            ERROR_LOG ("Error reallocating memory for diff->variables - %s", strerror (errno));

            return TREE_ERROR_COMMON |
                   COMMON_ERROR_ALLOCATING_MEMORY;
        }

        diff->variables = newVariables;

    }

    return TREE_OK;
}

// =============  CALCULATION   =============

int TreeCalculate (differentiator_t *diff, tree_t *expression)
{
    assert (diff);
    assert (expression);

    DEBUG_VAR ("%lu", diff->variablesSize);
    
    double result = NodeCalculate (diff, expression->root);

    PRINT ("Expression is equals to %g\n", result);

    return TREE_OK;
}

double NodeCalculate (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    double leftVal  = NAN;
    double rightVal = NAN;

    if (node->left != NULL)
        leftVal = NodeCalculate (diff, node->left);

    if (node->right != NULL)
        rightVal = NodeCalculate (diff, node->right);

    switch (node->type)
    {
        case TYPE_UKNOWN:
            ERROR_LOG ("%s", "Uknown node while calculating tree");

            return TREE_ERROR_INVALID_NODE;
            
        case TYPE_CONST_NUM:
            return node->value.number;

        case TYPE_MATH_OPERATION:
            return NodeCalculateDoMath (node, leftVal, rightVal);

        case TYPE_VARIABLE:
            return NodeGetVariable (diff, node);
    
        default:
            assert (0 && "Add new case in NodeCalctulate or wtf bro");
    }
}

double NodeCalculateDoMath (node_t *node, double leftVal, double rightVal)
{
    assert (node);

    switch (node->value.idx)
    {
        case OP_ADD:    return leftVal + rightVal;
        case OP_SUB:    return leftVal - rightVal;
        case OP_MUL:    return leftVal * rightVal;
        case OP_DIV:    return leftVal / rightVal;
        case OP_POW:    return pow (leftVal, rightVal);
        case OP_LOG:    return logWithBase (leftVal, rightVal);
        case OP_LN:     return log (rightVal);
        case OP_SIN:    return sin (rightVal);
        case OP_COS:    return cos (rightVal);
        case OP_TG:     return tan (rightVal);
        case OP_CTG:    return 1 / tan (rightVal);
        case OP_ARCSIN: return asin (rightVal);
        case OP_ARCCOS: return acos (rightVal);
        case OP_ARCTG:  return atan (rightVal);
        case OP_ARCCTG: return 1 / atan (rightVal);
        case OP_SH:     return sinh (rightVal);
        case OP_CH:     return cosh (rightVal);
        case OP_TH:     return tanh (rightVal);
        case OP_CTH:    return 1 / tanh (rightVal);
        
        case OP_UKNOWN: 
            ERROR_LOG ("%s", "Uknown math operation in node"); 
            return NAN;
        
        default:
            assert (0 && "Bro, add another case for NodeCalculateDoMath()");
    }
}

double NodeGetVariable (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    // DEBUG_VAR ("%lu", node->value.idx);

    if (isnan (diff->variables[node->value.idx].value))
    {
        AskVariableValue (diff, node);
    }
    
    return diff->variables[node->value.idx].value;
}

void AskVariableValue (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    size_t idx = node->value.idx;

    DEBUG_VAR ("%lu", idx);

    int status   = 0;
    int attempts = 0;

    while (attempts < 5 && status != 1)
    {
        if (attempts >= 1)
        {
            ERROR_PRINT ("%s", "This is not a float point number. Please, try again");
        }

        PRINT ("Input value of variable '%.*s'\n"
               " > ",
               (int)diff->variables[idx].len,
               diff->variables[idx].name);

        status = scanf ("%lg", &diff->variables[idx].value);
        ClearBuffer ();

        attempts++;
    }

    if (attempts >= 5)
    {
        diff->variables[idx].value = 0;
        PRINT ("I am tired.\n"
               "Value of '%.*s' was set to %g.\n"
               "Think about your behavior", 
               (int) diff->variables[idx].len, diff->variables[idx].name, 
               diff->variables[idx].value);
    }
}

// ============= SIMPLIFICATION =============

#define NUM_(num)                                                               \
        NodeCtorAndFill (tree, TYPE_CONST_NUM, {.number = num}, NULL, NULL)

void TreeSimplify (differentiator_t *diff, tree_t *tree)
{
    assert (diff);
    assert (tree);

    bool modifiedFirst = true;
    bool modifiedSecond = true;
    while (modifiedFirst || modifiedSecond)
    {
        modifiedFirst  = false;
        modifiedSecond = false;

        tree->root = NodeSimplifyCalc (tree, tree->root, &modifiedFirst);
        TREE_DUMP (diff, tree, "%s", "After NodeSimplifyCalc()");

        tree->root = NodeSimplifyTrivial (tree, tree->root, &modifiedSecond);
        TREE_DUMP (diff, tree, "%s", "After NodeSimplifyTrivial()");
    }
}

node_t *NodeSimplifyCalc (tree_t *tree, node_t *node, bool *modified)
{
    assert (tree);
    assert (node);
    assert (modified);

    if (node->left != NULL)
        node->left = NodeSimplifyCalc (tree, node->left, modified);

    if (node->right != NULL)
        node->right = NodeSimplifyCalc (tree, node->right, modified);
    
    if (node->right == NULL) 
        return node;

    double leftVal  = NAN;
    double rightVal = NAN;

    if (node->left != NULL)
        leftVal = node->left->value.number;
    if (node->right != NULL)
        rightVal = node->right->value.number;

    node_t *newNode = NULL;

    if (node->right->type == TYPE_CONST_NUM && 
        (node->left == NULL || node->left->type  == TYPE_CONST_NUM))
    {
        switch (node->value.idx)
        {
            case OP_ADD:    newNode = NUM_ (leftVal + rightVal);                    break;
            case OP_SUB:    newNode = NUM_ (leftVal - rightVal);                    break;
            case OP_MUL:    newNode = NUM_ (leftVal * rightVal);                    break;
            case OP_DIV:    newNode = NUM_ (leftVal / rightVal);                    break;
            case OP_POW:    newNode = NUM_ (pow (leftVal, rightVal));               break;
            case OP_LOG:    newNode = NUM_ (logWithBase (leftVal, rightVal));       break;
            case OP_LN:     newNode = NUM_ (log (rightVal));                        break;
            case OP_SIN:    newNode = NUM_ (sin (rightVal));                        break;
            case OP_COS:    newNode = NUM_ (cos (rightVal));                        break;
            case OP_TG:     newNode = NUM_ (tan (rightVal));                        break;
            case OP_CTG:    newNode = NUM_ (1 / tan (rightVal));                    break;
            case OP_ARCSIN: newNode = NUM_ (asin (rightVal));                       break;
            case OP_ARCCOS: newNode = NUM_ (acos (rightVal));                       break;
            case OP_ARCTG:  newNode = NUM_ (atan (rightVal));                       break;
            case OP_ARCCTG: newNode = NUM_ (1 / atan (rightVal));                   break;
            case OP_SH:     newNode = NUM_ (sinh (rightVal));                       break;
            case OP_CH:     newNode = NUM_ (cosh (rightVal));                       break;
            case OP_TH:     newNode = NUM_ (tanh (rightVal));                       break;
            case OP_CTH:    newNode = NUM_ (1 / tanh (rightVal));                   break;
            
            case OP_UKNOWN: 
                ERROR_LOG ("%s", "Uknown math operation in node"); 
                return NULL;
            
            default:
                assert (0 && "Bro, add another case for NodeSimplifyCalc()");
        }
    }
    
    if (newNode == NULL)
        return node;

    TreeDelete (tree, &node);

    *modified = true;

    return newNode;
}

#define MUL_(left, right)                                                        \
        NodeCtorAndFill (tree, TYPE_MATH_OPERATION, {.idx = OP_MUL},             \
                         left, right)

#define cL NodeCopy (node->left,    tree)
#define cR NodeCopy (node->right,   tree)

#define L left
#define R right

#define IS_VALUE_(childNode, numberValue)                        \
        (node->childNode->type == TYPE_CONST_NUM &&              \
        IsEqual (node->childNode->value.number, numberValue))   

node_t *NodeSimplifyTrivial (tree_t *tree, node_t *node, bool *modified)
{
    assert (tree);
    assert (node);
    assert (modified);

    if (node->left != NULL)
        node->left = NodeSimplifyTrivial (tree, node->left, modified);

    if (node->right != NULL)
        node->right = NodeSimplifyTrivial (tree, node->right, modified);
    
    if (node->right == NULL) 
        return node;

    node_t *newNode = NULL;

    switch (node->value.idx)
    {
        case OP_ADD:
            if (IS_VALUE_ (L, 0))
                newNode = cR;
            else if (IS_VALUE_ (R, 0))
                newNode = cL;
            break;

        case OP_SUB:
            if (IS_VALUE_ (L, 0))
                newNode = MUL_ (NUM_(-1), cR);
            else if (IS_VALUE_ (R, 0))
                newNode = cL;
            break;

        case OP_MUL:
            if (IS_VALUE_ (L, 1))
                newNode = cR;
            else if (IS_VALUE_ (R, 1))
                newNode = cL;
            else if (IS_VALUE_ (L, 0) || IS_VALUE_ (R, 0))
                newNode = NUM_ (0);
            break;
        
        case OP_DIV:
            if (IS_VALUE_ (R, 1)) // (...) / 1
                newNode = cL;
            else if (IS_VALUE_ (L, 0)) // 0 / (...) 
                newNode = NUM_ (0);
            break;

        case OP_POW:
            if (IS_VALUE_ (R, 1)) // ^1
                newNode = cL;
            else if (IS_VALUE_ (R, 0) || IS_VALUE_ (L, 1)) // (...)^0 || 1^(...)
                newNode = NUM_ (1);
            break;

        default: break;
    }

    if (newNode == NULL) 
        return node;

    TreeDelete (tree, &node);

    *modified = true;

    return newNode;
}

#undef MUL_
#undef cL
#undef cR
#undef L
#undef R
#undef IS_VALUE_

#undef NUM_

// ============= DIFFERENTATION =============

#include "dsl_define.h"

// TODO: make new function
int TreesDiff (differentiator_t *diff, tree_t *expression)
{
    assert (diff);
    assert (expression);

    DEBUG_PRINT ("%s", "\n==========  START OF DIFFERENTATION  ==========\n");

    size_t diffTimes = 0;
    variable_t *var = NULL;

    TREE_DO_AND_RETURN (AskUserAboutDifferentation (diff, &diffTimes, &var));

    if (diffTimes == 0) 
        return TREE_OK;

    diff->diffTreesCnt = diffTimes;
    diff->diffTrees = (tree_t *) calloc (diff->diffTreesCnt, sizeof (tree_t));
    if (diff->diffTrees == NULL) 
    {
        ERROR_LOG ("Error allocating memory for diff->diffTrees - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        TREE_DO_AND_RETURN (TREE_CTOR (&diff->diffTrees[i], &diff->log));
    }

    DumpLatexFunction (diff, expression->root);

    fprintf (diff->log.latexFile, "\\section*{Продифференцируем нашу функцию %lu раз(-а)}\n", diff->diffTreesCnt);

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        DEBUG_VAR ("%lu", i);

        fprintf (diff->log.latexFile, "\\subsection*{Найдём %lu-ую производную}\n", i + 1);

        tree_t *tree = &diff->diffTrees[i];
        if (i == 0)
            tree->root = NodeDiff (diff, expression->root, tree, var);
        else
            tree->root = NodeDiff (diff, diff->diffTrees[i - 1].root, tree, var);

        if (tree->root == NULL)
            return TREE_ERROR_NULL_ROOT;

        TREE_DUMP (diff, tree, "devirative tree by '%s'", var->name);

        TreeSimplify (diff, tree);
        TREE_DUMP (diff, tree, "%s", "Simplified derivative tree");

        DumpLatexAnswer (diff, tree->root, i + 1);
    }

    DEBUG_PRINT ("%s", "==========   END OF DIFFERENTATION   ==========\n\n");

    return TREE_OK;
}

int AskUserAboutDifferentation (differentiator_t *diff, size_t *diffTimes, variable_t **var)
{
    assert (diff);

    PRINT ("How many times program should differentiate the expression?\n"
           " > ");

    int status = scanf ("%lu", diffTimes);
    ClearBuffer();
    
    if (status != 1)
    {
        *diffTimes = 1;

        ERROR_PRINT ("Bro, this is not correct number.\n"
                     "I will differentiate expression only %lu time",
                     *diffTimes);
    }

    if (diffTimes == 0)
        return TREE_OK;


    PRINT ("By which variable should program differentiate the expression?\n"
           " > ");

    char *varName = NULL;
    size_t varNameLen = 0;
    status = SafeReadLine (&varName, &varNameLen);

    if (status != COMMON_ERROR_OK)
        return TREE_ERROR_COMMON |
               status;

    // -1 because '\0'
    *var = FindVariableByName (diff, varName, varNameLen - 1);

    if (*var == NULL)
    {
        *var = &diff->variables[0];

        ERROR_PRINT ("There is no such variable\n"
                     "I will differentiate expression by '%.*s'",
                     (int)(*var)->len,
                     (*var)->name);
    }

    diff->varToDiff = *var;

    free (varName);

    return TREE_OK;
}

node_t *NodeDiff (differentiator_t *diff, node_t *expression, tree_t *resTree,
                  variable_t *argument)
{
    assert (diff);
    assert (resTree);
    assert (expression);
    assert (argument);

    DEBUG_VAR ("%p", expression);

    node_t *resNode = NULL;

    switch (expression->type)
    {
        case TYPE_UKNOWN:
            ERROR_LOG ("%s", "Uknown type of node");

            resNode = NULL;
            break;

        case TYPE_CONST_NUM:
            resNode = NUM_ (0);
            break;
            
        case TYPE_VARIABLE:
            resNode = NodeDiffVariable (expression, resTree, argument);
            break;
        
        case TYPE_MATH_OPERATION:
            resNode = NodeDiffMathOperation (diff, expression, resTree, argument);
            break;

        default:
            assert (0 && "This should never happen");
    }

    DumpLatexDifferentation (diff, expression, resNode, argument);

    return resNode;
}

node_t *NodeDiffVariable (node_t *expression, tree_t *resTree, variable_t *argument)
{
    assert (expression);
    assert (resTree);

    DEBUG_VAR ("%lu", expression->value.idx);
    DEBUG_VAR ("%s", argument->name);

    if (argument->idx != (size_t) expression->value.idx)
    {
        return NUM_ (0);
    }
    else 
    {
        return NUM_ (1);
    }
}

node_t *NodeDiffMathOperation (differentiator_t *diff, node_t *expression, tree_t *resTree, variable_t *argument)
{
    assert (diff);
    assert (expression);
    assert (resTree);
    assert (argument);

    switch (expression->value.idx)
    {
        case OP_UKNOWN:
            ERROR_LOG ("%s", "Uknown math operation");

            return NULL;

        case OP_ADD:
            return ADD_ (dL, dR);
        case OP_SUB:
            return SUB_ (dL, dR);
        case OP_MUL:
            return ADD_ (MUL_ (dL, cR), 
                             MUL_ (cL, dR));
        case OP_DIV:
            return DIV_ (SUB_ (MUL_ (dL, cR), 
                                       MUL_ (cL, dR)), 
                             MUL_ (cR, cR));

        case OP_POW:
        {
            bool xInBase    = NodeFindVariable (expression->left, argument);
            bool xInDegree  = NodeFindVariable (expression->right, argument);

            if (xInBase && xInDegree)
                return MUL_ (cN,
                             ADD_ (MUL_ (dL,
                                         LN_ (cR)),
                                   MUL_ (cL,
                                         DIV_ (dR,
                                               cR))));
            
            if (xInBase && !xInDegree)
                return MUL_ (MUL_ (cR,
                                   POW_ (cL,
                                         SUB_ (cR, NUM_(1)))),
                             dL);
            
            if (!xInBase && xInDegree)
                return MUL_ (cN,
                             LN_ (cL));
            
            if (!xInBase && !xInDegree)
                return NUM_ (0);
            
            assert (0 && "Uknown case of OP_POW");
        }

        // FIXME: check all cases
        case OP_LOG:
            return DIV_ (dR,
                        MUL_ (cR, LN_ (cL)));

        case OP_LN:
            return DIV_ (dR,
                         cR);

        case OP_SIN:
            return MUL_ (COS_ (cR),
                         dR);
        case OP_COS:
            return MUL_ (NUM_ (-1),
                             MUL_ (SIN_ (cR),
                                       dR));

        case OP_TG:
            return DIV_ (NUM_ (1),
                         POW_ (MUL_ (COS_ (cR), dR),
                               NUM_ (2)));
        
        case OP_CTG:
            return DIV_ (NUM_ (1),
                         POW_ (MUL_ (SIN_ (cR), dR),
                               NUM_ (2)));

        case OP_ARCSIN:
            return DIV_ (dR,
                         POW_ (SUB_ (NUM_(1), 
                                     POW_ (cR, NUM_ (2))),
                               NUM_ (0.5)));
        case OP_ARCCOS:
            return DIV_ (MUL_ (NUM_ (-1),
                               dR),
                         POW_ (SUB_ (NUM_(1), 
                                     POW_ (cR, NUM_ (2))),
                               NUM_ (0.5)));

        case OP_ARCTG:
            return DIV_ (dR,
                         ADD_ (NUM_(1), 
                               POW_ (cR, 
                                     NUM_ (2))));
        case OP_ARCCTG:
            return DIV_ (MUL_ (NUM_ (-1), 
                               dR),
                         ADD_ (NUM_(1), 
                               POW_ (cR, 
                                     NUM_ (2))));

        case OP_SH:
            return MUL_ (CH_ (cR),
                         dR);
        case OP_CH:
            return MUL_ (SH_ (cR),
                         dR);

        case OP_TH:
            return DIV_ (dR,
                         POW_ (CH_ (cR),
                               NUM_(2)));
        case OP_CTH:
            return DIV_ (MUL_ (NUM_ (-1),
                               dR),
                         POW_ (SH_ (cR),
                               NUM_(2)));
        

        default:
            assert (0 && "There is not implemented differentation for function");
    }
}


bool NodeFindVariable (node_t *node, variable_t *argument)
{
    assert (node);
    assert (argument);

    if (node->type == TYPE_VARIABLE &&
        (size_t) node->value.idx == argument->idx)
        return 1;

    bool found = false;

    if (node->left != NULL)
        found = NodeFindVariable (node->left, argument);

    if (found)
        return found;
        
    if (node->right != NULL)
        found = NodeFindVariable (node->right, argument);

    return found;
}

#include "dsl_undef.h"