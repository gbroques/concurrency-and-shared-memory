
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
#include <sys/stat.h>

void free_memory(void);
void read_strings(void);

char* list[256];

/* ARGSUSED */
static void myhandler(int s) {
  // TODO: Free memory
  free_memory();
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

  read_strings();

  // free_memory();

  // int pid, status;
  // pid = fork();

  // if (pid < 0) {
  //   perror("fork()");
  //   return EXIT_FAILURE;
  // }

  // if (pid == 0) { // Child
  //   char procNum[12];
  //   sprintf(procNum, "%d", numProcs);
  //   execlp("palin", "palin", procNum, "0", (char*) NULL);
  //   perror("palin");
  //   _exit(EXIT_FAILURE);
  // }

  // wait(&status);
  // printf("palin terminated with status: %d\n", status);
  return EXIT_SUCCESS;
}

void read_strings(void) {
  int segment_id;
  char** shared_memory;
  struct shmid_ds shmbuffer;
  int segment_size;
  const int shared_segment_size = 0x100;
  int string_segment_ids[256];

  /* Allocate a shared memory segment. */
  segment_id = shmget(IPC_PRIVATE, shared_segment_size,
    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  printf("segment id = %d\n", segment_id);
  /* Attach the shared memory segment. */
  shared_memory = (char**) shmat(segment_id, 0, 0);
  printf("shared memory attached at address %p\n", shared_memory);
  /* Determine the segment’s size. */
  shmctl(segment_id, IPC_STAT, &shmbuffer);
  segment_size = shmbuffer.shm_segsz;
  printf("segment size: %d\n", segment_size);
  /* Write a string to the shared memory segment. */

  printf("Reading input...\n");
  int i = 0;
  int chars_read;
  do {
    string_segment_ids[i] = shmget(IPC_PRIVATE, shared_segment_size,
      IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    shared_memory[i] = (char*) shmat(string_segment_ids[i], NULL, 0);  
    chars_read = scanf("%s", shared_memory[i]);
    if (chars_read > -1) {
      printf("%5d: %12s %10d\n", i, shared_memory[i], string_segment_ids[i]);
      i++;
    }
  } while (chars_read > -1);
  shmdt(shared_memory[i]);
  shmctl(string_segment_ids[i], IPC_RMID, 0);


  int pid, status;
  int num_processes = 1;
  pid = fork();

  if (pid < 0) {
    perror("fork()");
    // return EXIT_FAILURE;
  }

  if (pid == 0) { // Child
    char process_id[12];
    sprintf(process_id, "%d", num_processes);
    char segment_id_string[12];
    sprintf(segment_id_string, "%d", string_segment_ids[0]);
    execlp("palin", "palin", process_id, "0", segment_id_string, (char*) NULL);
    perror("palin");
    _exit(EXIT_FAILURE);
  }

  wait(&status);
  printf("palin terminated with status: %d\n", status);

  printf("\nFreeing memory...\n");

  int j;
  for (j = 0; j < i; j++) {
    printf("%5d: %12s %10d\n", j, shared_memory[j], string_segment_ids[j]);
    shmdt(shared_memory[j]);
    shmctl(string_segment_ids[j], IPC_RMID, 0);
  }

  shmdt(shared_memory);
  printf("Final Segment ID: %d\n", segment_id);

  shmctl(segment_id, IPC_RMID, 0);
}

void free_memory(void) {
  int i = 0;
  while (list[i] != NULL) {
    free(list[i]);
    i++;
  }
}