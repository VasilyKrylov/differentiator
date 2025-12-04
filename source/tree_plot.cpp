#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "tree_plot.h"

#include "tree.h"
#include "tree_calc.h"

static int PlotGenerateData             (differentiator_t *diff, tree_t *tree, 
                                         const char *plotFilePath);
static int RunGnuPlot                   (differentiator_t *diff, const char *plotFilePath, 
                                         const char * fileNumber);
static int RunGnuPlot2Functions         (differentiator_t *diff, const char *pngFileName, 
                                         const char *firstData, const char *secondData);

// NOTE: maybe snprintf macro
// or move all of this to tree_log
// int TreeCreatePlotImages (differentiator_t *diff)
// {
//     assert (diff);

//     TREE_DO_AND_RETURN (TreePlotFunctionAndTaylor (diff));

//     for (size_t i = 0; i < diff->diffTreesCnt; i++)
//     {
//         TREE_DO_AND_RETURN (TreeCreatePlotImage (diff, &diff->diffTrees[i], i));
//     }

//     return TREE_OK;
// }

int TreeCreatePlotImage (differentiator_t *diff, tree_t *tree, const char *fileName)
{
    assert (diff);
    assert (tree);

    const size_t kPathMaxLen = kFileNameLen + 32;

    char plotPath[kPathMaxLen]   = {};
    
    int status = snprintf (plotPath, kPathMaxLen, "%s%s.txt", 
                           diff->log.plotFolderPath, fileName);
    if (status < 0)
    {
        ERROR_LOG ("Error in snprintf(), return code = %d", status);

        return TREE_ERROR_COMMON |
               COMMON_ERROR_SNPRINTF;
    }

    TREE_DO_AND_RETURN (PlotGenerateData (diff, tree, plotPath));

    TREE_DO_AND_RETURN (RunGnuPlot (diff, plotPath, fileName));

    return TREE_OK;
}


int TreePlotFunctionAndTaylor (differentiator_t *diff, const char *fileName)
{
    assert (diff);
    assert (fileName);

    const size_t kPathMaxLen = kFileNameLen + 32;

    char funcPlotPath[kPathMaxLen]   = {};
    char taylorPlotPath[kPathMaxLen] = {};
    
    int status = snprintf (funcPlotPath, kPathMaxLen, "%s%s.txt", 
                           diff->log.plotFolderPath, kExpressionFileName);
    if (status < 0)
    {
        ERROR_LOG ("Error in snprintf(), return code = %d", status);

        return TREE_ERROR_COMMON |
               COMMON_ERROR_SNPRINTF;
    }
    
    status = snprintf (taylorPlotPath, kPathMaxLen, "%s%s.txt", 
                       diff->log.plotFolderPath, kTaylorFileName);
    if (status < 0)
    {
        ERROR_LOG ("Error in snprintf(), return code = %d", status);

        return TREE_ERROR_COMMON |
               COMMON_ERROR_SNPRINTF;
    }

    TREE_DO_AND_RETURN (PlotGenerateData (diff, &diff->expression, funcPlotPath));
    TREE_DO_AND_RETURN (PlotGenerateData (diff, &diff->taylor, taylorPlotPath));

    TREE_DO_AND_RETURN (RunGnuPlot2Functions (diff, fileName, funcPlotPath, taylorPlotPath));

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

int RunGnuPlot (differentiator_t *diff, const char *plotFilePath, const char *fileName)
{
    assert (diff);
    assert (plotFilePath);
    assert (fileName);

    const size_t kPathMaxLen = kFileNameLen + 32;

    char pngFilePath[kPathMaxLen] = {};
    int status = snprintf (pngFilePath, kPathMaxLen, "%s%s.png", diff->log.plotFolderPath, fileName);
    if (status < 0)
    {
        ERROR_LOG ("Error in snprintf(), return code = %d", status);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_SNPRINTF;
    }

    FILE *scriptFile = fopen (diff->log.plotScriptFilePath, "w");
    if (scriptFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", diff->log.plotScriptFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (scriptFile, kPlotOneFuncScript, kLeftRange, kRightRange, pngFilePath, plotFilePath);
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

int RunGnuPlot2Functions (differentiator_t *diff, const char *pngFileName, 
                          const char *firstData, const char *secondData)
{
    assert (diff);
    assert (pngFileName);

    const size_t kPathMaxLen = kFileNameLen + 32;

    char pngFilePath[kPathMaxLen] = {};
    int status = snprintf (pngFilePath, kPathMaxLen, "%s%s.png", diff->log.plotFolderPath, pngFileName);

    FILE *scriptFile = fopen (diff->log.plotScriptFilePath, "w");
    if (scriptFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", diff->log.plotScriptFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (scriptFile, kPlotTwoFuncScript, kLeftRange, kRightRange, pngFilePath, firstData, secondData);
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
