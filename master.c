
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

/* ARGSUSED */
static void myhandler(int s) {
  // TODO: Free memory
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
    return 1;
  }

  if (setupitimer() == -1) {
    perror("Faled to set up the ITIMER_PROF interval timer");
    return 1;
  }

  signal(SIGINT, myhandler);
  for ( ; ; ); /* execute rest of main program here */
}
