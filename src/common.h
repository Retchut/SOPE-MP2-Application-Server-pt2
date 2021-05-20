// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_ 1

#include <sys/types.h>

typedef struct Message {
  int rid;       // request id
  pid_t pid;     // process id
  pthread_t tid; // thread id
  int tskload;   // task load
  int tskres;    // task result
} Message;
#endif // SRC_COMMON_H_
