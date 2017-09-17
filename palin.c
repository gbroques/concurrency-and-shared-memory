
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s id xx\n", argv[0]);
    return EXIT_FAILURE;
  }
  int id, index;
  id = atoi(argv[1]);
  index = atoi(argv[2]);

  printf("ID: %d Index: %d\n", id, index);

  return EXIT_SUCCESS;
}
