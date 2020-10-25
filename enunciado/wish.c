#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

char **path;
int pathLen = 0;
static char error_message[25] = "An error has occurred\n";

int consecutiveSpaces(char *input, int startIndex, short reverse)
{
    int spaces = 0;
    int i = startIndex;
    char actualChar = input[i];
    while (actualChar == ' ' && (i < strlen(input) && i >= 0))
    {
        spaces++;
        if (reverse == 0)
        {
            i++;
        }
        else
        {
            i--;
        }

        actualChar = input[i];
    }

    return spaces;
}

char *subString(const char *input, int offset, int len, char *dest)
{
    int input_len = strlen(input);

    if (offset + len > input_len)
    {
        return NULL;
    }
    dest[len] = '\0';
    strncpy(dest, input + offset, len);
    return dest;
}

void comandPath(char *comand)
{
    for (size_t i = 0; i < pathLen; i++)
    {
        free(path[i]);
        pathLen = 0;
    }
    char *startPath = strchr(comand, ' ');
    int startPathIndex = (startPath == NULL ? -1 : startPath - comand);
    char *acomulated = (char *)malloc(128);
    int nextLen;
    int spaces;
    if (startPathIndex == -1)
    {
        return;
    }
    else
    {
        spaces = consecutiveSpaces(comand, startPathIndex, 0);
        nextLen = strlen(comand) - startPathIndex - spaces;
        subString(comand, startPathIndex + spaces, nextLen, acomulated);
    }
    do
    {
        pathLen++;
        startPath = strchr(acomulated, ' ');
        startPathIndex = (startPath == NULL ? -1 : startPath - acomulated);
        if (startPathIndex != -1)
        {
            char nextPath[startPathIndex];
            subString(acomulated, 0, startPathIndex, nextPath);
            path = (char **)realloc(path, (pathLen) * sizeof(char *));
            path[pathLen - 1] = strdup(nextPath);
            spaces = consecutiveSpaces(comand, startPathIndex, 0);
            nextLen = strlen(acomulated) - startPathIndex - spaces;
            char temp[strlen(acomulated)];
            strcpy(temp, acomulated);
            acomulated = (char *)realloc(acomulated, startPathIndex);
            subString(temp, startPathIndex + spaces, nextLen, acomulated);
        }
        else
        {
            path = (char **)realloc(path, (pathLen) * sizeof(char *));
            path[pathLen - 1] = strdup(acomulated);
        }
    } while (startPathIndex > -1);
    free(acomulated);
}

void comandCD(char *comand)
{
    char *startPath = strchr(comand, ' ');
    int startPathIndex = (startPath == NULL ? -1 : startPath - comand);
    char *endPath = strrchr(comand, ' ');
    int endPathIndex = (endPath == NULL ? -1 : endPath - comand);
    if ((startPathIndex > -1) && (endPathIndex == startPathIndex))
    {
        int pathLen = strlen(comand) - startPathIndex + 1;
        char cdArg[pathLen - 1];
        char cdPath[pathLen];
        cdPath[0] = '\0';
        subString(comand, startPathIndex + 1, pathLen - 2, cdArg);
        strcat(cdPath, cdArg);
        int cdSuccess = chdir(cdPath);
        if (cdSuccess < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
}

char **getArguments(char *comand, char *lPath, int *argSize)
{
    char **arguments = NULL;
    int numArgs = 0;
    char *argEndPoint;
    int argEndIndex;
    char acomulated[128];
    char arg[64];
    char *temp;
    int nextLen;
    int spaces;
    do
    {

        arguments = (char **)realloc(arguments, (numArgs + 1) * sizeof(char *));
        if (numArgs > 0)
        {
            argEndPoint = strchr(acomulated, ' ');
            argEndIndex = (argEndPoint == NULL ? -1 : argEndPoint - acomulated);
            if (argEndIndex > -1)
            {
                spaces = consecutiveSpaces(acomulated, argEndIndex, 0);
                subString(acomulated, 0, argEndIndex, arg);
                arguments[numArgs] = strdup(arg);
                numArgs++;
                nextLen = strlen(acomulated) - argEndIndex - spaces;
                temp = (char *)malloc(strlen(acomulated));
                strcpy(temp, acomulated);
                subString(temp, argEndIndex + spaces, nextLen, acomulated);
                free(temp);
            }
            else
            {
                arguments[numArgs] = strdup(acomulated);
                numArgs++;
            }
        }
        else
        {
            argEndPoint = strchr(comand, ' ');
            argEndIndex = (argEndPoint == NULL ? -1 : argEndPoint - comand);
            if (argEndIndex > -1)
            {
                spaces = consecutiveSpaces(comand, argEndIndex, 0);
                int fullPathLen = strlen(lPath) + argEndIndex;
                char fullPath[fullPathLen + 1];
                strcpy(fullPath, lPath);
                subString(comand, 0, argEndIndex, arg);
                strcat(fullPath, "/");
                strcat(fullPath, arg);
                arguments[numArgs] = strdup(fullPath);
                numArgs++;
                nextLen = strlen(comand) - argEndIndex - spaces;
                subString(comand, argEndIndex + spaces, nextLen, acomulated);
            }
            else
            {
                int fullPathLen = strlen(lPath) + strlen(comand);
                char fullPath[fullPathLen + 1];
                strcpy(fullPath, lPath);
                strcat(fullPath, "/");
                strcat(fullPath, comand);
                arguments[numArgs] = strdup(fullPath);
                numArgs++;
            }
        }

    } while (argEndPoint != NULL);
    arguments = (char **)realloc(arguments, (numArgs + 1) * sizeof(char *));
    arguments[numArgs] = NULL;
    numArgs++;
    *argSize = numArgs;
    return arguments;
}

void freeArguments(char **arguments, int argSize)
{
    for (size_t i = 0; i < argSize; i++)
    {
        free(arguments[i]);
    }
}

void executeComand(char *comand)
{
    size_t i = 0;
    short executed = 0;
    char *initCommand;
    char *redirectFile;
    char *argEndPoint = strchr(comand, '>');
    int redirectIndex = (argEndPoint == NULL ? -1 : argEndPoint - comand);
    if (redirectIndex == -1)
    {
        initCommand = (char *)malloc(strlen(comand));
        strcpy(initCommand, comand);
    }
    else if (redirectIndex == 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
    else
    {
        int spaces = consecutiveSpaces(comand, redirectIndex - 1, 1);
        initCommand = (char *)malloc(redirectIndex - spaces);
        subString(comand, 0, redirectIndex - spaces, initCommand);
        spaces = consecutiveSpaces(comand, redirectIndex + 1, 0);
        int fileNameLen = strlen(comand) - redirectIndex - spaces - 1;
        if (fileNameLen == 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
        redirectFile = (char *)malloc(fileNameLen);
        subString(comand, redirectIndex + spaces + 1, fileNameLen, redirectFile);
    }

    while ((i < pathLen) && (executed == 0))
    {
        int argSize;
        char **arguments = getArguments(initCommand, path[i], &argSize);
        int rc = fork();
        if (rc < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
        if (rc == 0)
        {
            if (redirectIndex != -1)
            {
                int fd = open(redirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, 1);
                close(fd);
            }
            int successs = execv(arguments[0], arguments);
            if (successs < 0)
            {
                freeArguments(arguments, argSize);
                exit(1);
            }
        }
        else
        {
            int exitStatus;
            wait(&exitStatus);
            if (exitStatus == 0)
            {
                executed = 1;
            }
        }
        i++;
    }
    if (executed == 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
};

int selectComand(char *comand, int paralel)
{
    int exit = 0;
    if (strncmp(comand, "exit", 4) == 0)
    {
        if (strlen(comand) < 5)
        {
            exit = 1;
        }
        else
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    else if (strncmp(comand, "cd", 2) == 0)
    {
        comandCD(comand);
    }
    else if (strncmp(comand, "path", 4) == 0)
    {
        comandPath(comand);
    }
    else
    {
        executeComand(comand);
    }
    return exit;
}

int main(int argc, char const *argv[])
{
    pathLen = 0;
    path = (char **)realloc(path, (pathLen + 1) * sizeof(char *));
    path[pathLen] = strdup("/bin");
    pathLen++;
    if (argc < 2)
    {
        short exit = 0;
        char *comand;
        size_t bytesNumber = 0;
        int readBytes;
        {
            do
            {
                printf("wish> ");
                readBytes = getline(&comand, &bytesNumber, stdin);
                char realComand[readBytes];
                strncpy(realComand, comand, readBytes - 1);
                realComand[readBytes - 1] = '\0';
                if (readBytes == -1)
                {
                    return (0);
                }
                else if (readBytes > 1)
                {

                    FILE *file = fopen(realComand, "r");
                    if (file == NULL)
                    {
                        exit = selectComand(realComand, 0);
                    }
                    else
                    {
                        char *line;
                        size_t len = 0;
                        ssize_t read;
                        while ((read = getline(&line, &len, file)) != -1)
                        {
                            char realLine[read];
                            if (strchr(line, '\n') != NULL)
                            {
                                strncpy(realLine, line, read - 1);
                                realLine[read - 1] = '\0';
                            }
                            else
                            {
                                strncpy(realLine, line, read);
                            }
                            
                            exit = selectComand(realLine, 0);
                        }
                        fclose(file);
                    }
                }
            } while (exit != 1);
        }
    }
    exit(0);
}
