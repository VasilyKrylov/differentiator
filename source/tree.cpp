#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#include "debug.h"
#include "tree_log.h"
#include "utils.h"
#include "float_math.h"

#include "tree.h"
#include "tree_calc.h"

static int TreeCountNodes       (node_t *node, size_t size, size_t *nodesCount);


// maybe: pass varInfo here for ERROR_LOG
node_t *NodeCtor (tree_t *tree)
{
    assert (tree);

    node_t *node = (node_t *) calloc (1, sizeof(node_t));
    if (node == NULL)
    {
        ERROR_LOG ("Error allocating memory for new node - %s", strerror (errno));

        return NULL;
    }

    DEBUG_LOG ("tree->size = %lu", tree->size);
    tree->size += 1;
    DEBUG_LOG ("tree->size = %lu", tree->size);

    node->type          = TYPE_UKNOWN;
    node->value.number  = 0;
    node->left          = NULL;
    node->right         = NULL;

    return node;
}

// user may create new node without left and right child
// so they can be NULL
void NodeFill (node_t *node, type_t type, treeDataType value, 
               node_t *leftChild, node_t *rightChild)
{
    assert (node);
    // leftChild can be NULL

    node->type = type;
    
    if (type == TYPE_CONST_NUM)
        node->value.number = value.number;
    else if (type != TYPE_UKNOWN)
        node->value.idx = value.idx;

    node->left  = leftChild;
    node->right = rightChild;
}

node_t *NodeCtorAndFill (tree_t *tree,
                         type_t type, treeDataType value, 
                         node_t *leftChild, node_t *rightChild)
{
    assert (tree);
    // leftChild can be NULL

    DEBUG_PRINT ("%s", "\n========== NODE CTOR START ==========\n");

    node_t *node = (node_t *) calloc (1, sizeof(node_t));
    if (node == NULL)
    {
        ERROR_LOG ("Error allocating memory for new node - %s", strerror (errno));

        return NULL;
    }

    tree->size += 1;

    DEBUG_LOG ("tree->size = %lu", tree->size);
    DEBUG_LOG ("node [%p]", node);
    DEBUG_LOG ("\t type = %d", type);

    if (type == TYPE_CONST_NUM)
    {
        DEBUG_LOG ("\t value.number = %g", value.number);
    }
    else 
    {
        DEBUG_LOG ("\t value.idx = %lu", value.idx);
    }

    DEBUG_LOG ("\t left = [%p]", node->left);
    DEBUG_LOG ("\t right = [%p]", node->right);    

    node->type          = type;
    node->value         = value;
    node->left          = leftChild;
    node->right         = rightChild;

    DEBUG_PRINT ("%s", "========== NODE CTOR END ==========\n\n");

    return node;
}

int TreeCtor (tree_t *tree, treeLog_t *log
              ON_DEBUG (, varInfo_t varInfo))
{
    assert (tree);
    assert (log);

    tree->root = NULL;
    tree->size = 0;

    ON_DEBUG (
        tree->varInfo = varInfo;
    );

    // int status = LogCtor (&tree->log);
    // DEBUG_VAR ("%d", status);
    // if (status != TREE_OK)
    //     return status;

    tree->log = log;

    return TREE_OK;
}

void TreeDtor (tree_t *tree)
{
    assert (tree);

    TreeDelete (tree, &tree->root);
}

// TODO: 
void TreeDelete (tree_t *tree, node_t **node)
{
    assert (tree);
    assert (node);
    assert (*node);
    
    if ((*node)->left != NULL)
    {
        TreeDelete (tree, &(*node)->left);
    }
    if ((*node)->right != NULL)
    {
        TreeDelete (tree, &(*node)->right);
    }

    tree->size -= 1;
    
    DEBUG_VAR ("deleted [%p]", node);
    DEBUG_VAR ("tree->size = %lu", tree->size);
    
    free (*node);
    *node = NULL;
}

int TreeVerify (tree_t *tree)
{
    int error = TREE_OK;

    if (tree == NULL)       return TREE_ERROR_NULL_STRUCT;
    if (tree->root == NULL) return TREE_ERROR_NULL_ROOT;

    size_t nodesCount = 0;
    error |= TreeCountNodes (tree->root, tree->size, &nodesCount);

    if (nodesCount < tree->size) error |= TREE_ERROR_NOT_ENOUGH_NODES;
    if (nodesCount > tree->size) error |= TREE_ERROR_TO_MUCH_NODES;

    DEBUG_LOG ("nodesCount = %lu", nodesCount);
    DEBUG_LOG ("tree->size = %lu", tree->size);

    return error;
}

int TreeCountNodes (node_t *node, size_t size, size_t *nodesCount)
{
    int status = TREE_OK;
    
    *nodesCount += 1;

    if (*nodesCount > size)
        return status|
               TREE_ERROR_TO_MUCH_NODES;

    if (node->left != NULL)
        status |= TreeCountNodes (node->left, size, nodesCount);

    if (node->right != NULL)
        status |= TreeCountNodes (node->right, size, nodesCount);

    return status;
}

void TreeCopy (tree_t *source, tree_t *dest)
{
    assert (source);
    assert (dest);

    DEBUG_PRINT ("%s", "\n========== START OF COPYING TREE ==========\n");

    dest->root = NodeCopy (source->root, dest);

    DEBUG_VAR ("%p", dest->root);

    DEBUG_PRINT ("%s", "========== END OF COPYING TREE ==========\n\n");
}

node_t *NodeCopy (node_t *source, tree_t *tree)
{
    assert (source);
    assert (tree);

    node_t *dest = NodeCtorAndFill (tree, source->type, source->value, NULL, NULL);
    
    DEBUG_VAR ("%p", source);
    DEBUG_VAR ("%p", dest);

    if (source->left != NULL)
    {
        dest->left = NodeCopy (source->left, tree);
        if (dest->left == NULL)
            return NULL;
    }
        
    if (source->right != NULL)
    {
        dest->right = NodeCopy (source->right, tree);
        if (dest->right == NULL)
            return NULL;
    }

    return dest;
}

bool IsLeaf (node_t *node)
{
    assert (node);

    return node->left == NULL && node->right == NULL;
}

bool HasBothChildren (node_t *node)
{
    assert (node);

    return node->left != NULL && node->right != NULL;
}

bool HasOneChild (node_t *node)
{
    assert (node);

    return !IsLeaf (node) && !HasBothChildren (node);
}
