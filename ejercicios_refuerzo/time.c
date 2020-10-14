#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>


int main(int argc, char *argv[]) {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  time_t starting;
  time_t endind;
  starting = current_time.tv_usec;
  int rc = fork();
  if (rc < 0) {
    // fork failed; exit
    fprintf(stderr, "fork failed\n");
    exit(1);
  } else if (rc == 0) {
    // child (new process)
    char *myargs[3];
    myargs[0] = strdup(argv[1]);   // Comando ingresado

    if (argc > 2){
        myargs[1] = strdup(argv[2]); // Si un comando necesita dos argumentos
    }else
    {
       myargs[1] = NULL;
    }
    myargs[2] = NULL;       
    execvp(myargs[0], myargs); 
    printf("this shouldn't print out");
  } else {
    // parent goes down this path (original process)
    wait(NULL); // hasta que no se ejecute el hijo no salimos
    gettimeofday(&current_time, NULL);
    endind = current_time.tv_usec;
    time_t timeT = endind - starting;
    printf("El proceso duro: %ld \n", timeT);
  }
  return 0;
}