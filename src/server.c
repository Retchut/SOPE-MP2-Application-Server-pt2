// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include <errno.h>   // perror()
#include <fcntl.h>   // open()
#include <pthread.h>  // thread functions
#include <stdbool.h>  // bool
#include <stdio.h>
#include <stdlib.h>      // rand_r() atexit()
#include <string.h>      // snprintf() strcat()
#include <sys/inotify.h>  // inotify funcs
#include <sys/stat.h>    // open() mkfifo()
#include <sys/types.h>   // CLOCK_REALTIME mkfifo()
#include <time.h>        // clock functs
#include <unistd.h>      // usleep()

#include "./cmd_parser.h"
#include "./common.h"  // message
#include "./timer.h"

#define FIFONAME_LEN 1000


int pubFifoFD = -1;
bool serverOpen = true;

void cThreadFunc(void *taskId) {
  //enviar pedido a biblioteca e por resultado no buffer
  pthread_exit(0);
}

void closePubFifo(void) {
  /*if (close(pubFifoFD) == -1) {
    perror("Error closing public fifo");
  }*/
}

int main(int argc, char *const argv[]) {
  int nsecs = 0;
  int bufsz = 0;
  char *fifoname;
  if (cmdParser(argc, argv, &nsecs, &bufsz, &fifoname) != 0) {
    exit(EXIT_FAILURE);
  }

  printf("nsecs: %d, bufsz: %d, fifoname: %s\n", nsecs, bufsz, fifoname);

  /*// Setup detached threads
  pthread_attr_t detatched;

  if (pthread_attr_init(&detatched) != 0) {
    perror("Error in detached threads setup (init)");
    exit(EXIT_FAILURE);
  }
  if (pthread_attr_setdetachstate(&detatched, PTHREAD_CREATE_DETACHED) != 0) {
    perror("Error in detached threads setup (setdetachstate)");
    exit(EXIT_FAILURE);
  }

  pthread_t tid;

  // TaskIds
  int taskId = 0;

  // set start time in time.c
  setTimer(nsecs);

  
  //create  public fifo
  // Open public fifo
  while (getRemaining() > 0 && pubFifoFD == -1) {
    pubFifoFD = open(fifoname, O_WRONLY | O_NONBLOCK);
    if (pubFifoFD == -1) {
      if (errno != EACCES && errno != ENOENT && errno != ENXIO) {
        perror("Error opening public fifo");
        exit(EXIT_FAILURE);
      }
    }
  }

  atexit(&closePubFifo);

  while (getRemaining() > 0 && serverOpen) {  // Time remaining


    //dar enqueue das tarefas

    //cria se uma thread para cada tarefa na queue
    if (pthread_create(&tid, &detatched, (void *)(&cThreadFunc),
                       (void *)(&taskId)) != 0) {
      perror("Error creating threads");
      exit(EXIT_FAILURE);
    }

    //thread consumidora tira resultado do buffer e comunica com o client via fifo

  }

  // Destroy detached threads setup
  pthread_attr_destroy(&detatched);

  pthread_exit(0);*/
  exit(EXIT_SUCCESS);
}
