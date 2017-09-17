
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>


char* list[256];
int numProcs = 1;

void freeMemory(void);
void readStrings(void);


/* ARGSUSED */
static void myhandler(int s) {
  // TODO: Free memory
  freeMemory();
  abort();
}

static int setupinterrupt(void) {
  /* set up myhandler for SIGPROF */
  struct sigaction act;
  act.sa_handler = myhandler;
  act.sa_flags = 0;
  return (sigemptyset( & act.sa_mask) || sigaction(SIGPROF, & act, NULL));
}

static int setupitimer(void) {
  /* set ITIMER_PROF for 60-second interval */
  struct itimerval value;
  value.it_interval.tv_sec = 60;
  value.it_interval.tv_usec = 0;
  value.it_value = value.it_interval;
  return (setitimer(ITIMER_PROF, & value, NULL));
}


int main(void) {
  if (setupinterrupt() == -1) {
    perror("Failed to set up handler for SIGPROF");
    return EXIT_FAILURE;
  }

  if (setupitimer() == -1) {
    perror("Faled to set up the ITIMER_PROF interval timer");
    return EXIT_FAILURE;
  }

  signal(SIGINT, myhandler);

  // readStrings();

  // freeMemory();

  int pid, status;
  pid = fork();

  if (pid < 0) {
    perror("fork()");
    return EXIT_FAILURE;
  }

  if (pid == 0) { // Child
    char procNum[12];
    sprintf(procNum, "%d", numProcs);
    execlp("palin", "palin", procNum, "0", (char*) NULL);
    perror("palin");
    _exit(EXIT_FAILURE);
  }

  wait(&status);
  printf("palin terminated with status: %d\n", status);
  return EXIT_SUCCESS;
}

void readStrings(void) {
  int i = 0;
  list[i] = (char*) malloc(256*sizeof(char));
  while (scanf("%s", list[i]) > -1) {
    int j, len;
    for (j = 0, len = strlen(list[i]); j < len; j++) {
      printf("%c", list[i][j]);
    }
    printf("\n");
    i++;
    list[i] = (char*) malloc(256*sizeof(char));
  }
}

void freeMemory(void) {
  int i = 0;
  while (list[i] != NULL) {
    free(list[i]);
    i++;
  }
}
