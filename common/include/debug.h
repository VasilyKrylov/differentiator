// 26.10.2025 version of debug.h:
//     * ERROR() -> ERROR_LOG()
//     * Every macro requires ;

// shoul be renamed to colourfull_output.h (?)

#ifndef K_DEBUG_H
#define K_DEBUG_H

#include <stdio.h>

#define RED_COLOR           "\33[31m"
#define GREEN_COLOR         "\33[32m"
#define YELLOW_COLOR        "\33[33m"
#define BLUE_COLOR          "\33[34m"
#define MAGENTA_COLOR       "\33[35m"

#define RED_BOLD_COLOR      "\33[1;31m"
#define GREEN_BOLD_COLOR    "\33[1;32m"
#define YELLOW_BOLD_COLOR   "\33[1;33m"
#define BLUE_BOLD_COLOR     "\33[1;34m"

#define COLOR_END           "\33[0m"

// TODO: with PRINT_DEBUG add getchar() to prevent ignoring errors with lots of DEBUG_LOG()
#define PRINT(format, ...)        printf (GREEN_BOLD_COLOR format COLOR_END, ##__VA_ARGS__)
#define ERROR_LOG(format, ...)   fprintf (stderr, RED_BOLD_COLOR "[ERROR] %s:%d:%s(): " format "\n" COLOR_END, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define ERROR_PRINT(format, ...) fprintf (stderr, RED_BOLD_COLOR format "\n" COLOR_END, __VA_ARGS__)

enum commonErrors
{
    COMMON_ERROR_OK                     = 0,
    COMMON_ERROR_ALLOCATING_MEMORY      = 1 << 0,
    COMMON_ERROR_REALLOCATING_MEMORY    = 1 << 1,
    COMMON_ERROR_OPENING_FILE           = 1 << 2,
    COMMON_ERROR_NULL_POINTER           = 1 << 3,
    COMMON_ERROR_READING_INPUT          = 1 << 4,
    COMMON_ERROR_READING_FILE           = 1 << 5,
    COMMON_ERROR_WRITE_TO_FILE          = 1 << 6,
    COMMON_ERROR_CREATING_FILE          = 1 << 7,
    COMMON_ERROR_WRONG_USER_INPUT       = 1 << 8,
    COMMON_ERROR_RUNNING_SYSTEM_COMMAND = 1 << 10 // TODO: add text messages
};


#ifdef PRINT_DEBUG
    #define DEBUG_LOG(format, ...)                                      \
            do {                                                        \
                fprintf(stderr, BLUE_BOLD_COLOR                         \
                        "[DEBUG] %s:%d:%s(): " format "\n" COLOR_END,   \
                        __FILE__, __LINE__, __func__, __VA_ARGS__);     \
            } while(0)

    #define DEBUG_PRINT(format, ...)                                            \
            do {                                                                \
                fprintf(stderr, BLUE_BOLD_COLOR format COLOR_END, __VA_ARGS__); \
            } while(0)

    #define DEBUG_VAR(format, name)                              \
            do {                                                 \
                DEBUG_LOG ("%s = \"" format "\";", #name, name); \
            } while(0)
    #define ON_DEBUG(...) __VA_ARGS__

#else
    #define DEBUG_LOG(format, ...)
    #define DEBUG_PRINT(format, ...)
    #define DEBUG_VAR(format, name)
    #define ON_DEBUG(...)

#endif // PRINT_DEBUG

void PrintCommonError (int error);

#endif // K_DEBUG_H
