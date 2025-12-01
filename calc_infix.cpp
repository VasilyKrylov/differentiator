#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "debug.h"

#define SYNTAX_ERROR                                                    \
        {                                                               \
            ERROR_LOG ("Syntax Error on position \"%s\"", curPos);      \
            return NAN;                                                 \
        }

static double GetGramma             (char *s);
static double GetExpression         (char **curPos);
static double GetTerm               (char **curPos);
static double GetPrimaryExpression  (char **curPos);
static double GetNumber             (char **curPos, bool strict);

int main ()
{
    char s[1024] = {};
    scanf ("%1023s", s);

    double res = GetGramma (s);
    printf ("res: %g", res);
}

double GetGramma (char *s)
{
    assert (s);

    char *curPos = s;
    double val = GetExpression (&curPos);

    if (*curPos != '\0')
        SYNTAX_ERROR;

    return val;
}

double GetExpression (char **curPos)
{
    assert (curPos);
    assert (*curPos);

    DEBUG_VAR ("%s", *curPos);

    double val = GetTerm (curPos);

    while (**curPos == '+' ||
           **curPos == '-')
    {
        char operation = **curPos;
        (*curPos)++;

        double val2 = GetTerm (curPos);

        switch (operation)
        {
            case '+': val += val2; break;
            case '-': val -= val2; break;

            default: assert (0 && "ploxo");
        }
    }

    return val;
}

double GetTerm (char **curPos)
{
    assert (curPos);
    assert (*curPos);

    double val = GetPrimaryExpression (curPos);
    DEBUG_VAR ("%g", val);

    while (**curPos == '*' ||
           **curPos == '/')
    {
        char operation = **curPos;
        (*curPos)++;

        double val2 = GetPrimaryExpression (curPos);

        switch (operation)
        {
            case '*': val *= val2; break;
            case '/': val /= val2; break;

            default: assert (0 && "ploxo");
        }
    }

    return val;
}

double GetPrimaryExpression (char **curPos)
{
    assert (curPos);
    assert (*curPos);

    if (**curPos == '(')
    {
        (*curPos)++;
        
        double val = GetExpression (curPos);

        if (**curPos != ')')
            SYNTAX_ERROR;

        (*curPos)++;

        return val;
    }

    return GetNumber (curPos, true);
}

double GetNumber (char **curPos, bool strict)
{
    assert (curPos);
    assert (*curPos);

    double val = NAN;
    int readBytes = 0;

    int res = sscanf (*curPos, "%lf%n", &val, &readBytes);
    DEBUG_VAR ("%d", res);
    if (res != 1 && strict)
        SYNTAX_ERROR;

    *curPos += readBytes;
    DEBUG_VAR ("%s", *curPos);
    DEBUG_VAR ("%g", val);

    return val;
}