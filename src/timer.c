// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include "../src/timer.h"

#include <stdint.h>
#include <time.h>

long int start;
long int end;

long int getTime() { return time(NULL); }

void setTimer(int duration) {
  start = time(NULL);
  end = start + duration;
}

long int getElapsed() { return getTime() - start; }

long int getRemaining() { return end - getTime(); }

long int getServerRemaining() { return end + EXTRA_SECS - getTime() ; }
