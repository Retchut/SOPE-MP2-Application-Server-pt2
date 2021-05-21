// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include "./timer.h"

#include <stdint.h>
#include <time.h>

int64_t start;
int64_t end;

int64_t getTime() { return time(NULL); }

void setTimer(int duration) {
  start = time(NULL);
  end = start + duration;
}

int64_t getElapsed() { return getTime() - start; }

int64_t getRemaining() { return end - getTime(); }

int64_t getServerRemaining() { return end - getTime() + EXTRA_SECS; }
