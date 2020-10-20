#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static char* path = "/bin";

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        short exit=0;
        char* comand = NULL;
        size_t bytesNumber = 0;
        int readBytes;
        {
            do
            {
                printf("wish> ");
                readBytes = getline(&comand,&bytesNumber,stdin);
                if (readBytes == -1)
                {
                    return(0);
                }else if (readBytes>1)
                {
                    if (strncmp(comand, "exit",4) == 0)
                    {
                        printf("what");
                        exit=1;
                    }
                }
            } while (exit !=1);
        }
    }
    exit(0);
}
