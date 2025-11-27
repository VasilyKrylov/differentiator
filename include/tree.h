#ifndef K_TREE_H
#define K_TREE_H

#include <stdio.h>

#include "tree_log.h"
#include "debug.h"

typedef union value_t treeDataType;
const char * const treeDataTypeStr = "union value_t";

const char treeSaveFileName[]       = "tree.txt";

#define TREE_DO_AND_RETURN(action)          \
        do                                  \
        {                                   \
            int status = action;            \
            DEBUG_VAR("%d", status);        \
                                            \
            if (status != TREE_OK)          \
                return status;              \
        } while (0)                       

#define TREE_DO_AND_CLEAR(action, clearAction)      \
        do                                          \
        {                                           \
            int status = action;                    \
            DEBUG_VAR("%d", status);                \
                                                    \
            if (status != TREE_OK)                  \
            {                                       \
                clearAction;                        \
                                                    \
                return status;                      \
            }                                       \
        } while (0)

#define NODE_CTOR(tree, node)                       \
        node = NodeCtor (tree);                     \
        if (node == NULL)                           \
            return TREE_ERROR_CREATING_NODE


#ifdef PRINT_DEBUG

struct varInfo_t
{
    const char *name = NULL;
    const char *file = NULL;
    int line         = 0;
    const char *func = NULL;
};
// NOTE Change name to VarInfoTree_t (?)
#define TREE_CTOR(treeName, log)                    \
        TreeCtor (treeName, log,                    \
                 varInfo_t{.name = #treeName,       \
                           .file = __FILE__,        \
                           .line = __LINE__,        \
                           .func = __func__})

#define TREE_DUMP(diff, treeName, format, ...)      \
        TreeDump (diff, treeName,                   \
                  __FILE__, __LINE__, __func__,     \
                  format, __VA_ARGS__)

#define NODE_DUMP(diff, nodeName, format, ...)      \
        NodeDump (diff, nodeName,                   \
                  __FILE__, __LINE__, __func__,     \
                  format, __VA_ARGS__)

#define TREE_VERIFY(tree) TreeVerify (tree) 
#else

#define TREE_CTOR(treeName, log) TreeCtor (treeName, log) 
#define TREE_VERIFY(tree) TREE_OK;
#define TREE_DUMP(diff, tree, format, ...) 
#define NODE_DUMP(diff, node, format, ...) 

#endif // PRING_DEBUG

enum type_t // NOTE: maybe move to tree_calc.h
{
    TYPE_UKNOWN         = 0,
    TYPE_CONST_NUM      = 1,
    TYPE_MATH_OPERATION = 2,
    TYPE_VARIABLE       = 3
}; 

union value_t;
struct node_t;

struct tree_t
{
    node_t *root = NULL;

    size_t size = 0;

    treeLog_t *log = NULL;

#ifdef PRINT_DEBUG
    varInfo_t varInfo = {};
#endif
};

enum treeError_t
{
    TREE_OK                             = 0,
    TREE_ERROR_NULL_STRUCT              = 1 << 0,
    TREE_ERROR_NULL_ROOT                = 1 << 1,
    TREE_ERROR_NULL_DATA                = 1 << 2, // NOTE: remove(?)
    TREE_ERROR_NOT_ENOUGH_NODES         = 1 << 3,
    TREE_ERROR_TO_MUCH_NODES            = 1 << 4,
    TREE_ERROR_INVALID_NEW_QUESTION     = 1 << 5, // FIXME: move to akinator (?)
    TREE_ERROR_SAVE_FILE_SYNTAX         = 1 << 6,
    TREE_ERROR_LOAD_INTO_NOT_EMPTY      = 1 << 7,
    TREE_ERROR_INVALID_NODE             = 1 << 8,
    TREE_ERROR_INVALID_PATH             = 1 << 9, // bad value on stackNodePath
    TREE_ERROR_CREATING_NODE            = 1 << 10,
    TREE_ERROR_SYNTAX_IN_SAVE_FILE      = 1 << 11,

    TREE_NODE_FOUND                     = 1 << 12,
    TREE_NODE_NOT_FOUND                 = 1 << 13,

    TREE_ERROR_COMMON                   = 1 << 31
};

int TreeCtor            (tree_t *tree, treeLog_t *log
                         ON_DEBUG (, varInfo_t varInfo));
node_t *NodeCtor        (tree_t *tree);
void NodeFill           (node_t *node, type_t type, treeDataType value, 
                         node_t *leftChild, node_t *rightChild);
node_t *NodeCtorAndFill (tree_t *tree,
                         type_t type, treeDataType value, 
                         node_t *leftChild, node_t *rightChild);
void TreeDelete         (tree_t *tree, node_t **node);
void TreeDtor           (tree_t *tree);
void TreeCopy           (tree_t *source, tree_t *dest);
node_t *NodeCopy        (node_t *source, tree_t *tree);
int TreeVerify          (tree_t *tree);
bool IsLeaf             (node_t *node);
bool HasBothChildren    (node_t *node);
bool HasOneChild        (node_t *node);

#endif //K_TREE_H