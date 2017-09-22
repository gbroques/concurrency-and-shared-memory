
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
void process(const int i, char* str);
void critical_section(const int i, char* str);
void remainder_section(void);

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

  printf("Process %d assigned index %d\n", id, index);

  shared_memory = (char*) shmat(list_segment_id, 0, 0);
  flags = (process_state*) shmat(flags_segment_id, NULL, 0);
  turn = (int*) shmat(turn_segment_id, NULL, 0);

  // int turn = shared_memory[turn_index];
  // process_state flags = shared_memory[flags_index];
  // printf("\nflags %d\n", flags);
  // int j;
  // int max_flags_index = flags_index + MAX_PROCESSES * sizeof(process_state);
  // int size_of_process_state = sizeof(process_state);
  // for (j = flags_index; j < max_flags_index; j += size_of_process_state) {
  //   printf("Process #%d state = %d\n", (j - flags_index) / size_of_process_state, shared_memory[j]);
  // }
  // printf("\nTURN = %d\n", turn);
  // exit(EXIT_SUCCESS);

  if (*shared_memory < 0 || *flags < 0 || * turn < 0) {
    fprintf(stderr, "PID: %d\n", getpid());
    perror("palin shmat");
    return EXIT_FAILURE;
  }

  // Process 1: Index = 5 1280 - 2304
  int i;
  int start_index = index * MEMORY_OFFSET;
  int memory_offset_max = MAX_WRITES * MEMORY_OFFSET + start_index;
  for (i = start_index; i < memory_offset_max; i += MEMORY_OFFSET) {
    char* str = &shared_memory[i];
    process(id, str);
  }

  shmdt(shared_memory);
  shmdt(flags);
  shmdt(turn);

  return EXIT_SUCCESS;
}

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

// enum state {
//   idle,
//   want_in,
//   in_cs
// };
// extern int turn;
// extern state flag[n]; /*Flag corresponding to each process in shared memory */

void process(const int i, char* str) {
  // do {
    int n = MAX_PROCESSES;
    int j;
    do {
      flags[i] = want_in; // Raise my flag
      printf("Process %d: Raising my flag\n", i);
      j = *turn; // Set local variable
      // wait until its my turn
      printf("Process %d: Waiting until its my turn...\n", i);
      while (j != i)
        j = (flags[j] != idle) ? *turn : (j + 1) % n;

      printf("Process %d: My turn! Declaring my intention to enter critical section\n", i);

      // Declare intention to enter critical section
      flags[i] = in_cs;
      // Check that no one else is in critical section
      printf("Process %d: Making sure no one else is in their critical section...\n", i);
      for (j = 0; j < n; j++)
        if ((j != i) && (flags[j] == in_cs))
          break;
    } while ((j < n) || (*turn != i && flags[*turn] != idle));
    // Assign turn to self and enter critical section
    *turn = i;
    critical_section(i, str);
    // Exit section
    printf("Process %d: Exiting critical section\n", i);
    j = (*turn + 1) % n;
    while (flags[j] == idle)
      j = (j + 1) % n;

    printf("Process %d: Assigning turn to process %d\n", i, j);

    // Assign turn to next waiting process; change own flag to idle
    *turn = j;
    flags[i] = idle;
    remainder_section();
  // } while (1);
}

void critical_section(const int i, char* str) {
  printf("Process %d: Inside critical section\n", i); 
  // Sleep between 0 and 2 seconds
  sleep(rand() % 3);

  // char* string = &shared_memory[i];
  printf("\nProcess %d: String: %10s\n", i, str);

  if (is_palindrome(str)) {
    printf("Process %d: %s is a palindrome\n", i, str);
  } else {
    printf("Process %d: %s is not a palindrome\n", i, str);
  }

  // Sleep between 0 and 2 second
  sleep(rand() % 3);
}
void remainder_section(void) {
  
}