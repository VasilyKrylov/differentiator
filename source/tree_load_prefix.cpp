#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tree_load_prefix.h"

#include "tree.h"
#include "tree_calc.h"
#include "utils.h"

static int TreeLoadNode                 (differentiator_t *diff, node_t **node,
                                         char *buffer, char **curPos);
static int TreeLoadNodeAndFill          (differentiator_t *diff, node_t **node,
                                         char *buffer, char **curPos);
static int TreeLoadChildNodes           (differentiator_t *diff, node_t **node,
                                         char *buffer, char **curPos);

int TreeLoadPrefixFromFile (differentiator_t *diff, tree_t *tree,
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

        return TREE_ERROR_SYNTAX_IN_SAVE_FILE;
    }

    TREE_DUMP (diff, tree, "%s", "After load");
    
    DEBUG_PRINT ("%s", "==========    END OF LOADING TREE    ==========\n\n");
    return TREE_OK;
}

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
            int status = FindOrAddVariable (diff, curPos, (size_t) readBytes, &type, &value);
            if (status != TREE_OK)
                return status;
        }
    }

    DEBUG_LOG ("%s", "After detecting type:");
    DEBUG_VAR ("%s", *curPos);
    DEBUG_LOG ("readBytes = %d", readBytes);
    DEBUG_LOG ("type = %d", type);
    DEBUG_LOG ("value.idx    = %lu", value.idx);
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

int TreeLoadChildNodes (differentiator_t *diff, node_t **node,
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

