
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
static int read_strings_into_shared_memory(void);
static void fork_children(int strings_read);
static int get_shared_segment_size(void);
static int get_size_of_flags(void);
static int get_size_of_turn(void);
static void get_shared_memory_for_flags(void);
static void get_shared_memory_for_turn(void);
static void fork_and_exec_children(int i, pid_t* children_pids, int string_index);
static void clear_output_files();
static void exec_child_code(int process_number, int shared_memory_index);

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

  if (setup_interval_timer(60*6) == -1) {
    perror("Faled to set up the ITIMER_PROF interval timer");
    return EXIT_FAILURE;
  }

  signal(SIGINT, free_shared_memory_and_abort);

  int strings_read = read_strings_into_shared_memory();

  get_shared_memory_for_flags();

  get_shared_memory_for_turn();

  clear_output_files();

  fork_children(strings_read);

  free_shared_memory();

  return EXIT_SUCCESS;
}

/**
 * Reads strings from stdin into shared memory
 *
 * @return Number of strings read
 */
static int read_strings_into_shared_memory(void) {
  int strings_read = 0;

  int shared_segment_size = get_shared_segment_size();
  list_segment_id = shmget(IPC_PRIVATE, shared_segment_size,
    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

  if (list_segment_id == -1) {
    perror("master shmget");
    exit(EXIT_FAILURE);
  }

  shared_memory = (char*) shmat(list_segment_id, 0, 0);
  if (*shared_memory == -1) {
    perror("master shmat");
    exit(EXIT_FAILURE);
  }

  int i = 0;
  int chars_read;
  do { 
    chars_read = scanf("%s", &shared_memory[i]);
    if (chars_read > -1) {
      strings_read++;
      i += MEMORY_OFFSET;
    }
  } while (chars_read > -1 && strings_read < MAX_STRINGS);
  return strings_read;
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
  shmdt(shared_memory);
  shmctl(list_segment_id, IPC_RMID, 0);
  shmctl(flags_segment_id, IPC_RMID, 0);
  shmctl(turn_segment_id, IPC_RMID, 0);
}

static void fork_children(int strings_read) {
  pid_t children_pids[MAX_PROCESSES];
  int statuses[MAX_PROCESSES];
  int num_processes;
  int strings_processed = 0;
  int num_iters = 0;
  do {
    for (num_processes = 0; num_processes < MAX_PROCESSES; num_processes++) {
      int string_index = (num_processes + (num_iters * MAX_PROCESSES)) * MAX_WRITES;
      if (strings_processed < strings_read) {
        fork_and_exec_children(num_processes, children_pids, string_index);
        strings_processed += MAX_WRITES;
      } else {
        break;
      }
    }
    int k;
    for (k = 0; k < num_processes; k++) {
      waitpid(children_pids[k], &statuses[k], 0);
    }
    num_iters++;
  } while (strings_processed < strings_read);
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

static void fork_and_exec_children(int process_number, pid_t* children_pids, int index) {
  children_pids[process_number] = fork();

  if (children_pids[process_number] == -1) {
    perror("master fork");
    exit(EXIT_FAILURE);
  }

  if (children_pids[process_number] == 0) { // Child
    exec_child_code(process_number, index);
  }
}

static void clear_output_files() {
  fclose(fopen("palin.out", "w"));
  fclose(fopen("nopalin.out", "w"));
}

static void exec_child_code(int process_number, int shared_memory_index) {
    char process_number_string[12];
    sprintf(process_number_string, "%d", process_number);
    char string_index[12];
    sprintf(string_index, "%d", shared_memory_index);
    char list_segment_id_string[12];
    sprintf(list_segment_id_string, "%d", list_segment_id);
    char flags_segment_id_string[12];
    sprintf(flags_segment_id_string, "%d", flags_segment_id);
    char turn_segment_id_string[12];
    sprintf(turn_segment_id_string, "%d", turn_segment_id);

    execlp(
      "palin",
      "palin",
      process_number_string,
      string_index,
      list_segment_id_string,
      flags_segment_id_string,
      turn_segment_id_string,
      (char*) NULL
    );
    perror("palin");
    _exit(EXIT_FAILURE);
}