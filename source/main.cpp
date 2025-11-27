#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "tree.h"
#include "tree_calc.h"
#include "tree_log.h"

// А норм, что у меня верификатор нигде не используется?

int main()
{
    differentiator_t diff = {};
    DifferentiatorCtor (&diff, 3); // FIXME

    TreeCalculate (&diff, &diff.expression);

    TreesDiff (&diff, &diff.expression);

    DifferentiatorDtor (&diff);
}
