#ifndef K_TREE_PLOT
#define K_TREE_PLOT

#include "tree.h"
#include "tree_calc.h"

const char kExpressionFileName[]   = "expression";
const char kTaylorFileName[] = "taylor";

const int kLeftRange    = -25;
const int kRightRange   = 25;
const double kStep      = 0.0001;


const char kPlotOneFuncScript[] = "set terminal png size 1600,800\n"
                                  "set xrange[%d:%d]\n"
                                  "set output \"%s\"\n"
                                 //  "set title \"%lu-ая производная\"\n"
                               //    "plot x**2, cos(x), sin(x) * x"
                                  "plot \"%s\" with lines";

const char kPlotTwoFuncScript[] = // "gnuplot -e \""
                                 "set terminal png size 1600,800\n"
                                 "set xrange[%d:%d]\n"
                                 "set output \"%s\"\n"
                                 // "set title \"%lu-ая производная\"\n"
                              //    "plot x**2, cos(x), sin(x) * x"
                                 "plot \"%s\" with lines, \"%s\" with lines";

/*
gnuplot -e \"
set terminal png size 1600,800\n
set xrange[-25:25]\n
set output \\\"%s\\\"\n
plot \\\"%s\\\" with lines\"

plotScriptFilePath
set terminal png size 1600,800 
set xrange[-25:25] 
set output "dump/2025-12-02_04:54:18/plot/1.png"
plot "dump/2025-12-02_04:54:18/plot/1.txt" with lines
*/

// int TreeCreatePlotImages   (differentiator_t *diff);

int TreeCreatePlotImage       (differentiator_t *diff, tree_t *tree, const char *fileName);
int TreePlotFunctionAndTaylor (differentiator_t *diff, const char *fileName);

#endif // K_TREE_PLOT