#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
char** copy_2_d_char_arr(int arr_size, int start_index, char** arr);

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
	char** myargs = copy_2_d_char_arr(argc, 1, argv);
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

char** copy_2_d_char_arr(int org_arr_size, int start_index, char** org_arr){
	char** arr = (char**)malloc((org_arr_size-start_index+1)*sizeof(char*));
	for(int i = start_index; i < org_arr_size; i++)
		arr[i-start_index] = strdup(org_arr[i]);
	arr[org_arr_size-start_index] = NULL;
	return arr;
}