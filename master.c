
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
#include "master.h"

static int setup_interval_timer(int time);
static int setup_interrupt(void);
static void free_shared_memory(void);
static void free_shared_memory_and_abort(int s);
static void read_strings_into_shared_memory(void);
static void fork_children(void);
static int get_shared_segment_size(void);
static int get_size_of_flags(void);
static int get_size_of_turn(void);
static void get_shared_memory_for_flags(void);
static void get_shared_memory_for_turn(void);
static void execute_child_code(int i);

static const int MAX_STRINGS = 256;

static int list_segment_id;
static int flags_segment_id;
static int turn_segment_id;
static char* shared_memory;

int main(void) {
  if (setup_interrupt() == -1) {
    perror("Failed to set up handler for SIGPROF");
    return EXIT_FAILURE;
  }

  if (setup_interval_timer(60) == -1) {
    perror("Faled to set up the ITIMER_PROF interval timer");
    return EXIT_FAILURE;
  }

  signal(SIGINT, free_shared_memory_and_abort);

  read_strings_into_shared_memory();

  get_shared_memory_for_flags();

  get_shared_memory_for_turn();

  fork_children();

  free_shared_memory();

  return EXIT_SUCCESS;
}

/**
 * Reads strings from stdin into shared memory
 */
static void read_strings_into_shared_memory(void) {
  struct shmid_ds shmbuffer;
  int segment_size;

  /* Allocate a shared memory segment. */
  int shared_segment_size = get_shared_segment_size();
  list_segment_id = shmget(IPC_PRIVATE, shared_segment_size,
    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

  if (list_segment_id == -1) {
    perror("master shmget");
    exit(EXIT_FAILURE);
  }
  printf("segment id = %d\n", list_segment_id);
  /* Attach the shared memory segment. */
  shared_memory = (char*) shmat(list_segment_id, 0, 0);
  if (*shared_memory == -1) {
    perror("master shmat");
    exit(EXIT_FAILURE);
  }
  printf("shared memory attached at address %p\n", shared_memory);
  /* Determine the segment’s size. */
  shmctl(list_segment_id, IPC_STAT, &shmbuffer);
  segment_size = shmbuffer.shm_segsz;
  printf("segment size: %d\n", segment_size);
  /* Write a string to the shared memory segment. */
  printf("Reading input...\n");
  int i = 0;
  int chars_read;
  do { 
    chars_read = scanf("%s", &shared_memory[i]);
    if (chars_read > -1) {
      printf("%5d: %12s\n", i, &shared_memory[i]);
      i += MEMORY_OFFSET;
    }
  } while (chars_read > -1);
}

/**
 * Free shared memory and abort program
 */
static void free_shared_memory_and_abort(int s) {
  free_shared_memory();
  abort();
}

static int setup_interrupt(void) {
  struct sigaction act;
  act.sa_handler = free_shared_memory_and_abort;
  act.sa_flags = 0;
  return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}

/**
 * Sets up an interval timer
 *
 * @param time The duration of the timer
 *
 * @return Zero on success. -1 on error.
 */
static int setup_interval_timer(int time) {
  struct itimerval value;
  value.it_interval.tv_sec = time;
  value.it_interval.tv_usec = 0;
  value.it_value = value.it_interval;
  return (setitimer(ITIMER_PROF, &value, NULL));
}

/**
 * Frees all allocated shared memory
 */
static void free_shared_memory(void) {
  printf("\nFreeing shared memory...\n");
  shmdt(shared_memory);
  shmctl(list_segment_id, IPC_RMID, 0);

  shmctl(flags_segment_id, IPC_RMID, 0);

  shmctl(turn_segment_id, IPC_RMID, 0);
}

static void fork_children(void) {
  int children_pids[MAX_PROCESSES];
  int pid, status;
  int num_processes;
  int total_processes = 3;
  for (num_processes = 1; num_processes < total_processes; num_processes++) {
    children_pids[num_processes] = fork();
    printf("num_processes: %d\n", num_processes);


    if (children_pids[num_processes] == -1) {
      perror("master fork");
      exit(EXIT_FAILURE);
    }

    if (children_pids[num_processes] == 0) { // Child
      char process_id[12];
      sprintf(process_id, "%d", num_processes - 1);
      char index[12];
      sprintf(index, "%d", num_processes * MAX_WRITES - MAX_WRITES);
      char list_segment_id_string[12];
      sprintf(list_segment_id_string, "%d", list_segment_id);
      char flags_segment_id_string[12];
      sprintf(flags_segment_id_string, "%d", flags_segment_id);
      char turn_segment_id_string[12];
      sprintf(turn_segment_id_string, "%d", turn_segment_id);
      printf("Process ID: %d\n", num_processes - 1);
      execlp(
        "palin",
        "palin",
        process_id,
        index,
        list_segment_id_string,
        flags_segment_id_string,
        turn_segment_id_string,
        (char*) NULL
      );
      perror("palin");
      _exit(EXIT_FAILURE);
      // execute_child_code(num_processes);
    }
  }

  int k;
  for (k = 1; k < 3; k++) {
    waitpid(children_pids[k], &status, 0);
    printf("palin terminated with status: %d\n", status);
  }


  printf("master terminated\n");
}

/**
 * Returns the size of the segment in shared memory
 * 
 * @return The size of the segment in shared memory
 */
static int get_shared_segment_size(void) {
  return MEMORY_OFFSET * MAX_STRINGS;
}

/**
 * Returns the size of the flags in shared memory
 * 
 * @return The size of the flags
 */
static int get_size_of_flags(void) {
  return sizeof(process_state) * MAX_PROCESSES;
}

/**
 * Returns the size of the turn variable in shared memory
 * 
 * @return The size of the turn
 */
static int get_size_of_turn(void) {
  return sizeof(int);
}

static void get_shared_memory_for_flags(void) {
  int shared_segment_size = get_size_of_flags();
  flags_segment_id = shmget(IPC_PRIVATE, shared_segment_size,
    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
}

static void get_shared_memory_for_turn(void) {
  int shared_segment_size = get_size_of_turn();
  turn_segment_id = shmget(IPC_PRIVATE, shared_segment_size,
    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
}

static void execute_child_code(int i) {
    // char process_id[12];
    // sprintf(process_id, "%d", i);
    // char index[12];
    // sprintf(index, "%d", i * MAX_WRITES - MAX_WRITES);
    // char list_segment_id_string[12];
    // sprintf(list_segment_id_string, "%d", list_segment_id);
    // char flags_segment_id_string[12];
    // sprintf(flags_segment_id_string, "%d", flags_segment_id);
    // char turn_segment_id_string[12];
    // sprintf(turn_segment_id_string, "%d", turn_segment_id);

    // execlp(
    //   "palin",
    //   "palin",
    //   process_id - 1,
    //   index,
    //   list_segment_id_string,
    //   flags_segment_id_string,
    //   turn_segment_id_string,
    //   (char*) NULL
    // );
    // perror("palin");
    // _exit(EXIT_FAILURE);
}