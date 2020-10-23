#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

static char *path[] = {
    "/bin/",
};
static char error_message[25] = "An error has occurred\n";

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

void comandCD(char *comand)
{
    char *startPath = strchr(comand, ' ');
    int startPathIndex = (startPath == NULL ? -1 : startPath - comand);
    char *endPath = strrchr(comand, ' ');
    int endPathIndex = (endPath == NULL ? -1 : endPath - comand);
    if ((startPathIndex > -1) && (endPathIndex == startPathIndex))
    {
        int pathLen = strlen(comand) - startPathIndex + 1;
        char cdArg[pathLen-1];
        char cdPath[pathLen];
        cdPath[0]='\0';
        subString(comand, startPathIndex + 1, pathLen-2, cdArg);
        strcat(cdPath,cdArg);
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

char **getArguments(char *comand)
{
    char **arguments = NULL;
    char numArgs = 1;
    char *argEndPoin;
    do
    {
        argEndPoin = strchr(comand, ' ');
        int argEndIndex = (argEndPoin == NULL ? -1 : argEndPoin - comand);
        if (numArgs > 1)
        {
        }
        else
        {
            arguments = (char **)realloc(arguments, numArgs * sizeof(char *));
            if (argEndIndex > -1)
            {
                /* code */
            }
            else
            {
                int fullPathLen = strlen(path[0]) + strlen(comand);
                char fullPath[fullPathLen - 1];
                strcpy(fullPath, path[0]);
                strcat(fullPath, comand);
                arguments[numArgs - 1] = (char *)realloc(arguments[0], fullPathLen);
                strcpy(arguments[0], fullPath);
                numArgs++;
            }
        }

    } while (argEndPoin != NULL);
    return arguments;
}

void executeComand(char *comand)
{
    char **arguments = getArguments(comand);
    int rc = fork();
    if (rc < 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
    if (rc == 0)
    {
        int successs=execv(arguments[0], arguments);
        if (successs<0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        }
        
    }
    else
    {
        wait(NULL);
        return;
    }
};

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        short exit = 0;
        char *comand = NULL;
        size_t bytesNumber = 0;
        int readBytes;
        {
            do
            {
                printf("wish> ");
                readBytes = getline(&comand, &bytesNumber, stdin);
                int comandLen = strlen(comand);
                char realComand[comandLen];
                strncpy(realComand, comand, comandLen - 1);
                realComand[comandLen - 1] = '\0';
                if (readBytes == -1)
                {
                    return (0);
                }
                else if (readBytes > 1)
                {
                    if (strncmp(realComand, "exit", 4) == 0)
                    {
                        if (strlen(realComand) < 5)
                        {
                            exit = 1;
                        }
                        else
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                    }
                    else if (strncmp(realComand, "cd", 2) == 0)
                    {
                        comandCD(realComand);
                    }
                    else
                    {
                        executeComand(realComand);
                    }
                }
            } while (exit != 1);
        }
    }
    exit(0);
}
