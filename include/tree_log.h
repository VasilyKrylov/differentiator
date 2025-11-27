#ifndef K_TREE_LOG_H
#define K_TREE_LOG_H

struct node_t;
struct tree_t;
struct variable_t;
struct differentiator_t;

const char kParentDumpFolderName[] = "dump/";
const char kImgFolderName[]        = "img/";
const char kDotFolderName[]        = "dot/";
const char kHtmlFileName[]         = "log.html";
const char kLatexFileName[]        = "solve.tex";
const char kGraphFileName[]        = "dot.txt";

const size_t kLogFolderPathLen       = 44;
const size_t kFileNameLen            = 64;

struct treeLog_t
{
    char logFolderPath [kLogFolderPathLen] = {}; // dump/[date-time]
    char imgFolderPath [kFileNameLen]      = {}; // dump/[date-time]/img
    char dotFolderPath [kFileNameLen]      = {}; // dump/[date-time]/dot
    char htmlFilePath  [kFileNameLen]      = {}; // dump/[date-time]/log.html
    char latexFilePath [kFileNameLen]      = {}; // dump/[date-time]/solve.tex

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
int DumpLatexDifferentation     (differentiator_t *diff, node_t *expression, node_t *result, variable_t *argument);
int DumpLatex                   (differentiator_t *diff, node_t *node, const char *comment);
int DumpLatexBoxed              (differentiator_t *diff, node_t *node, const char *comment);
int DumpLatexNode               (differentiator_t *diff, node_t *node);
int DumpLatexNodeMathOperation  (differentiator_t *diff, node_t *node);

#endif // K_TREE_LOG_H