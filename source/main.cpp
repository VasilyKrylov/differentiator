#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "tree.h"
#include "tree_calc.h"
#include "tree_log.h"

int main()
{
    differentiator_t diff = {};
    TREE_DO_AND_CLEAR (DifferentiatorCtor (&diff, 4),
                       DifferentiatorDtor (&diff));

    TREE_DO_AND_CLEAR (TreeCalculate (&diff, &diff.expression),
                       DifferentiatorDtor (&diff));

    if (diff.variablesSize > 0)
    {
        TREE_DO_AND_CLEAR (TreesDiff (&diff, &diff.expression),
                           DifferentiatorDtor (&diff));
    }

    DifferentiatorDtor (&diff);
}
