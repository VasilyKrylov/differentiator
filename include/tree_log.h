#ifndef K_TREE_LOG_H
#define K_TREE_LOG_H

struct node_t;
struct tree_t;
struct variable_t;
struct differentiator_t;

const char kLatexHeader[] = "\\documentclass{article}\n"
                            "\\usepackage[utf8x]{inputenc}\n"
                            "\\usepackage[english,russian]{babel}\n"
                            "\\usepackage{amsmath}\n"
                            "\\usepackage{graphicx}\n" // for png
                            "\\usepackage{autobreak}\n"
                            "\\allowdisplaybreaks\n"
                            "\n"
                            "\\newcommand{\\ctan}{\\mathrm{ctan}}"
                            "\\newcommand{\\arcctan}{\\mathrm{arcctan}}"
                            "\n"
                            "\\title{Самая скучная домашка по матану}\n"
                            "\\author{Крылов Василий, Б05-532}\n"
                            "\n"
                            "\\begin{document}\n"
                            "\\maketitle\n"
                            "\n";

const char kParentDumpFolderName[] = "dump/";
const char kImgFolderName[]        = "img/";
const char kDotFolderName[]        = "dot/";
const char kPlotFolderName[]       = "plot/";
const char kPlotScriptFileName[]   = "plot/script.txt";
const char kHtmlFileName[]         = "log.html";
const char kLatexFileName[]        = "solve.tex";
const char kGraphFileName[]        = "dot.txt";

const size_t kFileNameLen            = 64;
const size_t kDateTimeLen            = 19;
const size_t kLogFolderPathLen       = kFileNameLen - (sizeof(kParentDumpFolderName) - 1) - kDateTimeLen;

struct treeLog_t
{
    char logFolderPath      [kLogFolderPathLen] = {}; // dump/[date-time]
    char imgFolderPath      [kFileNameLen]      = {}; // dump/[date-time]/img/
    char dotFolderPath      [kFileNameLen]      = {}; // dump/[date-time]/dot/
    char plotFolderPath     [kFileNameLen]      = {}; // dump/[date-time]/plot/
    char plotScriptFilePath [kFileNameLen]      = {}; // dump/[date-time]/plot/script.txt
    char htmlFilePath       [kFileNameLen]      = {}; // dump/[date-time]/log.html
    char latexFilePath      [kFileNameLen]      = {}; // dump/[date-time]/solve.tex

    FILE *htmlFile  = NULL;
    FILE *latexFile = NULL;
};

int LogCtor                     (treeLog_t *log);
void LogDtor                    (treeLog_t *log);

int TreeDump                    (differentiator_t *diff, tree_t *tree, 
                                 const char *file, int line, const char *func, 
                                 const char *format, ...)
                                __attribute__ ((format (printf, 6, 7)));

int NodeDump                    (differentiator_t *diff, node_t *node,
                                 const char *file, int line, const char *func, 
                                 const char *format, ...)
                                __attribute__ ((format (printf, 6, 7)));

int DumpLatexDifferentation     (differentiator_t *diff, node_t *expression, 
                                 variable_t *argument);

int DumpLatexFunction           (differentiator_t *diff, node_t *node);
int DumpLatexAnswer             (differentiator_t *diff, node_t *node, size_t devirativeCount);
int DumpLatexTaylor             (differentiator_t *diff);
int DumpLatexNode               (differentiator_t *diff, node_t *node, node_t *parent);
int DumpLatexNodeMathOperation  (differentiator_t *diff, node_t *node, node_t *parent);

int DumpLatexAddImages          (differentiator_t *diff);

#endif // K_TREE_LOG_H