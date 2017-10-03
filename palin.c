
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "master.h"

static int is_palindrome(char str[]);
void process(const int i, int index);
void critical_section(const int i, int index);
void get_timestamp(char* timestamp, int buffer_size);

char* shared_memory;
process_state* flags;
int* turn;

int main(int argc, char* argv[]) {
  if (argc != 6) {
    fprintf(stderr, "Usage: %s id xx list_segment_id flags_index turn_index\n", argv[0]);
    return EXIT_FAILURE;
  }

  srand(time(NULL));
  const int id = atoi(argv[1]);
  const int index = atoi(argv[2]);
  const int list_segment_id = atoi(argv[3]);
  const int flags_segment_id = atoi(argv[4]);
  const int turn_segment_id = atoi(argv[5]);

  shared_memory = (char*) shmat(list_segment_id, 0, 0);
  flags = (process_state*) shmat(flags_segment_id, NULL, 0);
  turn = (int*) shmat(turn_segment_id, NULL, 0);

  if (*shared_memory < 0 || *flags < 0 || * turn < 0) {
    fprintf(stderr, "PID: %d\n", getpid());
    perror("palin shmat");
    return EXIT_FAILURE;
  }

  int i;
  int start_index = index * MEMORY_OFFSET;
  int memory_offset_max = MAX_WRITES * MEMORY_OFFSET + start_index;
  for (i = start_index; i < memory_offset_max; i += MEMORY_OFFSET) {
    char* str = &shared_memory[i];
    if (strlen(str) == 0) {
      shmdt(shared_memory);
      shmdt(flags);
      shmdt(turn);
      return 0;
    }
    process(id, i);
  }

  shmdt(shared_memory);
  shmdt(flags);
  shmdt(turn);

  return EXIT_SUCCESS;
}

/**
 * Determines wheter a string is a palindrome.
 *
 * @param str The string to be tested.
 *
 * @return 1 if palindrome. 0 if not palindrome.
 */
static int is_palindrome(char str[]) {
    // Start from leftmost and rightmost corners of str
    int l = 0;
    int h = strlen(str) - 1;
 
    // Keep comparing characters while they are same
    while (h > l) {
        if (str[l++] != str[h--]) {
          return 0;
        }
    }
    return 1;
}

void process(const int i, int index) {
  int buffer_size = 80;
  char timestamp[buffer_size];
  int n = MAX_PROCESSES;
  int j;
  do {
    flags[i] = want_in; // Raise my flag
    j = *turn; // Set local variable
    // Wait until its my turn
    while (j != i)
      j = (flags[j] != idle) ? *turn : (j + 1) % n;

    get_timestamp(timestamp, buffer_size);
    fprintf(stderr, "[%s] PID %2d: Declaring intention to enter critical section\n", timestamp, i);

    // Declare intention to enter critical section
    flags[i] = in_cs;

    // Check that no one else is in critical section
    for (j = 0; j < n; j++)
      if ((j != i) && (flags[j] == in_cs))
        break;
  } while ((j < n) || (*turn != i && flags[*turn] != idle));
  // Assign turn to self and enter critical section
  *turn = i;
  critical_section(i, index);
  // Exit section
  j = (*turn + 1) % n;
  while (flags[j] == idle)
    j = (j + 1) % n;

  get_timestamp(timestamp, buffer_size);
  fprintf(stderr, "[%s] PID %2d: Exiting critical section\n", timestamp, i);

  // Assign turn to next waiting process; change own flag to idle
  *turn = j;
  flags[i] = idle;
}

void critical_section(const int process_number, int index) {
  int buffer_size = 80;
  char timestamp[buffer_size];
  get_timestamp(timestamp, buffer_size);
  fprintf(stderr, "[%s] PID %2d: Entering critical section\n", timestamp, process_number);
  char* str = &shared_memory[index];
  // Sleep between 0 and 2 seconds
  sleep(rand() % 3);

  FILE *fp;
  char output[256];
  int file_index = index / MEMORY_OFFSET + 1;
  get_timestamp(timestamp, buffer_size);
  if (is_palindrome(str)) {
    fp = fopen("palin.out", "a+");
    fprintf(stderr, "\n[%s] PID %2d: %3d %15s PALINDROME\n\n", timestamp, process_number, file_index, str);
    sprintf(output, "%5d %3d %20s\n", getpid(), file_index, str);
    fputs(output, fp);
  } else {
    fp = fopen("nopalin.out", "a+");
    fprintf(stderr, "\n[%s] PID %2d: %3d %15s NOT PALINDROME\n\n", timestamp, process_number, file_index, str);
    sprintf(output, "%5d %3d %20s\n", getpid(), file_index, str);
    fputs(output, fp);
  }

  fclose(fp);

  // Sleep between 0 and 2 second
  sleep(rand() % 3);
}

void get_timestamp(char* timestamp, int buffer_size) {
  time_t raw_time;
  struct tm* time_info;

  time(&raw_time);
  time_info = localtime(&raw_time);
  strftime(timestamp, buffer_size, "%I:%M:%S %p", time_info); 
}