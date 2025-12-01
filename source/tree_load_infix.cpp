#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "tree_load_infix.h"

#include "tree.h"
#include "tree_calc.h"
#include "utils.h"

// if we believe https://en.wikipedia.org/wiki/Parsing_expression_grammar,
// ? means optional

/*
Examples+ = {"log ( 3 + x * 2) -    (7+ 3)*x ^ 2"}

Gramma      ::= Expression Spaces '\0'
    *curPos = SkipSpaces (*curPos);

Expression  ::= Term        {['+', '-']Term}*
Term        ::= Power       {['/', '*']Power}*
// FIXME: power
Power       ::= PrimaryExp  {'^'PrimaryExp}* 
PrimaryExp  ::= '('Expression')' | Number | Function | Variable

Number      ::= ['0'-'9']+{'.'['0'-'9']+}?
Function    ::= ["sin", "cos", ...]'('Expression')' | 
                ["log"]'('Expression','Expression')'
Variable    ::= ['a'-'z', 'A'-'Z', '_']'a'-'z', 'A'-'Z', '0'-'9', '_']*

*/

/*


Examples+ = {"log ( 3 + x * 2) -    (7+ 3)*x ^ 2"}

Gramma      ::= Expression'\0'

Spaces      ::= {' '}*

Expression  ::= Term        {['+', '-'] Spaces Term}* 
Term        ::= Power       {['/', '*'] Spaces Power}*
// FIXME: power
Power       ::= PrimaryExp  {'^' Spaces PrimaryExp}* 
PrimaryExp  ::= Spaces {'('  Expression ')' | Number | Function | Variable} Spaces

Number      ::= ['0'-'9']+{'.'['0'-'9']+}?
Function    ::= ["sin", "cos", ...] Spaces '(' Expression ')' | 
                ["log"] Spaces '(' Expression ',' Expression')'
Variable    ::= ['a'-'z', 'A'-'Z', '_']['a'-'z', 'A'-'Z', '0'-'9', '_']*
/
*/

#define SYNTAX_ERROR                                                    \
        {                                                               \
            ERROR_LOG ("Syntax Error on position \"%s\"", *curPos);     \
                                                                        \
            return TREE_ERROR_SYNTAX_IN_SAVE_FILE;                      \
        }
        
static int GetGramma            (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);
static int GetExpression        (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);
static int GetTerm              (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);
static int GetPower             (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);
static int GetPrimaryExpression (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);
static int GetVariable          (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);

static int GetFunction          (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);
static int GetVariableName      (char **curPos);
static int GetNumber            (differentiator_t *diff, char **curPos, 
                                 tree_t *resTree, node_t **node);

int TreeLoadInfixFromFile (differentiator_t *diff, tree_t *tree,
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
    
    // int status = GetGramma (diff, &tree->root, *buffer, &curPos);
    int status = GetGramma (diff, &curPos, tree, &tree->root);

    if (status != TREE_OK)
    {
        ERROR_LOG ("%s", "Error in GetGramma()");

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    TREE_DUMP (diff, tree, "%s", "After load");
    
    DEBUG_PRINT ("%s", "==========    END OF LOADING TREE    ==========\n\n");

    return TREE_OK;
}

int GetGramma (differentiator_t *diff, char **curPos, tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    int status = GetExpression (diff, curPos, resTree, node);

    NODE_DUMP (diff, *node, "Created new node. curPos = \'%s\'", *curPos);

    if (status != TREE_OK)
        SYNTAX_ERROR;

    *curPos = SkipSpaces (*curPos);

    if (**curPos != '\0')
        SYNTAX_ERROR;

    return TREE_OK;
}

#include "dsl_define.h"

int GetExpression (differentiator_t *diff, char **curPos, tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_STR (*curPos);

    int status = GetTerm (diff, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (**curPos == '+' ||
           **curPos == '-')
    {
        char operation = **curPos;
        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        node_t *node2 = {};
        status = GetTerm (diff, curPos, resTree, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        switch (operation)
        {
            case '+': *node = ADD_ (*node, node2); break;
            case '-': *node = SUB_ (*node, node2); break;

            default: SYNTAX_ERROR;
        }
    }

    NODE_DUMP (diff, *node, "Created new node (expression). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetTerm (differentiator_t *diff, char **curPos, tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    int status = GetPower (diff, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (**curPos == '*' ||
           **curPos == '/')
    {
        char operation = **curPos;
        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        node_t *node2 = {};
        status = GetPower (diff, curPos, resTree, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        switch (operation)
        {
            case '*': *node = MUL_ (*node, node2); break;
            case '/': *node = DIV_ (*node, node2); break;

            default: SYNTAX_ERROR;
        }
    }

    DEBUG_LOG ("node [%p]",  (*node));
    DEBUG_LOG ("\tnode->left [%p]",  (*node)->left);
    DEBUG_LOG ("\tnode->right [%p]", (*node)->right);

    NODE_DUMP (diff, *node, "Created new node (term). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetPower (differentiator_t *diff, char **curPos, 
              tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    int status = GetPrimaryExpression (diff, curPos, resTree, node);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    while (**curPos == '^')
    {
        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        node_t *node2 = {};
        status = GetPrimaryExpression (diff, curPos, resTree, &node2);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        *node = POW_ (*node, node2);
    }

    return TREE_OK;
}

int GetPrimaryExpression (differentiator_t *diff, char **curPos, 
                          tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_STR (*curPos);

    *curPos = SkipSpaces (*curPos);

    if (**curPos == '(')
    {
        (*curPos)++;
        
        int status = GetExpression (diff, curPos, resTree, node);
        if (status != TREE_OK)
            SYNTAX_ERROR;

        if (**curPos != ')')
            SYNTAX_ERROR;

        (*curPos)++;

        *curPos = SkipSpaces (*curPos);

        return status;
    }

    int status = GetNumber (diff, curPos, resTree, node);
    if (status == TREE_OK)
    {
        *curPos = SkipSpaces (*curPos);

        return status;
    }

    status = GetFunction (diff, curPos, resTree, node);
    if (status == TREE_OK)
    {
        *curPos = SkipSpaces (*curPos);
        
        return status;
    }
    
    status = GetVariable (diff, curPos, resTree, node);
    DEBUG_LOG ("GetVariable() status = %d", status);
    if (status == TREE_OK)
    {
        *curPos = SkipSpaces (*curPos);
        
        return status;
    }

    DEBUG_STR (*curPos);

    SYNTAX_ERROR;
}

int GetNumber (differentiator_t *diff, char **curPos, tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    double val = NAN;
    int readBytes = 0;

    int res = sscanf (*curPos, "%lf%n", &val, &readBytes);
    DEBUG_VAR ("%d", res);
    if (res != 1)
        return TREE_ERROR_NODE_NOT_FOUND;

    *curPos += readBytes;
    DEBUG_VAR ("%s", *curPos);
    DEBUG_VAR ("%g", val);

    *node = NUM_ (val);

    NODE_DUMP (diff, *node, "Created new node (number). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetFunction (differentiator_t *diff, char **curPos, tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_STR (*curPos);

    const keyword_t *func = NULL;

    for (size_t i = 0; i < kNumberOfKeywords; i++)
    {
        if (strncmp (*curPos, keywords[i].name, keywords[i].nameLen) == 0 &&
            keywords[i].isFunction)
        {
            *node = NodeCtorAndFill (resTree, 
                                     TYPE_MATH_OPERATION, 
                                     {.idx = (size_t) keywords[i].idx}, 
                                     NULL, NULL);

            NODE_DUMP (diff, *node, "Created new node (function). curPos = \'%s\'", *curPos);
                                     
            *curPos += keywords[i].nameLen;
            func = &keywords[i];

            break;
        }
    }

    if (func == NULL)
    {
        DEBUG_LOG ("%s", "No function found. Return");

        return TREE_ERROR_INVALID_NODE;
    }

    if (func->numberOfArgs < 1 ||
        func->numberOfArgs > 2)
    {
        ERROR_LOG ("%s", "Where are functions with only 1 or 2 args now...\n"
                         "Maybe you forgot to rewrite this part of code?");

        return TREE_ERROR_INVALID_NODE;
    }

    *curPos = SkipSpaces (*curPos);

    DEBUG_STR (*curPos);

    if (**curPos != '(')
        SYNTAX_ERROR;
    
    (*curPos)++;

    node_t *firstArg = {};
    int status = GetExpression (diff, curPos, resTree, &firstArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    DEBUG_STR (*curPos);

    if (func->numberOfArgs == 1)
    {
        if (**curPos != ')')
            SYNTAX_ERROR;
        
        (*curPos)++;
        (*node)->right = firstArg;

        return TREE_OK;
    }
    
    if (**curPos != ',')
        SYNTAX_ERROR;
    
    (*curPos)++;

    DEBUG_LOG ("*curPos = \"%s\" (after ',') ", *curPos);

    node_t *secondArg = {};
    status = GetExpression (diff, curPos, resTree, &secondArg);
    if (status != TREE_OK)
        SYNTAX_ERROR;

    if (**curPos != ')')
        SYNTAX_ERROR;

    (*curPos)++;
    DEBUG_STR (*curPos);

    (*node)->left  = firstArg;
    (*node)->right = secondArg;

    return TREE_OK;
}

int GetVariable (differentiator_t *diff, char **curPos, tree_t *resTree, node_t **node)
{
    assert (diff);
    assert (curPos);
    assert (*curPos);
    assert (resTree);
    assert (node);

    DEBUG_VAR ("%s", *curPos);

    char *varName = *curPos;
    DEBUG_VAR ("%p", varName);

    int res = GetVariableName (curPos);
    DEBUG_VAR ("%p", *curPos);

    if (res != TREE_OK)
    {
        DEBUG_LOG ("'%s' not a variable", *curPos);

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    type_t type   = {};
    value_t value = {};
    
    size_t varNameLen = size_t (*curPos - varName);
    DEBUG_VAR ("%lu", varNameLen);

    int status = FindOrAddVariable (diff, &varName, varNameLen, &type, &value);
    if (status != TREE_OK)
        return status;

    *node = NodeCtorAndFill (resTree, type, value, NULL, NULL);

    DEBUG_LOG ("(after adding variable) *curPos = \"%s\"", *curPos);

    NODE_DUMP (diff, *node, "Created new node (number). curPos = \'%s\'", *curPos);

    return TREE_OK;
}

int GetVariableName (char **curPos)
{
    assert (curPos);

    DEBUG_VAR ("%s", *curPos);

    DEBUG_LOG ("**curPos(variable name) = '%c'", **curPos);

    if (!isalpha (**curPos) && **curPos != '_')
        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    
    (*curPos)++;

    DEBUG_VAR ("%p", *curPos);

    while (isalpha (**curPos) || isdigit (**curPos) || **curPos == '_')
    {
        DEBUG_LOG ("**curPos(variable name) = \"%c\"", **curPos);
        DEBUG_VAR ("%p", *curPos);
        
        (*curPos)++;    
    }
    
    return TREE_OK;
}

#include "dsl_undef.h"

#undef SYNTAX_ERROR