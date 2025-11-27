#include <stdio.h>

#include "debug.h"

#define CHECK_ERROR(errorCode, message)                        \
        do {                                                   \
            if (error & errorCode) ERROR_PRINT ("%s", message);\
        } while(0)

void PrintCommonError (int error)
{
    DEBUG_LOG ("Common error = %d;\n", error);

    CHECK_ERROR (COMMON_ERROR_ALLOCATING_MEMORY,    "Error allocating memory");
    CHECK_ERROR (COMMON_ERROR_REALLOCATING_MEMORY,  "Error reallocating memory");
    CHECK_ERROR (COMMON_ERROR_OPENING_FILE,         "Error opening file");
    CHECK_ERROR (COMMON_ERROR_NULL_POINTER,         "Some pointer is NULL, but it should not be NULL");
    CHECK_ERROR (COMMON_ERROR_READING_INPUT,        "Error reading data from stdin");
    CHECK_ERROR (COMMON_ERROR_WRITE_TO_FILE,        "Error while writing some data to file");
}

#undef CHECK_ERROR