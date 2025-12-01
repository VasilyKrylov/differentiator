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
#define SH_(right)                                                                 \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_SH},             \
                         NULL, right)
#define CH_(right)                                                                 \
        NodeCtorAndFill (resTree, TYPE_MATH_OPERATION, {.idx = OP_CH},             \
                         NULL, right)
