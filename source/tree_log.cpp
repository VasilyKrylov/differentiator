#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include "tree.h"
#include "tree_calc.h"
#include "utils.h"

#include "tree_log.h"

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

void TreePrefixPass     (differentiator_t *diff, node_t *node, FILE *graphFile);
int TreeDumpImg         (differentiator_t *diff, node_t *node);
int DumpMakeConfig      (differentiator_t *diff, node_t *node);
int DumpMakeImg         (node_t *node, treeLog_t *log);

int LogCtor (treeLog_t *log)
{
    time_t t = time (NULL);
    struct tm tm = *localtime (&t);

    int randomNumber = rand();

    snprintf (log->logFolderPath, kFileNameLen, "%s%d-%02d-%02d_%02d:%02d:%02d_%d/",
              kParentDumpFolderName,
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
              tm.tm_hour,        tm.tm_min,     tm.tm_sec,
              randomNumber);

    snprintf (log->latexFilePath, kFileNameLen, "%s%s",
              log->logFolderPath, kLatexFileName);

    snprintf (log->htmlFilePath,   kFileNameLen, "%s%s",
              log->logFolderPath, kHtmlFileName);

    snprintf (log->imgFolderPath, kFileNameLen, "%s%s",
              log->logFolderPath, kImgFolderName);

    snprintf (log->dotFolderPath, kFileNameLen, "%s%s",
              log->logFolderPath, kDotFolderName);


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
    fprintf (log->latexFile, "%s",  "\\documentclass{article}\n"
                                    "\\usepackage[utf8x]{inputenc}\n"
                                    "\\usepackage[english,russian]{babel}\n"
                                    "\\usepackage{amsmath}\n"
                                    "\\usepackage{autobreak}\n"
                                    "\\allowdisplaybreaks\n"
                                    "\n"
                                    "\\newcommand{\\ctan}{\\mathrm{ctan}}"
                                    "\\newcommand{\\arcctan}{\\mathrm{arcctan}}"
                                    "\n"
                                    "\\begin{document}\n"
                                    "\n");

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

    snprintf (command, commandSize, "pdflatex "
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

// NOTE: better to make argument for dump to choose in wich style to make image (?)
// void TreePrefixPassOld (differentiator_t *diff, node_t *node, FILE *graphFile)
// {
//     assert (diff);
//     assert (node);
//     assert (graphFile);

//     fprintf (graphFile,
//             "\tnode%p [shape=Mrecord; style=\"filled\"; fillcolor=\"%s\"; "
//             "label = \"{ type = %s",
//             node, kBlue, GetValueTypeName(node->type));
    
//     fprintf (graphFile, "%s", " | value = ");

//     int idx = node->value.idx;

//     switch (node->type)
//     {
//         case TYPE_UKNOWN:           fprintf (graphFile, "%g", node->value.number);                      break;
//         case TYPE_CONST_NUM:        fprintf (graphFile, "%g", node->value.number);                      break;
//         case TYPE_MATH_OPERATION:   fprintf (graphFile, "%s", operators[idx].name);                     break; // FIXME: make find function
//         // case TYPE_VARIABLE:         fprintf (graphFile, "%d (\\\"%s\\\")", idx, diff->variables[idx].name);   break;
//         case TYPE_VARIABLE:         fprintf (graphFile, "%d (\\\" \\\")", idx);   break;
//         default:                    fprintf (graphFile, "error");                                       break;
//     }

//     fprintf (graphFile, " | ptr = [%p] | left = [%p] | right = [%p] }\"];\n", 
//                         node, node->left, node->right);

//     DEBUG_VAR ("%p", node);
//     DEBUG_LOG ("\t node->left: %p", node->left);
//     DEBUG_LOG ("\t node->right: %p", node->right);

//     if (node->left != NULL)
//     {
//         DEBUG_LOG ("\t %p->%p\n", node, node->left);

//         fprintf (graphFile, "\tnode%p->node%p\n", node, node->left);
        
//         TreePrefixPass (diff, node->left, graphFile);
//     }    

//     if (node->right != NULL)
//     {
//         DEBUG_LOG ("\t %p->%p\n", node, node->left);

//         fprintf (graphFile, "\tnode%p->node%p\n", node, node->right);
        
//         TreePrefixPass (diff, node->right, graphFile);
//     }
// }
void TreePrefixPass (differentiator_t *diff, node_t *node, FILE *graphFile)
{
    assert (diff);
    assert (node);
    assert (graphFile);

    fprintf (graphFile,
             "\tnode%p [shape=Mrecord; style=\"filled\"; ",
             node);

    switch (node->type)
    {
        case TYPE_UKNOWN:           fprintf (graphFile, "fillcolor=\"%s\";", kRed); break;
        case TYPE_CONST_NUM:        fprintf (graphFile, "fillcolor=\"%s\";", kBlue); break;
        case TYPE_MATH_OPERATION:   fprintf (graphFile, "fillcolor=\"%s\";", kGreen); break;
        case TYPE_VARIABLE:         fprintf (graphFile, "fillcolor=\"%s\";", kViolet); break;                                        break;
        
        default: assert (0);
    }

    fprintf (graphFile, " label = \" {");

    int idx = node->value.idx;

    switch (node->type)
    {
        // FIXME: make find function instead of [idx]
        case TYPE_UKNOWN:           fprintf (graphFile, "%g", node->value.number);                          break;
        case TYPE_CONST_NUM:        fprintf (graphFile, "%g", node->value.number);                          break;
        case TYPE_MATH_OPERATION:   fprintf (graphFile, "%s", operators[idx].name);                         break;
        case TYPE_VARIABLE:         fprintf (graphFile, "%s", diff->variables[idx].name); break;
        default:                    fprintf (graphFile, "error");                                           break;
    }

    fprintf (graphFile, " }\"];\n");

    DEBUG_VAR ("%p", node);
    DEBUG_LOG ("\t node->left: %p", node->left);
    DEBUG_LOG ("\t node->right: %p", node->right);

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
    snprintf (imgFileName, kFileNameLen, "%lu.svg", imageCounter);

    const size_t kMaxCommandLen = 256;
    char command[kMaxCommandLen] = {};

    snprintf (command, kMaxCommandLen, "dot %s%lu.dot -T svg -o %s%s", 
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

int DumpLatexDifferentation (differentiator_t *diff, node_t *expression, node_t *result,
                             variable_t *argument)
{
    assert (diff);
    assert (expression);
    assert (result);
    assert (argument);

    treeLog_t *log = &diff->log;

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
        "Чтобы сыграть психически больного и глубоко депрессивного человека, Хоакин Феникс решил это:",
    };
    fprintf (log->latexFile, "%s\\\\\n"
                             "\\begin{align}\n"
                             "\\begin{autobreak}\n"
                             "\\MoveEqLeft\n"
                             "\t\\frac{d}{d%s}(",
                             funny[rand() % 7],
                             argument->name);
     
    int status = DumpLatexNode (diff, expression);

    fprintf (log->latexFile, ") = \n"
                             "\t");

    status |= DumpLatexNode (diff, result);

    fprintf (log->latexFile, "\n"
                             "\\end{autobreak}\n"
                             "\\end{align}\n");

    return status;
}

int DumpLatex (differentiator_t *diff, node_t *node, const char *comment)
{
    assert (diff);
    assert (node);

    treeLog_t *log = &diff->log;

    fprintf (log->latexFile, "%s\n", comment);

    fprintf (log->latexFile, "\\[\n"
                             "\t");

    int status = DumpLatexNode (diff, node);

    fprintf (log->latexFile, "\n\\]\n\n");

    return status;
}

// FIXME: fix boxed
int DumpLatexBoxed (differentiator_t *diff, node_t *node, const char *comment)
{
    assert (diff);
    assert (node);

    treeLog_t *log = &diff->log;

    fprintf (log->latexFile, "%s\n", comment);

    fprintf (log->latexFile,// "\\[\n"
                            //  "\t\\boxed {\n"
                             "\\begin{align}\n"
                             "\\begin{autobreak}\n"
                             "\t");

    int status = DumpLatexNode (diff, node);

    fprintf (log->latexFile, "\n"
                             "\\end{autobreak}\n"
                             "\\end{align}\n");

    return status;
}

int DumpLatexNode (differentiator_t *diff, node_t *node)
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
            fprintf (log->latexFile, "%s", diff->variables[node->value.idx].name);
            break;

        case TYPE_MATH_OPERATION:
            DumpLatexNodeMathOperation (diff, node);
            break;

        default:
            break;
    }

    fputc (' ', log->latexFile);

    return TREE_OK;
}

int DumpLatexNodeMathOperation (differentiator_t *diff, node_t *node)
{
    assert (diff);
    assert (node);

    treeLog_t *log = &diff->log;

    switch (node->value.idx)
    {
        case OP_ADD:
            fprintf (log->latexFile, "%s", "(");
            DumpLatexNode (diff, node->left);
            fprintf (log->latexFile, "%s", "\n\t+");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")");
            break;

        case OP_SUB:
            fprintf (log->latexFile, "%s", "(");
            DumpLatexNode (diff, node->left);
            fprintf (log->latexFile, "%s", "-");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")");
            break;

        case OP_MUL:
            DumpLatexNode (diff, node->left);
            fprintf (log->latexFile, "%s", "\\times ");
            DumpLatexNode (diff, node->right);
            break;

        case OP_DIV:
            fprintf (log->latexFile, "%s", "\\frac{");
            DumpLatexNode (diff, node->left);
            fprintf (log->latexFile, "%s", "}{");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", "}");
            break;

        case OP_POW:
            fprintf (log->latexFile, "%s", "{(");
            DumpLatexNode (diff, node->left);
            fprintf (log->latexFile, "%s", ")}^{");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", "}");
            break;

        case OP_LOG:
            fprintf (log->latexFile, "%s", "\\log_{");
            DumpLatexNode (diff, node->left);
            fprintf (log->latexFile, "%s", "} ");
            DumpLatexNode (diff, node->right);
            break;

        case OP_LN:
            fprintf (log->latexFile, "%s", "\\ln ");
            DumpLatexNode (diff, node->right);
            break;

        case OP_SIN:
            fprintf (log->latexFile, "%s", "\\sin(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")");
            break;

        case OP_COS:
            fprintf (log->latexFile, "%s", "\\cos(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")");
            break;

        case OP_TG:
            fprintf (log->latexFile, "%s", "\\tan{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;

        case OP_CTG:
            fprintf (log->latexFile, "%s", "\\ctan{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;

        case OP_ARCSIN:
            fprintf (log->latexFile, "%s", "\\arcsin{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;
        case OP_ARCCOS:
            fprintf (log->latexFile, "%s", "\\arccos{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;

        case OP_ARCTG:
            fprintf (log->latexFile, "%s", "\\arctg{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;
        case OP_ARCCTG:
            fprintf (log->latexFile, "%s", "\\arcctg{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;

        case OP_SH:
            fprintf (log->latexFile, "%s", "\\sh{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;
        case OP_CH:
            fprintf (log->latexFile, "%s", "\\ch{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;
        case OP_TH:
            fprintf (log->latexFile, "%s", "\\th{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;
        case OP_CTH:
            fprintf (log->latexFile, "%s", "\\cth{(");
            DumpLatexNode (diff, node->right);
            fprintf (log->latexFile, "%s", ")}");
            break;
        
        default:
            ERROR_LOG ("%s", "Uknown operation");
            return TREE_ERROR_INVALID_NODE;
    }

    return TREE_OK;
}