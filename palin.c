
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

int is_palindrome(char str[]);

int main(int argc, char* argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s id xx segment_id\n", argv[0]);
    return EXIT_FAILURE;
  }
  int id, index, segment_id;
  id = atoi(argv[1]);
  index = atoi(argv[2]);
  segment_id = atoi(argv[3]);
  char* string = (char*) shmat(segment_id, 0, 0);

  printf("\nID: %d Index: %d Segment ID: %d String: %s\n", id, index, segment_id, string);

  if (is_palindrome(string)) {
    printf("%s is a palindrome\n", string);
  } else {
    printf("%s is not a palindrome\n", string);
  }

  return EXIT_SUCCESS;
}

int is_palindrome(char str[]) {
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