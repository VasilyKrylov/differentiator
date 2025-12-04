#define cN NodeCopy (expression,         tree)
#define cL NodeCopy (expression->left,   tree)
#define cR NodeCopy (expression->right,  tree)

#define dL NodeDiff (diff, expression->left,  tree, argument)
#define dR NodeDiff (diff, expression->right, tree, argument)

#define NUM_(num) NodeCtorAndFill (tree, TYPE_CONST_NUM, {.number = num}, NULL, NULL)

#define MATH_OP_(nodeOperation, left, right)                                             \
        NodeCtorAndFill (tree, TYPE_MATH_OPERATION, {.idx = nodeOperation}, left, right)

#define ADD_(left, right)               \
        MATH_OP_ (OP_ADD, left, right)

#define SUB_(left, right)               \
        MATH_OP_ (OP_SUB, left, right)

#define MUL_(left, right)               \
        MATH_OP_ (OP_MUL, left, right)

#define DIV_(left, right)               \
        MATH_OP_ (OP_DIV, left, right)

#define POW_(left, right)               \
        MATH_OP_ (OP_POW, left, right)

#define LN_(right)                      \
        MATH_OP_ (OP_LN, NULL, right)

#define SIN_(right)                     \
        MATH_OP_ (OP_SIN, NULL, right)

#define COS_(right)                     \
        MATH_OP_ (OP_COS, NULL, right)

#define SH_(right)                      \
        MATH_OP_ (OP_SH, NULL, right)

#define CH_(right)                      \
        MATH_OP_ (OP_CH, NULL, right)
