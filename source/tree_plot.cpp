#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "tree_plot.h"

#include "tree.h"
#include "tree_calc.h"

static int TreeCreatePlotImage (differentiator_t *diff, tree_t *tree, size_t fileNumber);
static int PlotGenerateData (differentiator_t *diff, tree_t *tree, const char *plotFilePath);
static int RunGnuPlot (differentiator_t *diff, const char *plotFilePath, size_t fileNumber);

int TreeCreatePlotImages (differentiator_t *diff)
{
    assert (diff);

    TREE_DO_AND_RETURN (TreeCreatePlotImage (diff, &diff->expression, 0));

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        TREE_DO_AND_RETURN (TreeCreatePlotImage (diff, &diff->diffTrees[i], i + 1));
    }

    return TREE_OK;
}


int TreeCreatePlotImage (differentiator_t *diff, tree_t *tree, size_t fileNumber)
{
    assert (diff);
    assert (tree);

    const size_t kPathMaxLen = kFileNameLen + 32;

    char plotFilePath[kPathMaxLen] = {};
    
    int status = snprintf (plotFilePath, kPathMaxLen, "%s%lu.txt", diff->log.plotFolderPath, fileNumber);
    if (status < 0)
    {
        ERROR_LOG ("Error in snprintf(), return code = %d", status);

        return TREE_ERROR_COMMON |
               COMMON_ERROR_SNPRINTF;
    }

    TREE_DO_AND_RETURN (PlotGenerateData (diff, tree, plotFilePath));

    TREE_DO_AND_RETURN (RunGnuPlot (diff, plotFilePath, fileNumber));

    return TREE_OK;
}

int PlotGenerateData (differentiator_t *diff, tree_t *tree, const char *plotFilePath)
{
    assert (diff);
    assert (tree);
    assert (plotFilePath);
    
    FILE *plotFile = fopen (plotFilePath, "w");
    if (plotFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", plotFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (plotFile, "%s", "# x \t y\n");

    double saveValue = diff->varToDiff->value;
    for (double x = kLeftRange; x <= kRightRange; x += kStep)
    {
        diff->varToDiff->value = x;

        fprintf (plotFile, "%g \t %g\n", x, NodeCalculate (diff, tree->root));
    }
    diff->varToDiff->value = saveValue;

    fclose (plotFile);

    return TREE_OK;
}

int RunGnuPlot (differentiator_t *diff, const char *plotFilePath, size_t fileNumber)
{
    assert (diff);
    assert (plotFilePath);

    const size_t kPathMaxLen = kFileNameLen + 32;

    char pngFilePath[kPathMaxLen] = {};
    int status = snprintf (pngFilePath, kPathMaxLen, "%s%lu.png", diff->log.plotFolderPath, fileNumber);

    FILE *scriptFile = fopen (diff->log.plotScriptFilePath, "w");
    if (scriptFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", diff->log.plotScriptFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (scriptFile, kPlotScript, kLeftRange, kRightRange, pngFilePath, fileNumber, plotFilePath);
    fclose (scriptFile);

    const size_t kCommandLen = kFileNameLen + 32;
    char command[kCommandLen] = {};
    
    status = snprintf (command, kCommandLen, "gnuplot -c %s", diff->log.plotScriptFilePath);
    if (status < 0)
    {
        ERROR_LOG ("Error in snprintf(), return code = %d", status);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_SNPRINTF;
    }

    DEBUG_STR (command);

    status = system (command);
    if (status != 0)
    {
        ERROR_LOG ("ERROR executing command \"%s\"", command);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_RUNNING_SYSTEM_COMMAND;
    }

    return TREE_OK;
}