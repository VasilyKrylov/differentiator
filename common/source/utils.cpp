#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

#include "debug.h"

ssize_t GetFileSize (const char *fileName);

int SafeMkdir (const char *fileName)
{
    struct stat st = {};
    if (stat (fileName, &st) != 0)
    {
        // rwxrwxr-x
        // idk WHY, but when I use rw-rw-r-- I can't create folder inside folder
        int status = mkdir (fileName, S_IRUSR | S_IWUSR | S_IXUSR |
                                      S_IRGRP | S_IWGRP | S_IXGRP |
                                      S_IROTH |           S_IXOTH);
        if (status != 0)
        {
            ERROR_LOG ("Error creating folder \"%s\" : %s",
                       fileName, strerror(errno));

            return COMMON_ERROR_CREATING_FILE;
        }
    }

    return COMMON_ERROR_OK;
}


void ClearBuffer() 
{
    int trash = '\0';

    while (trash != '\n' && trash != EOF) 
    {
        trash = getchar ();
    }
}

ssize_t GetFileSize (const char *fileName)
{
    struct stat fileStat;

    int status = stat (fileName, &fileStat);
    if (status != 0)
    {
        perror ("Error getting file size");

        return -1;
    }
    
    return fileStat.st_size;
}
// creat content buffer fileSize + 1
// content[fileSize] = '\0'
char *ReadFile (const char *inputFileName, size_t *bufferLen)
{
    FILE *inputFile = fopen (inputFileName, "r");
    if (inputFile == NULL)
    {
        ERROR_LOG ("Error opening input file \"%s\"", inputFileName);
        
        return NULL;
    }

    ssize_t fileSize = GetFileSize (inputFileName);
    if (fileSize == -1)
    {
        return NULL;
    }
    *bufferLen = (size_t) fileSize + 1;

    char *content = (char *) calloc (*bufferLen, sizeof(char));

    if (content == NULL)
    {
        perror ("Failed to allocate memory for text");

        fclose (inputFile);

        return NULL;
    }

    size_t bytesRead = fread (content, sizeof(char), (size_t)fileSize, inputFile);
    fclose (inputFile);

    if ((ssize_t)bytesRead != fileSize)
    {
        ERROR_LOG ("fread() status code(how many bytes read) is: %lu", bytesRead);
        ERROR_LOG ("fileSize is: %ld", fileSize);

        free (content);

        return NULL;
    }

    return content;
}

char *SkipSpaces (char *buffer)
{
    assert (buffer);

    while (isspace (*buffer))
    {
        DEBUG_LOG ("%d - '%c'", *buffer, *buffer);
        buffer++;
    }

    return buffer;
}

int SafeReadLine (char **str, size_t *len)
{
    size_t bufSize = 0;
    ssize_t res = getline (str, &bufSize, stdin);
    
    DEBUG_LOG ("bufSize of getline = %lu", bufSize);
    DEBUG_LOG ("res of getline = %zd", res);
    
    if (res < 0)
    {
        ERROR_PRINT ("Error reading user input - %s", strerror (errno));
        free (*str);
        
        return COMMON_ERROR_READING_INPUT;
    }

    (*str)[res - 1] = '\0';
    
    *len = (size_t) res;

    return COMMON_ERROR_OK;
}