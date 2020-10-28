#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

char **path;
int pathLen = 0;
static char error_message[25] = "An error has occurred\n";


//devuelve la cantidad de espacios consecutivos en un string a partir de un index dado
//en caso de reverse == 1 se retrocederá en vez de avanzar en el string para contar
int consecutiveSpaces(char *input, int startIndex, short reverse)
{
    int spaces = 0;
    int i = startIndex;
    char actualChar = input[i];
    while ((actualChar == ' ' || actualChar == '\t' || actualChar == '\n') && (i < strlen(input) && i >= 0))
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
    char *acomulated = (char *)malloc(strlen(comand));
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
            spaces = consecutiveSpaces(acomulated, startPathIndex, 0);
            nextLen = strlen(acomulated) - startPathIndex - spaces;
            char temp[strlen(acomulated)];
            strcpy(temp, acomulated);
            acomulated = (char *)realloc(acomulated, nextLen);
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


//devuelve una lista de strings con los argumentos de un comando no integrado
char **getArguments(char *comand, int *argSize)
{
    char **arguments = NULL;
    int numArgs = 0;
    char *argEndPoint;
    int argEndIndex;
    char acomulated[strlen(comand)];
    char arg[64];
    char *temp;
    int nextLen;
    int spaces;
    strcpy(acomulated, comand);
    do
    {
        arguments = (char **)realloc(arguments, (numArgs + 1) * sizeof(char *));
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
        else if (strlen(acomulated) > 1)
        {
            arguments[numArgs] = strdup(acomulated);
            numArgs++;
        }
    } while (argEndPoint != NULL);
    arguments = (char **)realloc(arguments, (numArgs + 1) * sizeof(char *));
    arguments[numArgs] = NULL;
    numArgs++;
    *argSize = numArgs;
    return arguments;
}

//libera el espacio ocupado por los argumentos de un comando
void freeArguments(char **arguments, int argSize)
{
    for (size_t i = 0; i < argSize; i++)
    {
        free(arguments[i]);
    }
}

//ejecuta todos los comandos no integrados
void executeComand(char *comand, int paralel)
{
    size_t i = 0;
    short executed = 0;
    short commandNoFound = 1;

    while ((i < pathLen) && (executed == 0))
    {
        int argSize;
        char **arguments = getArguments(comand, &argSize);
        int rc = 0;

        if (paralel == 0)
        {
            rc = fork();
        }
        else if (rc < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
        if (rc == 0)
        {
            //obtengo la direccion del ejecutable
            int fullPathLen = strlen(path[i]) + strlen(arguments[0]);
            char fullPath[fullPathLen + 1];
            strcpy(fullPath, path[i]);
            strcat(fullPath, "/");
            strcat(fullPath, arguments[0]);
            int successs = execv(fullPath, arguments);
            if (successs < 0 && paralel == 0)
            {
                exit(1);
            }
        }
        else
        {
            int exitStatus;
            waitpid(rc, &exitStatus, 0);
            if (exitStatus == 0)
            {
                executed = 1;
                commandNoFound = 0;
            }
            else if (exitStatus != 256)
            {
                commandNoFound = 0;
            }
        }
        i++;
    }
    if (commandNoFound == 1)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
};

//segun el comando redirecciona al metodo correspondiente si es integrado o a la ejecucion general si no lo es
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
        executeComand(comand, paralel);
    }
    return exit;
}


//Se encarga de ver si el comando ingresado corresponde a un archivo o a un comando de verdad
//ademas verifica si hay redireccion de la salida para ejecutarla
int executeFileOrComand(char *comand, int paralel)
{
    int exitConsole = 0;

    char *initCommand;
    char *redirectFile;

    //busco el signo de redireccion para saber si es necesario realizarla
    char *redicetionPint = strchr(comand, '>');
    int redirectIndex = (redicetionPint == NULL ? -1 : redicetionPint - comand);
    int console = dup(1);
    if (redirectIndex == -1)
    {
        initCommand = (char *)malloc(strlen(comand));
        strcpy(initCommand, comand);
    }
    else if (redirectIndex == 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return exitConsole;
    }
    else
    {
        //si hay redireccion separo el comando del archivo destino
        int spaces = consecutiveSpaces(comand, redirectIndex - 1, 1);
        //error si no hay comando
        if (redirectIndex - spaces == 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
        initCommand = (char *)malloc(redirectIndex - spaces);
        subString(comand, 0, redirectIndex - spaces, initCommand);
        spaces = consecutiveSpaces(comand, redirectIndex + 1, 0);
        int fileNameLen = strlen(comand) - redirectIndex - spaces - 1;
        //error si no hay archivo destino
        if (fileNameLen == 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
        redirectFile = (char *)malloc(fileNameLen);
        subString(comand, redirectIndex + spaces + 1, fileNameLen, redirectFile);
        char *error = strchr(redirectFile, ' ');
        //error si existe otro argumento despues de la redirección
        if (error != NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
        //error si hay mas de una redirección 
        error = strchr(redirectFile, '>');
        if (error != NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
    }
    FILE *file = fopen(initCommand, "r");

    if (file == NULL)
    {
        //si hay redirección se desvia a la salida al archivo destino
        if (redirectIndex != -1)
        {
            int fd = open(redirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            dup2(fd, 1);

            close(fd);
        }
        exitConsole = selectComand(initCommand, paralel);
        //despues de imprimir el comando se reestablece la salida a la consola
        if (redirectIndex != -1)
        {
            dup2(console, 1);
            close(console);
        }
    }
    else
    {
        char *line;
        size_t len = 0;
        ssize_t read;
        //si hay redirección se desvia a la salida al archivo destino
        if (redirectIndex != 1)
        {
            int fd = open(redirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            dup2(fd, 1); 

            close(fd);
        }
        while ((read = getline(&line, &len, file)) != -1)
        {
            if (strchr(line, '#') != NULL)
            {
                continue;
            }
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
            exitConsole = selectComand(realLine, 0);
        }
        fclose(file);
        //despues de imprimir el comando se reestablece la salida a la consola
        if (redirectIndex != -1)
        {
            dup2(console, 1);
            close(console);
        }
        if (paralel != 0)
        {
            exit(0);
        }
    }
    return exitConsole;
}


//verifica si hay un llamado que implique paralelismo
int execute(char *comand)
{
    int exitBash = 0;
    char *paralelPoint = strchr(comand, '&');
    int paralelIndex = (paralelPoint == NULL ? -1 : paralelPoint - comand);
    if (paralelIndex == -1)
    {
        exitBash = executeFileOrComand(comand, 0);
    }
    else if (paralelIndex > 1)
    {
        int acomulatedLen = strlen(comand);
        char acomulated[acomulatedLen];
        char temp[acomulatedLen];
        strcpy(acomulated, comand);
        int comandLen;
        int spaces;
        int pSize = 0;
        for (size_t i = 0; i < acomulatedLen; i++)
        {
            if (acomulated[i] == '&')
            {
                pSize++;
            }
        }
        int pids[pSize + 1];
        int pNumber = 0;
        do
        {
            paralelPoint = strchr(acomulated, '&');
            paralelIndex = (paralelPoint == NULL ? -1 : paralelPoint - acomulated);
            if (paralelIndex != -1)
            {
                spaces = consecutiveSpaces(acomulated, paralelIndex - 1, 1);
                comandLen = paralelIndex - spaces;
            }
            else
            {
                comandLen = acomulatedLen;
            }
            char paralelComand[comandLen];
            subString(acomulated, 0, comandLen, paralelComand);
            if (paralelIndex != -1)
            {
                strcpy(temp, acomulated);
                spaces = consecutiveSpaces(acomulated, paralelIndex + 1, 0);
                acomulatedLen = acomulatedLen - paralelIndex - spaces - 1;
                subString(temp, paralelIndex + spaces + 1, acomulatedLen, acomulated);
            }
            int rc = fork();

            if (rc == -1)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else if (rc == 0)
            {
                executeFileOrComand(paralelComand, 1);
            }
            else
            {
                pids[pNumber] = rc;
                pNumber++;
            }

        } while (paralelIndex != -1);
        int status;
        for (size_t i = 0; i < pNumber; i++)
        {
            waitpid(pids[i], &status, 0);
        }
    }
    return exitBash;
}

int main(int argc, char const *argv[])
{
    //path inicial
    pathLen = 0;
    path = (char **)realloc(path, (pathLen + 1) * sizeof(char *));
    path[pathLen] = strdup("/bin");
    pathLen++;
    //señal para salir de la shell
    short exitBash = 0;
    //comprobar si no hay mas argumentos para entrar en el modo bash
    if (argc < 2)
    {
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
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (readBytes > 1)
                {
                    exitBash = execute(realComand);
                }
            } while (exitBash == 0);
        }
    }
    //en caso de que si halla otro argumento se intentara abrir el archivo para ejecutar las instrucciones
    else if (argc == 2)
    {

        FILE *file = fopen(argv[1], "r");
        if (file != NULL)
        {
            char *line;
            size_t len = 0;
            ssize_t read;
            char *spacePoint;
            int spaceIndex;
            int spaces = 0;
            while (((read = getline(&line, &len, file)) != -1) && (exitBash == 0))
            {
                //se omite el simbolo # que es usado como comentario
                if (strchr(line, '#') != NULL)
                {
                    continue;
                }

                spacePoint = strchr(line, ' ');
                spaceIndex = (spacePoint == NULL ? -1 : spacePoint - line);
                if (spaceIndex == 0)
                {
                    spaces = consecutiveSpaces(line, spaceIndex, 0);
                }

                char realLine[read - spaces];
                if (read - spaces < 1)
                {
                    continue;
                }
                if (strchr(line, '\n') != NULL)
                {
                    subString(line, spaces, read - spaces - 1, realLine);
                    realLine[read - spaces - 1] = '\0';
                }
                else
                {
                    strncpy(realLine, line, read);
                }
                exitBash = execute(realLine);
            }
            fclose(file);
        }
        else
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    exit(0);
}
