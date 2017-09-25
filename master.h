#ifndef MASTER_H_
#define MASTER_H_

typedef enum process_state {
  idle,
  want_in,
  in_cs
} process_state;

static const int MEMORY_OFFSET = 256;
static const int MAX_WRITES = 5; // Max number of writes per child
static const int MAX_PROCESSES = 19; // Max number of child processes

#endif
