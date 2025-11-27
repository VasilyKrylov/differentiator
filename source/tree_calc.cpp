#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "tree_calc.h"

#include "tree.h"
#include "utils.h"
#include "float_math.h"

static int TreeLoadNode                 (differentiator_t *diff, node_t **node,
                                         char *buffer, char **curPos);
static int TreeLoadNodeAndFill          (differentiator_t *diff, node_t **node,
                                         char *buffer, char **curPos);
static int TreeLoadChildNodes           (differentiator_t *diff, node_t **node,
                                         char *buffer, char **curPos);
static void TryToFindOperator           (char *str, int len, type_t *type, treeDataType *value);

static int FindOrAddVariable            (differentiator_t *diff, char **curPos, int len, 
                                         type_t *type, treeDataType *value);
static variable_t *FindVariableByName   (differentiator_t *diff, char *varName);
static int CheckForReallocVariables     (differentiator_t *diff);

static double NodeCalculateDoMath       (node_t *node, double leftVal, double rightVal);
static double NodeGetVariable           (differentiator_t *diff, node_t *node, double *values);
static void AskVariableValue            (differentiator_t *diff, node_t *node, double *values);

static node_t *NodeDiffMathOperation    (differentiator_t *diff,node_t *expression, tree_t *resTree, variable_t *argument);
static node_t *NodeDiffVariable         (node_t *expression, tree_t *resTree, variable_t *argument);
static bool NodeFindVariable            (node_t *node, variable_t *argument);
int AskUserAboutDifferentation          (differentiator_t *diff, size_t *diffTimes, variable_t **var);



// NOTE: diffNumber - how many times to differentiate, idk better naming
int DifferentiatorCtor (differentiator_t *diff, size_t variablesCapacity)
{
    assert (diff);

    TREE_DO_AND_RETURN (LogCtor (&diff->log));
    
    diff->variables = (variable_t *) calloc (variablesCapacity, sizeof (variable_t));
    if (diff->variables == NULL)
    {
        ERROR_LOG ("Error allocating memory for diff->variables - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    diff->variablesCapacity = variablesCapacity;

    TREE_DO_AND_RETURN (TREE_CTOR (&diff->expression, &diff->log));

    size_t len = 0;
    TREE_DO_AND_RETURN (TreeLoadFromFile (diff, &diff->expression, treeSaveFileName, &diff->buffer, &len));

    return TREE_OK;
}

void DifferentiatorDtor (differentiator_t *diff)
{
    assert (diff);

    TreeDtor (&diff->expression);

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        TreeDtor (&diff->diffTrees[i]);
    }

    LogDtor (&diff->log);

    free (diff->variables);
    diff->variables = NULL;
    diff->variablesCapacity = 0;
    diff->variablesSize     = 0;

    free (diff->diffTrees);
    diff->diffTrees = NULL;
    diff->diffTreesCnt = 0;

    free (diff->buffer);
    diff->buffer = NULL;
}

int TreeSaveToFile (tree_t *tree, const char *fileName)
{
    FILE *outputFile = fopen (fileName, "w");
    if (outputFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", fileName);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    int status = NodeSaveToFile (tree->root, outputFile);

    fclose (outputFile);

    return status;
}

int NodeSaveToFile (node_t *node, FILE *file)
{
    assert (node);
    assert (file);

    // NOTE: maybe add macro for printf to check it's return code
    if (node->type == TYPE_CONST_NUM)
    {
        fprintf (file, "(\"%g\"", node->value.number);
    }
    else if (node->type == TYPE_MATH_OPERATION)
    {
        // FIXME:
    }
    else if (node->type == TYPE_VARIABLE)
    {
        // FIXME:
    }

    if (node->left != NULL)
        NodeSaveToFile (node->left, file);
    else
        fprintf (file, "%s", "nil");

    if (node->right != NULL)
        NodeSaveToFile (node->right, file);
    else
        fprintf (file, "%s", "nil");

    fprintf (file, "%s", ")");

    return TREE_OK;
}

int TreeLoadFromFile (differentiator_t *diff, tree_t *tree,
                     const char *fileName, char **buffer, size_t *bufferLen)
{
    assert (diff);
    assert (tree);
    assert (fileName);
    assert (buffer);
    assert (bufferLen);

    DEBUG_PRINT ("\n========== LOADING TREE FROM \"%s\" ==========\n", fileName);

    if (tree->root != NULL)
    {
        ERROR_LOG ("%s", "TREE_ERROR_LOAD_INTO_NOT_EMPTY");
        
        return TREE_ERROR_LOAD_INTO_NOT_EMPTY;
    }
    
    *buffer = ReadFile (fileName, bufferLen);
    if (buffer == NULL)
        return TREE_ERROR_COMMON |
               COMMON_ERROR_READING_FILE;

    char *curPos = *buffer;
    
    int status = TreeLoadNode (diff, &tree->root, *buffer, &curPos);

    if (status != TREE_OK)
    {
        ERROR_LOG ("%s", "Error in TreeLoadNode()");

        return TREE_ERROR_SAVE_FILE_SYNTAX;
    }

    TREE_DUMP (diff, tree, "%s", "After load");
    
    DEBUG_PRINT ("%s", "==========    END OF LOADING TREE    ==========\n\n");
    return TREE_OK;
}

// FIXME: buffer overflow
// file must begin with '(', I don't see any problem with it
// TODO: move to tree_calc.cpp
int TreeLoadNode (differentiator_t *diff, node_t **node,
                  char *buffer, char **curPos)
{
    assert (diff);
    assert (node);
    assert (buffer);
    assert (curPos);
    assert (*curPos);

    tree_t *tree = &diff->expression;

    if (**curPos == '(')
    {
        NODE_CTOR (tree, *node);

        int status = TreeLoadNodeAndFill (diff, node, buffer, curPos);

        return status;
    }
    else if (strncmp (*curPos, "nil", sizeof("nil") - 1) == 0)
    {
        *curPos += sizeof ("nil") - 1;

        *node = NULL;

        return TREE_OK;
    }
    else 
    {
        ERROR_LOG ("%s", "Syntax error in tree dump file - uknown beginning of the node");
        ERROR_LOG ("curPos = \'%s\';", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }
}

int TreeLoadNodeAndFill (differentiator_t *diff, node_t **node,
                         char *buffer, char **curPos)
{
    assert (diff);
    assert (node);
    assert (buffer);
    assert (curPos);
    assert (*curPos);

    DEBUG_PRINT ("%s", "\n===== CREATING NEW NODE =====\n");
    DEBUG_VAR ("%s", *curPos);

    (*curPos)++; // move after '('
    *curPos = SkipSpaces (*curPos);
    
    int readBytes = 0;
    type_t type = TYPE_UKNOWN;
    treeDataType value = {};

    // FIXME: rewrite this part
    if (sscanf (*curPos, "%lf%n", &value.number, &readBytes) == 1)
    {
        type = TYPE_CONST_NUM;
        
        DEBUG_LOG ("number %g detected", value.number);
    }
    else
    {
        sscanf (*curPos, "%*s%n", &readBytes);
        
        TryToFindOperator (*curPos, readBytes, &type, &value);

        if (type == TYPE_UKNOWN)
        {
            int status = FindOrAddVariable (diff, curPos, readBytes, &type, &value);
            if (status != TREE_OK)
                return status;
        }
    }

    DEBUG_LOG ("%s", "After detecting type:");
    DEBUG_VAR ("%s", *curPos);
    DEBUG_LOG ("readBytes = %d", readBytes);
    DEBUG_LOG ("type = %d", type);
    DEBUG_LOG ("value.idx    = %d", value.idx);
    DEBUG_LOG ("value.number = %g", value.number);
    
    NodeFill (*node, type, value, NULL, NULL);
    
    *curPos += readBytes;
    *curPos += 1; // because it can be '\0', not space
    DEBUG_VAR ("%s", *curPos);
    *curPos = SkipSpaces (*curPos);
    
    // DEBUG_VAR ("%s", data);
    NODE_DUMP (diff, *node, "Created new node. curPos = \'%s\'", *curPos);

    int status = TreeLoadChildNodes (diff, node, buffer, curPos);
    if (status != TREE_OK)
        return status;
    
    if (**curPos != ')')
    {
        ERROR_LOG ("%s", "Syntax error in tree dump file - missing closing bracket ')'");
        ERROR_LOG ("curPos = \'%s\';", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    (*curPos)++;

    return TREE_OK;
}

int TreeLoadChildNodes (differentiator_t *diff, node_t **node, // FIXME child node
                        char *buffer, char **curPos)
{
    assert (diff);
    assert (node);
    assert (*node);
    assert (buffer);

    int status = TreeLoadNode (diff, &(*node)->left, buffer, curPos);
    if (status != TREE_OK)
        return status;

    if ((*node)->left != NULL)
    {
        NODE_DUMP (diff, (*node)->left, "After creating left subtree. \n"
                                        "curPos = \'%s\'", *curPos);
    }
    
    status = TreeLoadNode (diff, &(*node)->right, buffer, curPos);
    if (status != TREE_OK)
        return status;

    if ((*node)->right != NULL)
    {
        NODE_DUMP (diff, (*node)->right, "After creating right subtree. \n"
                                         "curPos = \'%s\'", *curPos);
    }
    
    return TREE_OK;
}

const char * GetValueTypeName (type_t type)
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

    for (size_t i = 0; i < operatorNumber; i++)
    {
        if (strncmp (str, operators[i].name, (size_t) len) == 0)
        {
            *type = TYPE_MATH_OPERATION;
            value->idx = operators[i].idx;

            DEBUG_LOG ("FOUND \"%s\"", operators[i].name);
        }
    }
}


variable_t *FindVariableByName (differentiator_t *diff, char *varName)
{
    assert (diff);
    assert (varName);

    for (size_t i = 0; i < diff->variablesSize; i++)
    {
        DEBUG_VAR ("%lu", i);
        DEBUG_VAR ("%p", diff->variables[i].name);
        DEBUG_VAR ("%s", diff->variables[i].name);
        
        if (strcmp (diff->variables[i].name, varName) == 0)
        {
            return &diff->variables[i];
        }
    }

    return NULL;
}

int FindOrAddVariable (differentiator_t *diff, char **curPos, int len, type_t *type, treeDataType *value)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (type);
    assert (value);

    TREE_DO_AND_RETURN ( CheckForReallocVariables (diff));

    *type = TYPE_VARIABLE;
    char *varName = *curPos;
    (*curPos)[len] = '\0';

    variable_t *variable = FindVariableByName (diff, varName);

    if (variable == NULL)
    {
        // NOTE: New function - add new variable ?
        value->idx = int (diff->variablesSize);

        diff->variables[diff->variablesSize].idx = diff->variablesSize;

        DEBUG_VAR ("%p", varName);
        diff->variables[diff->variablesSize].name = varName;
        DEBUG_VAR ("%s", diff->variables[diff->variablesSize].name);

        diff->variablesSize++;
    }
    else
    {
        value->idx = (int) variable->idx;
    }

    DEBUG_VAR ("%lu", diff->variablesSize);
    DEBUG_LOG ("variable name is '%s'", varName);
    DEBUG_LOG ("(*value).idx = '%d'", (*value).idx);

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
        
        variable_t *newVariables = (variable_t *)realloc (diff->variables, diff->variablesCapacity);
        if (newVariables == NULL)
        {
            ERROR_LOG ("Error reallocating memory for diff->variables - %s", strerror (errno));

            return TREE_ERROR_COMMON |
                   COMMON_ERROR_ALLOCATING_MEMORY;
        }

        diff->variables = newVariables;

        DEBUG_LOG ("new diff->variablesCapacity = %lu", diff->variablesCapacity);
    }

    return TREE_OK;
}

// =============  CALCULATION   =============

int TreeCalculate (differentiator_t *diff, tree_t *expression)
{
    assert (diff);
    assert (expression);

    DEBUG_VAR ("%lu", diff->variablesSize);

    double *values = (double *) calloc (diff->variablesSize, sizeof (double));
    if (values == NULL)
    {
        ERROR_LOG ("Error allocating memory for values - %s",
                    strerror (errno));

        return TREE_ERROR_COMMON |
               COMMON_ERROR_ALLOCATING_MEMORY;
    }

    for (size_t i = 0; i < diff->variablesSize; i++)
        values[i] = NAN;
    
    double result = NodeCalculate (diff, expression->root, values);

    PRINT ("\n");
    PRINT ("Expression is equals to %g\n", result);

    free (values);

    return TREE_OK;
}

double NodeCalculate (differentiator_t *diff, node_t *node, double *values)
{
    assert (node);

    double leftVal  = NAN;
    double rightVal = NAN;

    if (node->left != NULL)
        leftVal = NodeCalculate (diff, node->left, values);

    if (node->right != NULL)
        rightVal = NodeCalculate (diff, node->right, values);

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
            return NodeGetVariable (diff, node, values);
    
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

double NodeGetVariable (differentiator_t *diff, node_t *node, double *values)
{
    assert (diff);
    assert (node);
    assert (values);

    DEBUG_VAR ("%d", node->value.idx);

    if (isnan (values[node->value.idx]))
    {
        AskVariableValue (diff, node, values);
    }
    
    return values[node->value.idx];
}

void AskVariableValue (differentiator_t *diff, node_t *node, double *values)
{
    assert (diff);

    int idx = node->value.idx;

    DEBUG_VAR ("%d", idx);

    int status   = 0;
    int attempts = 0;

    while (attempts < 5 && status != 1)
    {
        if (attempts >= 1)
        {
            ERROR_PRINT ("%s", "This is not a float point number. Please, try again");
        }

        PRINT ("Input value of variable '%s'\n"
               " > ", 
               diff->variables[idx].name);

        status = scanf ("%lg", &values[idx]);
        ClearBuffer ();

        attempts++;
    }

    if (attempts >= 5)
    {
        values[idx] = 1.0;
        PRINT ("I am tired.\n"
               "Value of '%s' was set to %g.\n"
               "Think about your behavior", 
               diff->variables[idx].name, values[idx]);
    }
}

// ============= SIMPLIFICATION =============

#define NUM_(num)                                                               \
        NodeCtorAndFill (tree, TYPE_CONST_NUM, {.number = num}, NULL, NULL);

void TreeSimplify (tree_t *tree)
{
    assert (tree);

    tree->root = NodeSimplifyCalc (tree, tree->root);
}

node_t *NodeSimplifyCalc (tree_t *tree, node_t *node)
{
    assert (tree);
    assert (node);

    if (node->left != NULL)
        node->left = NodeSimplifyCalc (tree, node->left);

    if (node->right != NULL)
        node->right = NodeSimplifyCalc (tree, node->right);
    
    if (node->right == NULL) 
        return node;

    double leftValue  = NAN;
    double rightValue = NAN;

    if (node->left != NULL)
        leftValue = node->left->value.number;
    if (node->right != NULL)
        rightValue = node->right->value.number;

    node_t *newNode = NULL;

    if (node->right->type == TYPE_CONST_NUM && 
        (node->left == NULL || node->left->type  == TYPE_CONST_NUM))
    {
        switch (node->value.idx)
        {
            case OP_UKNOWN:
                ERROR_LOG ("%s", "Uknown math operation");

                return NULL;

            case OP_ADD: newNode = NUM_ (leftValue + rightValue);   break;
            case OP_SUB: newNode = NUM_ (leftValue - rightValue);   break;
            case OP_MUL: newNode = NUM_ (leftValue * rightValue);   break;
            case OP_DIV: newNode = NUM_ (leftValue / rightValue);   break;
// NOTE: excepted argument in radians
            case OP_SIN: newNode = NUM_ (sin (rightValue));         break;
            case OP_COS: newNode = NUM_ (cos (rightValue));         break;
            
            default:
                break;
        }
    }
    
    if (newNode == NULL) 
        return node;

    TreeDelete (tree, &node);

    return newNode;
}

// node_t *NodeSimplifyStupid (tree_t *tree, node_t *node)
// {
//     assert (tree);
//     assert (node);

//     if (node->left != NULL)
//         node->left = NodeSimplifyCalc (tree, node->left);

//     if (node->right != NULL)
//         node->right = NodeSimplifyCalc (tree, node->right);
    
//     if (node->right == NULL) return node;

//     double leftValue  = node->left ->value.number;
//     double rightValue = node->right->value.number;

//     if (node->left->type == TYPE_CONST_NUM && 
//         node->left ->value.number == 1)

//     return newNode;
// }

#undef NUM_

// ============= DIFFERENTATION =============

#define cN NodeCopy (expression,         resTree)
#define cL NodeCopy (expression->left,   resTree)
#define cR NodeCopy (expression->right,  resTree)

#define dL NodeDiff (diff, expression->left,  resTree, argument)
#define dR NodeDiff (diff, expression->right, resTree, argument)

#define NUM_(num)                                                                   \
        NodeCtorAndFill (resTree, TYPE_CONST_NUM, {.number = num}, NULL, NULL)
#define ADD_(left, right)                                                           \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_ADD},             \
                         left, right)
#define SUB_(left, right)                                                           \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_SUB},             \
                         left, right)
#define MUL_(left, right)                                                           \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_MUL},             \
                         left, right)
#define DIV_(left, right)                                                           \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_DIV},             \
                         left, right)
#define POW_(left, right)                                                           \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_POW},             \
                         left, right)
#define LN_(right)                                                                  \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_LN},              \
                         NULL, right)
#define SIN_(right)                                                                 \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_SIN},             \
                         NULL, right)
#define COS_(right)                                                                 \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_COS},             \
                         NULL, right)


int TreesDiff (differentiator_t *diff, tree_t *expression)
{
    assert (diff);
    assert (expression);

    DEBUG_PRINT ("%s", "\n==========  START OF DIFFERENTATION  ==========\n");

    size_t diffTimes = 0;
    variable_t *var = 0;

    TREE_DO_AND_RETURN (AskUserAboutDifferentation (diff, &diffTimes, &var));


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


    DumpLatex (diff, expression->root, "Найдём производную следующей функции:");


    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        DEBUG_VAR ("%lu", i);

        tree_t *tree = &diff->diffTrees[i];
        if (i == 0)
            tree->root = NodeDiff (diff, expression->root, tree, var);
        else
            tree->root = NodeDiff (diff, diff->diffTrees[i - 1].root, tree, var);

        if (tree->root == NULL)
            return TREE_ERROR_NULL_ROOT;

        TREE_DUMP (diff, tree, "first devirative tree by '%s'", var->name);

        TreeSimplify (tree);
        TREE_DUMP (diff, tree, "%s", "Simplified firstDerivative");

        DumpLatexBoxed (diff, tree->root, "Ответ: ");
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


    PRINT ("By which variable should program differentiate the expression?\n"
           " > ");

    char *varName = NULL;
    size_t varNameLen = 0;
    status = SafeReadLine (&varName, &varNameLen);

    if (status != COMMON_ERROR_OK)
        return TREE_ERROR_COMMON |
               status;

    *var = FindVariableByName (diff, varName);

    if (var == NULL)
    {
        *var = &diff->variables[0];

        ERROR_PRINT ("There is no such variable\n"
                     "I will differentiate expression by '%s'",
                     (*var)->name);
    }

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

    DEBUG_VAR ("%d", expression->value.idx);
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

            // NOTE: does I need this ?
            // or maybe do verificator specially for calc tree
            // or add this to define CHECK

            // if (!HasBothChildren (expression))
            // {
            //     ERROR_LOG ("Addition node [%p] doesn't have both children nodes", expression);
                
            //     return NULL;
            // }
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
        

        default:
            assert (0 && "There is not implemented differentation for function");
    }
}

#undef cN
#undef cL
#undef cR
#undef dL
#undef dR
#undef NUM_
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
#undef POW_
#undef LN_
#undef SIN_
#undef COS_

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