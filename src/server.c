// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include "cmd_parser.h"
#include "common.h" // message
#include "queue.h"
#include "timer.h"
#include <errno.h>   // perror()
#include <fcntl.h>   // open()
#include <pthread.h> // thread functions
#include <semaphore.h>
#include <stdbool.h> // bool
#include <stdio.h>
#include <stdlib.h>      // rand_r() atexit()
#include <string.h>      // snprintf() strcat()
#include <sys/inotify.h> // inotify funcs
#include <sys/stat.h>    // open() mkfifo()
#include <sys/types.h>   // CLOCK_REALTIME mkfifo()
#include <time.h>        // clock functs
#include <unistd.h>      // usleep()

#define FIFONAME_LEN 1000

int pubFifoFD = -1;
// Queue storehouse;

int sender(Message *message) {
  int privFifoFD = -1;

  // Assemble fifoname
  char privFifoName[FIFONAME_LEN];
  snprintf(privFifoName, FIFONAME_LEN, "/tmp/%d.%ld", message->pid,
           message->tid);

  // Open private fifo
  privFifoFD = open(privFifoName, O_WRONLY | O_NONBLOCK);
  if (privFifoFD == -1) {
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message->rid,
           message->tskload, message->pid, message->tid, message->tskres);
    return 1;
  }

  // Set select
  int writeReady = 0;
  fd_set rfds;
  struct timeval timeout;
  FD_ZERO(&rfds);
  FD_SET(privFifoFD, &rfds);
  timeout.tv_sec = getRemaining() + 2;
  timeout.tv_usec = 0;

  writeReady = select(privFifoFD + 1, &rfds, NULL, NULL, &timeout);

  if (writeReady == -1) {
    perror("Error waiting for private FIFO");
    if (close(privFifoFD) == -1) {
      perror("Error closing private fifo");
    }
    return 1;
  } else if (writeReady == 0) {
    fprintf(stderr, "TIMEOUT: Consumer Thread couldnt write to private fifo");
    if (close(privFifoFD) == -1) {
      perror("Error closing private fifo");
    }
    return 1;
  } else {
    if (write(privFifoFD, &message, sizeof(Message)) == -1) {
      perror("Error writing to private fifo");
      if (close(privFifoFD) == -1) {
        perror("Error closing private fifo");
      }
      return 1;
    } else {
      if (message.tskres == -1) {
        printf("%ld ; %d ; %d ; %d ; %ld ; %d ; 2LATE\n", getTime(),
               message->rid, message->tskload, message->pid, message->tid,
               message->tskres);
      } else {
        printf("%ld ; %d ; %d ; %d ; %ld ; %d ; TSKEX\n", getTime(),
               message->rid, message->tskload, message->pid, message->tid,
               message->tskres);
      }
    }
  }
  return 0;
}

void cThreadFunc(void *arg) {
  // queue buffer
  bool gotMessage = false;
  int senderRet = 0;
  Message* message = NULL;

  while (getRemaining > 0 && senderRet == 0) {
    // mutex/semaforo

    
    senderRet = sender(message);
    free(message);
    message = NULL;
  }

  // Destroy container

  pthread_exit(0);
}

void pThreadFunc(void *msg) {
  Message *rcvdMsg = (Message *)msg;

  int result = task(rcvdMsg->t);

  // semaphore
  //         cliente fica bloqueado quando o armazem está vazio
  //         produtor fica bloqueado quando o armazem está cheio
  // mutex

  // meter result no buffer armazém

  // end mutex
  // end semaphore

  // notificar thread consumidora para que esta retire
  //  valores do armazém e enviar ao cliente

  // terminar thread produtora

  pthread_exit(0);

  /*
  // // Set message struct

  // struct argsThPrd *auxTaskArgs = (struct argsThPrd *)taskArgs; // var
  auxiliar

  // Message rep_message;
  // memset(&rep_message, 0, sizeof(rep_message));

  // rep_message.rid = auxTaskArgs->infoArgsThProSave.rid;
  // rep_message.tskload = auxTaskArgs->infoArgsThProSave.tskload;
  // rep_message.pid = getpid(); // ver melhor isto
  // rep_message.tid = pthread_self();
  // rep_message.tskres = task(auxTaskArgs->infoArgsThProSave.tskload); // ver
  melhor isto, biblioteca, certo?

  sem_t sem;
  sem_init(&sem, 0, 1);
  sem_wait(&sem);
  while (auxTaskArgs->q.isEmpty() || auxTaskArgs->q->size ==
  auxTaskArgs->maxBfSize)
  {

  }

  sem_post(&sem);



  enqueue(auxTaskArgs->q, rep_message);
  */
}

void closePubFifo(void) {
  if (close(pubFifoFD) == -1) {
    perror("Error closing public fifo");
  }
}

int main(int argc, char *const argv[]) {
  int nsecs = 0;
  int bufsz = 0;
  char *pubFifoName;
  if (cmdParser(argc, argv, &nsecs, &bufsz, &pubFifoName) != 0) {
    exit(EXIT_FAILURE);
  }

  // printf("nsecs: %d, bufsz: %d, fifoname: %s\n", nsecs, bufsz, fifoname);

  // Setup detached threads
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

  // set start time in time.c
  setTimer(nsecs);

  // Create fifo
  while (true) {
    if (mkfifo(pubFifoName, 0777) == -1) {
      if (errno != EEXIST) {
        perror("Error creating public Fifo");
        exit(EXIT_FAILURE);
      } else {
        if (unlink(pubFifoName) == -1) {
          perror("Error removing FIFO with same name");
          exit(EXIT_FAILURE);
        }
        continue;
      }
    }
    break;
  }

  // Open fifo
  pubFifoFD = open(pubFifoName, O_RDONLY);
  if (pubFifoFD == -1) {
    perror("Error opening public fifo");
    exit(EXIT_FAILURE);
  }
  atexit(&closePubFifo);

  // Queue
  // initQueue(&storehouse);

  // Consumer thread creation
  if (pthread_create(&tid, &detatched, (void *)(&cThreadFunc), NULL) != 0) {
    perror("Error creating consumer thread");
    pthread_attr_destroy(&detatched);
    exit(EXIT_FAILURE);
  }

  // Select Setup
  int dataReady = 0;
  fd_set rfds;
  struct timeval timeout;
  FD_ZERO(&rfds);
  FD_SET(pubFifoFD, &rfds);
  timeout.tv_usec = 0;

  // Message pointer to read to
  Message *msg = NULL;

  while (getRemaining() > 0) { // Time remaining
    // Read from public fifo
    timeout.tv_sec = getRemaining();
    dataReady = select(pubFifoFD + 1, &rfds, NULL, NULL, &timeout);

    if (dataReady == -1) {
      perror("Error waiting for public FIFO");
      pthread_attr_destroy(&detatched);
      exit(EXIT_FAILURE); // also closes pub fifo
    } else if (dataReady == 0) {
      break;
    } else {
      if ((msg = malloc(sizeof(Message))) == NULL) {
        perror("Error allocating memory to read message to");
        // Should tell other pthreads/cthread to free their messages ->
        // Try later to make static alloc and copy to a variable
        // inside pthread/cthread (Mem leak when program fails)
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE); // also closes pub fifo
      }
      if (read(pubFifoFD, &msg, sizeof(Message)) == -1) {
        perror("Error reading from pub fifo");
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE); // also closes pub fifo
      }

      if (pthread_create(&tid, &detatched, (void *)(&pThreadFunc),
                         (void *)(&msg)) != 0) {
        perror("Error creating producer thread");
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE);
      }
      msg = NULL;
    }
  }

  // Destroy detached threads setup
  pthread_attr_destroy(&detatched);

  pthread_exit(0);
}
