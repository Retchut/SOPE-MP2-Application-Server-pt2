// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include <errno.h>     // perror()
#include <fcntl.h>     // open()
#include <pthread.h>   // thread functions
#include <semaphore.h>  // semaphore functs
#include <stdbool.h>   // bool
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
#include "./lib.h"
#include "./queue.h"
#include "./timer.h"

#define FIFONAME_LEN 1000
#define PTHREAD_ATTEMPTS 3

static char *pubFifoName;
static int pubFifoFD = -1;
static int pThreadNr = 0;
static pthread_mutex_t pThreadNrMutex;
static Queue *queue = NULL;
sem_t prodSem, conSem;

void destroyPThreadNrMutex(void) {
  if (pthread_mutex_destroy(&pThreadNrMutex) != 0)
    perror("Error destroying pThreadNrMutex");
}

void destroyQueue(void) { queue_destroy(queue); }

void destroySem(void) {
  sem_destroy(&conSem);
  sem_destroy(&prodSem);
}

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

void *pThreadFunc(void *msg) {
  Message rcvdMsg = *(Message *)msg;

  if (getRemaining() > 0) {
    rcvdMsg.tskres = task(rcvdMsg.tskload);
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; TSKEX\n", getTime(), rcvdMsg.rid,
           rcvdMsg.tskload, getpid(), pthread_self(), rcvdMsg.tskres);
  }

  struct timespec twait;
  twait.tv_nsec = 0;
  twait.tv_sec = EXTRA_SECS;
  sem_timedwait(&prodSem, &twait);
  // Add task result to warehouse
  if (queue_enqueue(queue, &rcvdMsg) == NULL) {
    printf("Failed enqueuing\n");
  }
  sem_post(&conSem);  // signal the consumer thread it can pop from the queue

  // terminar thread produtora
  pthread_mutex_lock(&pThreadNrMutex);
  pThreadNr--;
  pthread_mutex_unlock(&pThreadNrMutex);

  pthread_exit(0);
}

void sender(Message *message) {
  int privFifoFD;

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
  fd_set wfds;
  struct timeval timeout;
  FD_ZERO(&wfds);
  FD_SET(privFifoFD, &wfds);

  timeout.tv_sec = EXTRA_SECS;
  timeout.tv_usec = 0;

  writeReady = select(privFifoFD + 1, NULL, &wfds, NULL, &timeout);

  if (writeReady == -1) {
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message->rid,
           message->tskload, message->pid, message->tid, message->tskres);
    perror("Error waiting for private FIFO");
    if (close(privFifoFD) == -1) {
      perror("Error closing private fifo");
    }
    return;
  } else if (writeReady == 0) {
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message->rid,
           message->tskload, message->pid, message->tid, message->tskres);
    fprintf(stderr, "TIMEOUT: Consumer Thread couldnt write to private fifo");
    if (close(privFifoFD) == -1) {
      perror("Error closing private fifo");
    }
    return;
  } else {
    if (write(privFifoFD, message, sizeof(Message)) < sizeof(Message)) {
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

  if (close(privFifoFD) == -1) {
    perror("Error closing private fifo");
  }
}

void *cThreadFunc() {
  bool nThread;
  pthread_mutex_lock(&pThreadNrMutex);
  nThread = pThreadNr > 0;
  pthread_mutex_unlock(&pThreadNrMutex);

  while (getServerRemaining() + 3 * EXTRA_SECS > 0
    || nThread || !queue_isEmpty(queue)) {
    Message message;
    struct timespec twait;
    twait.tv_nsec = 0;
    twait.tv_sec = time(NULL) + EXTRA_SECS;

    if (sem_timedwait(&conSem, &twait) == -1) {
      if (errno == ETIMEDOUT) {
        continue;
      }
    }
    message = queue_dequeue(queue);
    sem_post(&prodSem);  // allow productor thread to push to the queue

    sender(&message);
  }

  pthread_exit(0);
}

int main(int argc, char *const argv[]) {
  setbuf(stderr, NULL);
  setbuf(stdout, NULL);

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
  pubFifoFD = open(pubFifoName, O_RDONLY | O_NONBLOCK);
  if (pubFifoFD == -1) {
    perror("Error opening public fifo");
    exit(EXIT_FAILURE);
  }

  if (atexit(&closePubFifo) != 0) {
    fprintf(stderr, "Cannot register closePubFifo to run at exit\n");
    exit(EXIT_FAILURE);
  }

  // setup prodSem
  sem_init(&prodSem, 0, bufsz);
  sem_init(&conSem, 0, 0);

  if (atexit(&destroySem) != 0) {
    fprintf(stderr, "Cannot register Semdestroy to run at exit\n");
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

  if (atexit(&destroyPThreadNrMutex) != 0) {
    fprintf(stderr, "Cannot register pthreadMutexNr to run at exit\n");
    exit(EXIT_FAILURE);
  }

  // tid for all threads
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
  if (pthread_create(&tid, &detatched, cThreadFunc, NULL) != 0) {
    perror("Error creating consumer thread");
    pthread_attr_destroy(&detatched);
    exit(EXIT_FAILURE);
  }
  // Read From pubFifo Setup
  int dataReady = 0;

  while (getServerRemaining() > 0 || pThreadNr > 0 ||
         !queue_isEmpty(queue)) {  // Time remaining needs
    // Read From pubFifo Setup
    fd_set rfds;
    struct timeval timeout;
    FD_ZERO(&rfds);
    FD_SET(pubFifoFD, &rfds);
    timeout.tv_usec = 100;
    if (getServerRemaining() > 0)
      timeout.tv_sec = getServerRemaining();

    // Read from public fifo if
    dataReady = select(pubFifoFD + 1, &rfds, NULL, NULL, &timeout);

    if (dataReady == -1) {
      perror("Error waiting for public FIFO");
      pthread_attr_destroy(&detatched);
      exit(EXIT_FAILURE);  // also closes pub fifo
    } else if (dataReady == 0) {
      break;
    } else {
      Message msg;
      int r = 0;
      if ((r = read(pubFifoFD, &msg, sizeof(Message))) < sizeof(Message)) {
        if (r == -1) {
          perror("Error reading from pub fifo");
          pthread_attr_destroy(&detatched);
          pthread_exit(0);
        }
        break;
      }

      printf("%ld ; %d ; %d ; %d ; %ld ; %d ; RECVD\n", getTime(), msg.rid,
             msg.tskload, getpid(), pthread_self(), msg.tskres);

      pthread_mutex_lock(&pThreadNrMutex);
      pThreadNr++;
      pthread_mutex_unlock(&pThreadNrMutex);
      for (int i = 1; i <= PTHREAD_ATTEMPTS; i++) {
        if (pthread_create(&tid, &detatched, pThreadFunc, (void *)(&msg)) ==
            0) {
          break;  // if creating a thread succeeds
        }
        if (i == PTHREAD_ATTEMPTS) {
          pthread_mutex_lock(&pThreadNrMutex);
          pThreadNr--;
          pthread_mutex_unlock(&pThreadNrMutex);
        }
      }
    }
  }

  // Destroy detached threads setup
  pthread_attr_destroy(&detatched);

  pthread_exit(0);
}
