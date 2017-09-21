
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
void process(const int i, int turn, process_state flags[]);
void critical_section(void);
void remainder_section(void);

int main(int argc, char* argv[]) {
  if (argc != 6) {
    fprintf(stderr, "Usage: %s id xx segment_id flags_index turn_index\n", argv[0]);
    return EXIT_FAILURE;
  }

  srand(time(NULL));
  const int id = atoi(argv[1]);
  const int index = atoi(argv[2]);
  const int segment_id = atoi(argv[3]);
  const int flags_index = atoi(argv[4]);
  const int turn_index = atoi(argv[5]);

  char* shared_memory = (char*) shmat(segment_id, 0, 0);
  int turn = shared_memory[turn_index];
  process_state flags = shared_memory[flags_index];

  if (*shared_memory < 0) {
    fprintf(stderr, "PID: %d\n", getpid());
    perror("palin shmat");
    return EXIT_FAILURE;
  }

  int i;
  int memory_offset_max = MAX_WRITES * MEMORY_OFFSET + index;
  for (i = index; i < memory_offset_max; i += MEMORY_OFFSET) {
    // Sleep between 0 and 2 seconds
    sleep(rand() % 3);

    char* string = &shared_memory[i];
    printf("\nID: %2d Index: %4d Segment ID: %d String: %15s\n", id, i, segment_id, string);

    if (is_palindrome(string)) {
      printf("%s is a palindrome\n", string);
    } else {
      printf("%s is not a palindrome\n", string);
    }

    // Sleep between 0 and 2 second
    sleep(rand() % 3); 
  }

  shmdt(shared_memory);

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

void process(const int i, int turn, process_state flags[]) {
  do {
    int j = turn; // Set local variable
    int n = MAX_PROCESSES;
    do {
      flags[i] = want_in; // Raise my flag

      // wait until its my turn
      while (j != i)
        j = (flags[j] != idle) ? turn : (j + 1) % n;
      // Declare intention to enter critical section
      flags[i] = in_cs;
      // Check that no one else is in critical section
      for (j = 0; j < n; j++)
        if ((j != i) && (flags[j] == in_cs))
          break;
    } while ((j < n) || (turn != i && flags[turn] != idle));
    // Assign turn to self and enter critical section
    turn = i;
    critical_section();
    // Exit section
    j = (turn + 1) % n;
    while (flags[j] == idle)
      j = (j + 1) % n;
    // Assign turn to next waiting process; change own flag to idle
    turn = j;
    flags[i] = idle;
    remainder_section();
  } while (1);
}

void critical_section(void) {

}
void remainder_section(void) {
  
}