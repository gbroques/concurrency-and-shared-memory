#ifndef MASTER_H_
#define MASTER_H_

#include <stdio.h>

typedef enum process_state {
  idle,
  want_in,
  in_cs
} process_state;

// extern int turn;
// extern state flag[n]; /*Flag corresponding to each process in shared memory */

static const int MEMORY_OFFSET = 256;
static const int MAX_WRITES = 5;
static const int MAX_PROCESSES = 20;

#endif
