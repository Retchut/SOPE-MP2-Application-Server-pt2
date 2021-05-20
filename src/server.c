// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include <errno.h>     // perror()
#include <fcntl.h>     // open()
#include <pthread.h>   // thread functions
#include <semaphore.h> // semaphore functs
#include <stdbool.h>   // bool
#include <stdio.h>
#include <stdlib.h>      // rand_r() atexit()
#include <string.h>      // snprintf() strcat()
#include <sys/inotify.h> // inotify funcs
#include <sys/stat.h>    // open() mkfifo()
#include <sys/types.h>   // CLOCK_REALTIME mkfifo()
#include <time.h>        // clock functs
#include <unistd.h>      // usleep()

#include "cmd_parser.h"
#include "common.h" // message
#include "lib.h"
#include "queue.h"
#include "timer.h"

#define FIFONAME_LEN 1000
#define PTHREAD_ATTEMPTS 3
#define EXTRA_SECS 2

static char *pubFifoName;
static int pubFifoFD = -1;
static int pThreadNr = 0;
static pthread_mutex_t pThreadNrMutex;
static Queue *queue = NULL;

void destroyPThreadNrMutex(void) {
  if (pthread_mutex_destroy(&pThreadNrMutex) != 0)
    perror("Error destroying pThreadNrMutex");
}

void destroyQueue(void) { queue_destroy(queue); }

void closePubFifo(void) {
  if (close(pubFifoFD) == -1) {
    perror("Error closing public fifo");
  }
}

void unlinkPubFifo(void) {
  if (unlink(pubFifoName) == -1) {
    perror("Error unlinking public fifo");
  }
}

void pThreadFunc(void *msg) {
  Message *rcvdMsg = (Message *)msg;

  if (getRemaining() > 0) {
    rcvdMsg->tskres = task(rcvdMsg->tskload);
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; TSKEX\n", getTime(), rcvdMsg->rid,
           rcvdMsg->tskload, getpid(), pthread_self(), rcvdMsg->tskres);
  }

  // Add task result to warehouse
  while(queue_enqueue(queue, rcvdMsg) == NULL){}

  // terminar thread produtora
  pthread_mutex_lock(&pThreadNrMutex);
  pThreadNr--;
  pthread_mutex_unlock(&pThreadNrMutex);

  pthread_exit(0);
}

void sender(Message *message) {
  int privFifoFD = -1;

  // Assemble fifoname
  char privFifoName[FIFONAME_LEN];
  snprintf(privFifoName, FIFONAME_LEN, "/tmp/%d.%ld", message->pid,
           message->tid);

  message->pid = getpid();
  message->tid = pthread_self();

  // Open private fifo
  privFifoFD = open(privFifoName, O_WRONLY | O_NONBLOCK);
  if (privFifoFD == -1) {
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message->rid,
           message->tskload, message->pid, message->tid, message->tskres);
    return;
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
    return;
  } else if (writeReady == 0) {
    fprintf(stderr, "TIMEOUT: Consumer Thread couldnt write to private fifo");
    if (close(privFifoFD) == -1) {
      perror("Error closing private fifo");
    }
    return;
  } else {
    if (write(privFifoFD, &(*message), sizeof(Message)) == -1) {
      printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message->rid,
             message->tskload, message->pid, message->tid, message->tskres);
      perror("Error writing to private fifo");
      if (close(privFifoFD) == -1) {
        perror("Error closing private fifo");
      }
    } else {
      if (message->tskres == -1) {
        printf("%ld ; %d ; %d ; %d ; %ld ; %d ; 2LATE\n", getTime(),
               message->rid, message->tskload, message->pid, message->tid,
               message->tskres);
      } else {
        printf("%ld ; %d ; %d ; %d ; %ld ; %d ; TSKDN\n", getTime(),
               message->rid, message->tskload, message->pid, message->tid,
               message->tskres);
      }
    }
  }
}

void cThreadFunc(void *arg) {
  Message *message = NULL;

  while(((getRemaining() + EXTRA_SECS) > 0 || pThreadNr > 0 || !(queue_isEmpty(queue)))) {
    message = queue_dequeue(queue);
    if(message != NULL){
      sender(message);
      free(message);
    }
    message = NULL;
  }

  pthread_exit(0);
}

int main(int argc, char *const argv[]) {
  int nsecs = 0;
  int bufsz = 0;
  if (cmdParser(argc, argv, &nsecs, &bufsz, &pubFifoName) != 0) {
    exit(EXIT_FAILURE);
  }
  
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

  if (atexit(&unlinkPubFifo) != 0) {
    fprintf(stderr, "Cannot register unlinkPubFifo to run at exit\n");
    exit(EXIT_FAILURE);
  }

  // Open fifo
  pubFifoFD = open(pubFifoName, O_RDONLY);
  if (pubFifoFD == -1) {
    perror("Error opening public fifo");
    exit(EXIT_FAILURE);
  }

  if (atexit(&closePubFifo) != 0) {
    fprintf(stderr, "Cannot register closePubFifo to run at exit\n");
    exit(EXIT_FAILURE);
  }

  // Queue
  if ((queue = queue_init(bufsz)) == NULL) {
    exit(EXIT_FAILURE);
  }

  if (atexit(&destroyQueue) != 0) {
    fprintf(stderr, "Cannot register destroyQueue to run at exit\n");
    exit(EXIT_FAILURE);
  }

  // Setup pThreadNrMutex
  pthread_mutex_init(&pThreadNrMutex, NULL);
  atexit(&destroyPThreadNrMutex);

  //tid for all threads
  pthread_t tid;

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

  // set start time in time.c
  setTimer(nsecs);
  // Consumer thread creation
  if (pthread_create(&tid, &detatched, (void *)(&cThreadFunc), NULL) != 0) {
    perror("Error creating consumer thread");
    pthread_attr_destroy(&detatched);
    exit(EXIT_FAILURE);
  }
  // Read From pubFifo Setup
  int dataReady = 0;
  Message *msg = NULL;

  while ((getRemaining() + EXTRA_SECS) > 0) { // Time remaining needs + 5 extra seconds to give out 2LATEs
    
    // Read From pubFifo Setup
    fd_set rfds;
    struct timeval timeout;
    FD_ZERO(&rfds);
    FD_SET(pubFifoFD, &rfds);
    timeout.tv_usec = 0;
    timeout.tv_sec = getRemaining();
    
    // Read from public fifo
    dataReady = select(pubFifoFD + 1, &rfds, NULL, NULL, &timeout);

    if (dataReady == -1) {
      perror("Error waiting for public FIFO");
      pthread_attr_destroy(&detatched);
      exit(EXIT_FAILURE); // also closes pub fifo
    } else if (dataReady == 0) {
      continue;
    } else {
      if ((msg = malloc(sizeof(Message))) == NULL) {
        perror("Error allocating memory to read message to");
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE); // also closes pub fifo and unlinks it
      }
      if (read(pubFifoFD, &(*msg), sizeof(Message)) == -1) {
        perror("Error reading from pub fifo");
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE); // also closes pub fifo and unlinks it
      }

      pthread_mutex_lock(&pThreadNrMutex);
      pThreadNr++;
      pthread_mutex_unlock(&pThreadNrMutex);
      for(int i = 1; i <= PTHREAD_ATTEMPTS; i++){
        if (pthread_create(&tid, &detatched, (void *)(&pThreadFunc),
                          (void *)(&msg)) != 0) {
          perror("Error creating producer thread");
        }
        else{
          break;
        }
        if(i == PTHREAD_ATTEMPTS){
          pthread_mutex_lock(&pThreadNrMutex);
          pThreadNr--;
          pthread_mutex_unlock(&pThreadNrMutex);
          free(msg);
        }
      }
      msg = NULL;
    }
  }

  // Destroy detached threads setup
  pthread_attr_destroy(&detatched);

  pthread_exit(0);
}
