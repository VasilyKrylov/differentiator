#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "tree_log.h"

#include "tree.h"
#include "tree_calc.h"
#include "tree_plot.h"
#include "utils.h"

static size_t imageCounter = 0;

const char * const kBlack       = "#000000";
const char * const kGray        = "#ebebe0";

const char * const kRed         = "#ff0000";
const char * const kViolet      = "#cc99ff";
const char * const kBlue        = "#66ccff";

const char * const kGreen       = "#99ff66";
const char * const kDarkGreen   = "#33cc33";
const char * const kYellow      = "#ffcc00";

const char * const kHeadColor   = kViolet;
const char * const kFreeColor   = kDarkGreen;
const char * const kEdgeNormal  = kBlack;

static void TreePrefixPass     (differentiator_t *diff, node_t *node, FILE *graphFile);
static int TreeDumpImg         (differentiator_t *diff, node_t *node);
static int DumpMakeConfig      (differentiator_t *diff, node_t *node);
static int DumpMakeImg         (node_t *node, treeLog_t *log);
static int NodeMathOpDiverativeLatex (differentiator_t *diff, node_t *node, variable_t *argument);

int LogCtor (treeLog_t *log)
{
    time_t t = time (NULL);
    struct tm tm = *localtime (&t);

    snprintf (log->logFolderPath, kFileNameLen, "%s%d-%02d-%02d_%02d:%02d:%02d/",
              kParentDumpFolderName,
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
              tm.tm_hour,        tm.tm_min,     tm.tm_sec);

    snprintf (log->latexFilePath,       kFileNameLen, "%s%s",
              log->logFolderPath,       kLatexFileName);

    snprintf (log->htmlFilePath,        kFileNameLen, "%s%s",
              log->logFolderPath,       kHtmlFileName);

    snprintf (log->plotScriptFilePath,  kFileNameLen, "%s%s",
              log->logFolderPath,       kPlotScriptFileName);

    snprintf (log->imgFolderPath,       kFileNameLen, "%s%s",
              log->logFolderPath,       kImgFolderName);

    snprintf (log->dotFolderPath,       kFileNameLen, "%s%s",
              log->logFolderPath,       kDotFolderName);

    snprintf (log->plotFolderPath,      kFileNameLen, "%s%s",
              log->logFolderPath,       kPlotFolderName);

    if (SafeMkdir (kParentDumpFolderName) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->logFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->imgFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->dotFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    if (SafeMkdir (log->plotFolderPath) != TREE_OK)
        return TREE_ERROR_COMMON | 
               COMMON_ERROR_CREATING_FILE;

    log->htmlFile = fopen (log->htmlFilePath, "w");
    if (log->htmlFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", log->htmlFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }
    fprintf (log->htmlFile, "%s", "<pre>\n");

    log->latexFile = fopen (log->latexFilePath, "w");
    if (log->latexFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", log->latexFilePath);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }
    fprintf (log->latexFile, "%s",  kLatexHeader);

    return TREE_OK;
}

void LogDtor (treeLog_t *log)
{
    fprintf (log->htmlFile, "%s", "</pre>\n");

    fprintf (log->latexFile, "%s", "\\end{document}\n");

    fclose (log->htmlFile);
    fclose (log->latexFile);

    const size_t commandSize = kFileNameLen + 64;

    char command[commandSize] = {};

    snprintf (command, commandSize, "pdflatex -interaction=batchmode "
                                     ON_RELEASE ("> /dev/null ") 
                                     "%s", log->latexFilePath);

    DEBUG_VAR ("%s", command);

    system (command);
}

int NodeDump (differentiator_t *diff, node_t *node,
              const char *file, int line, const char *func, 
              const char *format, ...)
{
    assert (diff);
    assert (node);
    assert (file);
    assert (func);
    assert (format);

    DEBUG_PRINT ("%s", "\n========== NODE DUMP START ==========\n");

    treeLog_t *log = &diff->log;

    fprintf (log->htmlFile,
             "<h3>NODE DUMP called at %s:%d:%s(): <font style=\"color: green;\">",
             file, line, func);

    va_list  args = {};
    va_start (args, func);
    vfprintf (log->htmlFile, format, args);
    va_end   (args);
    
    fprintf (log->htmlFile,
             "%s",
             "</font></h3>\n");
    
    TREE_DO_AND_RETURN (TreeDumpImg (diff, node));

    fprintf (log->htmlFile, "%s", "<hr>\n\n");

    fflush (log->htmlFile);

    DEBUG_PRINT ("%s", "========== NODE DUMP END ==========\n\n");

    return TREE_OK;
}

int TreeDump (differentiator_t *diff, tree_t *tree, 
              const char *file, int line, const char *func, 
              const char *format, ...)
{
    assert (tree);
    assert (tree->root);
    assert (format);
    assert (file);
    assert (func);

    DEBUG_PRINT ("%s", "\n========== START OF TREE DUMP TO HTML  ==========\n");

    treeLog_t *log = &diff->log;
    
    fprintf (log->htmlFile,
             "<h3>TREE DUMP called at %s:%d:%s(): <font style=\"color: green;\">",
             file, line, func);
        
    va_list args = {};
    va_start (args, func);
    vfprintf (log->htmlFile, format, args);
    va_end   (args);

    fprintf (log->htmlFile, "%s", "</font></h3>\n");
        
    ON_DEBUG (
        fprintf (log->htmlFile,
                 "%s[%p] initialized in {%s:%d}\n",
                 tree->varInfo.name, tree, tree->varInfo.file, tree->varInfo.line);
    );
    fprintf (log->htmlFile,
             "tree->size = %lu;\n",
             tree->size);

    TREE_DO_AND_RETURN (TreeDumpImg (diff, tree->root));

    fprintf (log->htmlFile, "%s", "<hr>\n\n");

    fflush (log->htmlFile);

    DEBUG_PRINT ("%s", "========== END OF TREE DUMP TO HTML  ==========\n\n");
    
    return TREE_OK;
}

int TreeDumpImg (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    TREE_DO_AND_RETURN (DumpMakeConfig (diff, node));

    TREE_DO_AND_RETURN (DumpMakeImg (node, &diff->log));

    return TREE_OK;
}


void TreePrefixPass (differentiator_t *diff, node_t *node, FILE *graphFile)
{
    assert (diff);
    assert (node);
    assert (graphFile);

    fprintf (graphFile,
             "\tnode%p [shape=Mrecord; style=\"filled\"; fillcolor=",
             node);

    DEBUG_VAR ("%lu", node->value.idx);

    const keyword_t *keyword = FindKeywordByIdx (node->value.idx);

    switch (node->type)
    {
        case TYPE_UKNOWN:           fprintf (graphFile, "\"%s\";", kGray);      break;
        case TYPE_CONST_NUM:        fprintf (graphFile, "\"%s\";", kBlue);      break;
        case TYPE_MATH_OPERATION:   if (keyword->isFunction)
                                        fprintf (graphFile, "\"%s\";", kYellow);
                                    else
                                        fprintf (graphFile, "\"%s\"", kGreen);  
                                    break;
        case TYPE_VARIABLE:         fprintf (graphFile, "\"%s\";", kViolet);    break;
        default:                    fprintf (graphFile, "\"%s\";", kRed);      break;
    }

    variable_t *var = FindVariableByIdx (diff, (size_t) node->value.idx);

    fprintf (graphFile, " label = \" {");

    switch (node->type)
    {
        case TYPE_UKNOWN:           fprintf (graphFile, "%g", node->value.number);          break;
        case TYPE_CONST_NUM:        fprintf (graphFile, "%g", node->value.number);          break;
        case TYPE_MATH_OPERATION:   fprintf (graphFile, "%s", keyword->name);               break;
        case TYPE_VARIABLE:         fprintf (graphFile, "%.*s", (int)var->len, var->name);  break;
        default:                    fprintf (graphFile, "error");                           break;
    }

    fprintf (graphFile, " }\"];\n");

    DEBUG_PTR (node);
    // DEBUG_LOG ("\t node->left: %p", node->left);
    // DEBUG_LOG ("\t node->right: %p", node->right);

    if (node->left != NULL)
    {
        DEBUG_LOG ("\t %p->%p\n", node, node->left);

        fprintf (graphFile, "\tnode%p->node%p\n", node, node->left);
        
        TreePrefixPass (diff, node->left, graphFile);
    }    

    if (node->right != NULL)
    {
        DEBUG_LOG ("\t %p->%p\n", node, node->left);

        fprintf (graphFile, "\tnode%p->node%p\n", node, node->right);
        
        TreePrefixPass (diff, node->right, graphFile);
    }
}

int DumpMakeConfig (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    imageCounter++;

    char graphFilePath[kFileNameLen + 22] = {};
    snprintf (graphFilePath, kFileNameLen + 22, "%s%lu.dot", diff->log.dotFolderPath, imageCounter);

    DEBUG_VAR ("%s", graphFilePath);

    FILE *graphFile  = fopen (graphFilePath, "w");
    if (graphFile == NULL)
    {
        ERROR_LOG ("Error opening file \"%s\"", graphFilePath);

        return TREE_ERROR_COMMON |
               COMMON_ERROR_OPENING_FILE;
    }

    fprintf (graphFile  ,   "digraph G {\n"
                            // "\tsplines=ortho;\n"
                            // "\tnodesep=0.5;\n"
                            "\tnode [shape=octagon; style=\"filled\"; fillcolor=\"#ff8080\"];\n");

    TreePrefixPass (diff, node, graphFile);

    fprintf (graphFile, "%s", "}");
    fclose  (graphFile);

    return TREE_OK;
}
int DumpMakeImg (node_t *node, treeLog_t *log)
{
    assert (node);
    assert (log);

    char imgFileName[kFileNameLen] = {};
    snprintf (imgFileName, kFileNameLen, "%lu.png", imageCounter);

    const size_t kMaxCommandLen = 256;
    char command[kMaxCommandLen] = {};

    snprintf (command, kMaxCommandLen, "dot %s%lu.dot -T png -o %s%s", 
              log->dotFolderPath, imageCounter,
              log->imgFolderPath, imgFileName);

    int status = system (command);
    DEBUG_VAR ("%d", status);
    if (status != 0)
    {
        ERROR_LOG ("ERROR executing command \"%s\"", command);
        
        return TREE_ERROR_COMMON |
               COMMON_ERROR_RUNNING_SYSTEM_COMMAND;
    }

    fprintf (log->htmlFile,
             "<img src=\"img/%s\" hieght=\"500px\">\n",
             imgFileName);

    DEBUG_VAR ("%s", command);

    return TREE_OK;
}

// =============== LATEX ===============

int DumpLatexDifferentation (differentiator_t *diff, node_t *expression, 
                             variable_t *argument)
{
    assert (diff);
    assert (expression);
    assert (argument);

    FILE *latexFile = diff->log.latexFile;

    // https://ctan.math.utah.edu/ctan/tex-archive/macros/latex/contrib/autobreak/autobreak.pdf
    // awesome package

    const char *funny[] = 
    {
        "Нетрудно заметить, что:",
        "Любому ёжику понятно, что:",
        "Из программы 3 класса вы должны знать следующее преобразование:",
        "Любой образованный человек знает следующее:",
        "Элементарно, Ватсон:",
        "Это база:",
        "Кокнуло:",
        "Используем шустрые преобрзования:",
        "Чтобы сыграть психически больного и глубоко депрессивного человека в фильме Джокер, Хоакин Феникс решил это:",
    };

    fprintf (latexFile, "%s\\\\\n"
                        "\\begin{align*}\n"
                        "\\begin{autobreak}\n"
                        "\\MoveEqLeft\n"
                        "\t\\frac{d}{d%.*s}(",
                        funny[rand() % 9],
                        (int) argument->len,
                        argument->name);

    int status = DumpLatexNode (diff, expression, NULL);
    if (status != TREE_OK)
        return status;


    fprintf (latexFile, ") = \n"
                        "\t");

    // status = DumpLatexNode (diff, resultNode, NULL);
    // if (status != TREE_OK)
    //     return status;

    switch (expression->type)
    {
        case TYPE_UKNOWN:
            ERROR_LOG ("%s", "Uknown type");

            return TREE_ERROR_INVALID_NODE;

        case TYPE_CONST_NUM:
            fprintf (latexFile, "0");
            break;
        
        case TYPE_VARIABLE:
            if (expression->value.idx == argument->idx)
                fprintf (latexFile, "1");
            else
                fprintf (latexFile, "0");
            break;

        case TYPE_MATH_OPERATION:
            status = NodeMathOpDiverativeLatex (diff, expression, argument);
            if (status != TREE_OK)
                return status;

            break;

        default: assert (0 && "Uknown type");
        
    }

    // status |= DumpLatexNode (diff, result, NULL);

    fprintf (latexFile, "\n"
                        "\\end{autobreak}\n"
                        "\\end{align*}\n");

    return status;
}

// NOTE: I use macros, not string format in variables array, because i need more than 1 variant for '^'
// overall this looks like copy paste

#define dL                                                                              \
        fprintf (latexFile, "\\frac{d}{d%.*s}(", (int) argument->len, argument->name);  \
        DumpLatexNode (diff, node->left, node);                                         \
        fprintf (latexFile, ")")

#define dR                                                                              \
        fprintf (latexFile, "\\frac{d}{d%.*s}(", (int) argument->len, argument->name);  \
        DumpLatexNode (diff, node->right, node);                                        \
        fprintf (latexFile, ")")

#define cL                                      \
        DumpLatexNode (diff, node->left, node)

#define cR                                      \
        DumpLatexNode (diff, node->right, node)

#define cN                                      \
        DumpLatexNode (diff, node, node)

#define FILE_PRINT(str)                 \
        fprintf (latexFile, "%s", str)

#define NUM_(num)                       \
        fprintf (latexFile, "%g", num)

#define ADD_(left, right)               \
        left;                           \
        FILE_PRINT ("\n+");             \
        right

#define SUB_(left, right)               \
        left;                           \
        FILE_PRINT ("\n-");             \
        right

#define MUL_(left, right)               \
        left;                           \
        FILE_PRINT ("\n\\cdot");        \
        right;                          \

#define DIV_(left, right)               \
        FILE_PRINT ("\\frac{");         \
        left;                           \
        FILE_PRINT ("}{");              \
        right;                          \
        FILE_PRINT ("}")

#define POW_(left, right)               \
        FILE_PRINT ("(");               \
        left;                           \
        FILE_PRINT (")^");              \
        right

#define LN_(right)                      \
        FILE_PRINT ("\\ln {");          \
        right;                          \
        FILE_PRINT ("}")

#define SIN_(right)                      \
        FILE_PRINT ("\\sin (");          \
        right;                           \
        FILE_PRINT (")")

#define COS_(right)                      \
        FILE_PRINT ("\\cos (");          \
        right;                           \
        FILE_PRINT (")")

#define SH_(right)                      \
        FILE_PRINT ("\\sh (");          \
        right;                          \
        FILE_PRINT (")")

#define CH_(right)                      \
        FILE_PRINT ("\\ch (");          \
        right;                          \
        FILE_PRINT (")")

#define TH_(right)                      \
        FILE_PRINT ("\\sh (");          \
        right;                          \
        FILE_PRINT (")")

#define CTH_(right)                     \
        FILE_PRINT ("\\sh (");          \
        right;                          \
        FILE_PRINT (")")


int NodeMathOpDiverativeLatex (differentiator_t *diff, node_t *node, variable_t *argument)
{
    assert (diff);
    assert (node);
    assert (argument);

    DEBUG_PTR (node);

    FILE *latexFile = diff->log.latexFile;

    switch (node->value.idx)
    {
        case OP_UNKNOWN:
            ERROR_LOG ("%s", "Uknown math operation");

            return TREE_ERROR_INVALID_NODE;

        case OP_ADD:
            ADD_ (dL, dR);
            break;
        case OP_SUB:
            SUB_ (dL, dR);
            break;
        case OP_MUL:
            ADD_ (MUL_ (dL, cR), 
                  MUL_ (cL, dR));
            break;
        case OP_DIV:
            DIV_ (SUB_ (MUL_ (dL, cR), 
                        MUL_ (cL, dR)), 
                  POW_ (cR, NUM_(2.0)));
            break;

        case OP_POW:
        {
            bool xInBase    = NodeFindVariable (node->left, argument);
            bool xInDegree  = NodeFindVariable (node->right, argument);

            if (xInBase && xInDegree)
            {
                MUL_ (cN,
                      ADD_ (MUL_ (dR,
                                  LN_ (cL)),
                            MUL_ (cR,
                                  DIV_ (dL,
                                        cL))));
                break;
            }

            if (xInBase && !xInDegree)
            {
                MUL_ (MUL_ (cR,
                            POW_ (cL,
                                  SUB_ (cR, NUM_(1.0)))),
                      dL);
                break;
            }
            
            if (!xInBase && xInDegree)
            {
                MUL_ (cN,
                      LN_ (cL));
                break;
            }
            
            if (!xInBase && !xInDegree)
            {
                NUM_ (0.0);
                break;
            }
            
            assert (0 && "Uknown case of OP_POW");
        }

        case OP_LOG:
        {
            bool xInBase      = NodeFindVariable (node->left, argument);
            bool xInArgument  = NodeFindVariable (node->right, argument);

            if (!xInBase && xInArgument)
            {
                DIV_ (dR,
                     MUL_ (cR, LN_ (cL)));
                break;
            }
            if (!xInBase && !xInArgument)
            {
                NUM_ (0.0);
                break;
            }
            
            DIV_ (SUB_ (MUL_ (DIV_ (dR, cR),
                              LN_ (cL)),
                        MUL_ (DIV_ (dL, cL),
                              LN_ (cR))),
                  POW_ (LN_ (cL),
                        NUM_ (2.0)));

            break;
        }

        case OP_LN:
            DIV_ (dR, cR);
            break;

        case OP_SIN:
            MUL_ (COS_ (cR),
                  dR);
            break;
        case OP_COS:
            MUL_ (NUM_ (-1.0),
                      MUL_ (SIN_ (cR),
                                dR));
            break;

        case OP_TG:
            DIV_ (NUM_ (1.0),
                  POW_ (MUL_ (COS_ (cR), dR),
                        NUM_ (2.0)));
            break;
        
        case OP_CTG:
            DIV_ (NUM_ (1.0),
                  POW_ (MUL_ (SIN_ (cR), dR),
                        NUM_ (2.0)));
            break;

        case OP_ARCSIN:
            DIV_ (dR,
                  POW_ (SUB_ (NUM_(1.0), 
                              POW_ (cR, NUM_ (2.0))),
                        NUM_ (0.5)));
            break;

        case OP_ARCCOS:
            DIV_ (MUL_ (NUM_ (-1.0),
                        dR),
                  POW_ (SUB_ (NUM_(1.0), 
                              POW_ (cR, NUM_ (2.0))),
                        NUM_ (0.5)));
            break;

        case OP_ARCTG:
            DIV_ (dR,
                  ADD_ (NUM_(1.0), 
                        POW_ (cR, 
                              NUM_ (2.0))));
            break;

        case OP_ARCCTG:
            DIV_ (MUL_ (NUM_ (-1.0), 
                        dR),
                  ADD_ (NUM_(1.0), 
                        POW_ (cR, 
                              NUM_ (2.0))));
            break;

        case OP_SH:
            MUL_ (CH_ (cR),
                  dR);
            break;

        case OP_CH:
            MUL_ (SH_ (cR),
                  dR);
            break;

        case OP_TH:
            DIV_ (dR,
                  POW_ (CH_ (cR),
                        NUM_(2.0)));
            break;

        case OP_CTH:
            DIV_ (MUL_ (NUM_ (-1.0),
                        dR),
                  POW_ (SH_ (cR),
                        NUM_(2.0)));
            break;

        default:
            assert (0 && "There is not implemented differentation for function");
    }

    return TREE_OK;
}

#undef FILE_PRINT
#include "dsl_undef.h"

int DumpLatexFunction (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    FILE *latexFile = diff->log.latexFile;

    fprintf (latexFile, "%s", "\\section*{Давайте пересчитаем кости этой каверзной функции}\n");

    fprintf (latexFile, "\\[\n"
                        "\t f(%.*s",
                        (int) diff->variables[0].len,
                        diff->variables[0].name);
    
    for (size_t i = 1; i < diff->variablesSize; i++)
    {
        fprintf (latexFile, ", %.*s",
                           (int) diff->variables[i].len,
                           diff->variables[i].name);
    }
    fprintf (latexFile, "%s", ") = ");

    int status = DumpLatexNode (diff, node, NULL);

    fprintf (latexFile, "\n\\]\n\n");

    return status;
}

// FIXME: fix boxed
int DumpLatexAnswer (differentiator_t *diff, node_t *node, size_t devirativeCount)
{
    assert (diff);
    assert (node);

    treeLog_t *log = &diff->log;

    fprintf (log->latexFile, "\\subsection*{Ответ для %lu производной:}\n", devirativeCount);

    fprintf (log->latexFile,// "\\[\n"
                            //  "\t\\boxed {\n"
                             "\\begin{align*}\n"
                             "\\begin{autobreak}\n"
                             "\t");

    int status = DumpLatexNode (diff, node, NULL);

    fprintf (log->latexFile, "\n"
                             "\\end{autobreak}\n"
                             "\\end{align*}\n");

    return status;
}


#define NUM_(num)                                                                       \
        NodeCtorAndFill (&diff->taylor, TYPE_CONST_NUM, {.number = num}, NULL, NULL)
#define ADD_(left, right)                                                               \
        NodeCtorAndFill (&diff->taylor, TYPE_MATH_OPERATION, {.idx = OP_ADD}, left, right)
#define SUB_(left, right)                                                               \
        NodeCtorAndFill (&diff->taylor, TYPE_MATH_OPERATION, {.idx = OP_SUB}, left, right)
#define MUL_(left, right)                                                               \
        NodeCtorAndFill (&diff->taylor, TYPE_MATH_OPERATION, {.idx = OP_MUL}, left, right)
#define DIV_(left, right)                                                               \
        NodeCtorAndFill (&diff->taylor, TYPE_MATH_OPERATION, {.idx = OP_DIV}, left, right)
#define POW_(left, right)                                                               \
        NodeCtorAndFill (&diff->taylor, TYPE_MATH_OPERATION, {.idx = OP_POW}, left, right)
#define VAR_(idxVar)                                                                    \
        NodeCtorAndFill (&diff->taylor, TYPE_VARIABLE, {.idx = idxVar}, NULL, NULL)

int DumpLatexTaylor (differentiator_t *diff)
{
    assert (diff);

    // tree_t *tree = &diff->taylor; // FIXME: use tree in all macros
    TREE_CTOR (&diff->taylor, &diff->log);

    FILE *latexFile = diff->log.latexFile;

    fprintf (latexFile, "\\section*{Разложение по Тейлору} \\\n");

    fprintf (latexFile, "\\begin{align*}\n"
                        "\\begin{autobreak}\n"
                        "\t");

    double value = NodeCalculate (diff, diff->expression.root);

    fprintf (latexFile, "f (%.*s) = %g \n\t", 
                        (int) diff->varToDiff->len,
                        diff->varToDiff->name,
                        value);

    diff->taylor.root = NUM_ (value);
    
    size_t factorial = 1;

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        value = NodeCalculate (diff, diff->diffTrees[i].root);
        factorial *= (i + 1);

        fprintf (latexFile, 
                 "+ \\frac{%g}{%lu!} \\cdot (%.*s - %g) ^ %lu\n\t",
                 value,
                 i + 1, 
                 (int)diff->varToDiff->len, diff->varToDiff->name,
                 diff->varToDiff->value,
                 i + 1);

        diff->taylor.root = ADD_ (diff->taylor.root, 
                                  MUL_ (DIV_ (NUM_(value), 
                                              NUM_ ((double)factorial)
                                             ),
                                        POW_ (SUB_ (VAR_(diff->varToDiff->idx),
                                                   NUM_(diff->varToDiff->value)
                                                   ),
                                              NUM_ ((double)i + 1)
                                              )
                                       )
                                 );
    }

    fprintf (latexFile, "+ o(%.*s - %g) ^ %lu", 
                        (int) diff->varToDiff->len, diff->varToDiff->name,
                        diff->varToDiff->value,
                        diff->diffTreesCnt);
    
    fprintf (latexFile, "\n"
                        "\\end{autobreak}\n"
                        "\\end{align*}\n");

    return TREE_OK;
}

#undef NUM_
#undef POW_
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
#undef VAR_

int DumpLatexNode (differentiator_t *diff, node_t *node, node_t *parent) 
{
    assert (diff);
    assert (node);

    treeLog_t *log = &diff->log;

    fputc (' ', log->latexFile);

    switch (node->type)
    {
        case TYPE_UKNOWN:
            ERROR_LOG ("%s", "Uknown type");

            return TREE_ERROR_INVALID_NODE;

        case TYPE_CONST_NUM:
            if (node->value.number < 0)
                fprintf (log->latexFile, "(%g)", node->value.number);
            else
                fprintf (log->latexFile, "%g", node->value.number);
            break;
        
        case TYPE_VARIABLE:
            fprintf (log->latexFile, "%.*s", (int) diff->variables[node->value.idx].len,
                                             diff->variables[node->value.idx].name);
            break;

        case TYPE_MATH_OPERATION:
            DumpLatexNodeMathOperation (diff, node, parent);
            break;

        default:
            break;
    }

    fputc (' ', log->latexFile);

    return TREE_OK;
}

// FIXME: functions1
int DumpLatexNodeMathOperation (differentiator_t *diff, node_t *node, node_t *parent)
{
    assert (diff);
    assert (node);

    FILE *latexFile = diff->log.latexFile;

    const keyword_t *keyword = FindKeywordByIdx (node->value.idx);
    if (keyword == NULL)
    {
        ERROR_LOG ("%s", "Uknown operation");

        return TREE_ERROR_INVALID_NODE;
    }
    const char *latexFormat = keyword->latexFormat;


    bool requireBracket = false;

    if (parent == NULL) 
        requireBracket = false;
    else
        requireBracket = ((keyword->idx == OP_ADD || keyword->idx == OP_SUB) && 
                          (parent->value.idx == OP_MUL || parent->value.idx == OP_DIV)) ||
                         (parent->right == node && parent->value.idx == OP_SUB);
    
    if (requireBracket) fprintf (latexFile, "(");
    
    while (true)
    {
        const char *specificator = strchr (latexFormat, '%');
        const char *argument = specificator + 1;

        DEBUG_STR (specificator);
        DEBUG_STR (argument);

        fprintf (latexFile, "%.*s", int(specificator - latexFormat), latexFormat);

        if (*argument == 'l') DumpLatexNode (diff, node->left, node);
        if (*argument == 'r') DumpLatexNode (diff, node->right, node);
        if (*argument == 'e') break;

        latexFormat = argument + 1;
    }

    if (requireBracket) fprintf (latexFile, ")");

    return TREE_OK;
}

int DumpLatexAddImages (differentiator_t *diff)
{
    assert (diff);

    FILE *latexFile = diff->log.latexFile;

    fprintf (latexFile, "%s", "\\section*{Посмотрим теперь на интересные(или не очень) картиночки:} \\\\\n");

    const char kImageLatex[] = "\\begin{center}\n"
                               "\t\\includegraphics[width=13cm]{%s%s.png}\n"
                               "\\end{center}\n\n";

    TREE_DO_AND_RETURN (TreeCreatePlotImage (diff, &diff->expression, kExpressionFileName));
    fprintf (latexFile, "%s", "\\subsection*{Исходная функция:} \n");
    fprintf (latexFile, kImageLatex, diff->log.plotFolderPath, kExpressionFileName);

    TREE_DO_AND_RETURN (TreePlotFunctionAndTaylor (diff, kTaylorFileName));
    fprintf (latexFile, "%s", "\\subsection*{Исходная функция вместе с графиком Тейлора:} \n");
    fprintf (latexFile, kImageLatex, diff->log.plotFolderPath, kTaylorFileName);

    const size_t numMaxLen = 20;
    char number[numMaxLen] = {};

    for (size_t i = 0; i < diff->diffTreesCnt; i++)
    {
        int status = snprintf (number, numMaxLen, "%lu", i + 1);
        if (status < 0)
        {
            ERROR_LOG ("Error in snprintf(), return code = %d", status);

            return TREE_ERROR_COMMON |
                   COMMON_ERROR_SNPRINTF;
        }

        TreeCreatePlotImage (diff, &diff->diffTrees[i], number);
        
        fprintf (latexFile, "\\subsection*{График %s производной:} \n", number);
        
        fprintf (latexFile, kImageLatex, diff->log.plotFolderPath, number);
    }

    return TREE_OK;
}