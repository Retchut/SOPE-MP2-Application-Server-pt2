// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include <errno.h>   // perror()
#include <fcntl.h>   // open()
#include <pthread.h> // thread functions
#include <stdbool.h> // bool
#include <stdio.h>
#include <stdlib.h>      // rand_r() atexit()
#include <string.h>      // snprintf() strcat()
#include <sys/inotify.h> // inotify funcs
#include <sys/stat.h>    // open() mkfifo()
#include <sys/types.h>   // CLOCK_REALTIME mkfifo()
#include <time.h>        // clock functs
#include <unistd.h>      // usleep()
#include <semaphore.h>
#include "cmd_parser.h"
#include "common.h" // message
#include "timer.h"
#include "queue.h"


#define FIFONAME_LEN 1000

//TODO: remove this probably
struct argsThPrd
{
  struct Message *infoArgsThProSave;

  int maxBfSize;

  struct queue q;
};

int pubFifoFD = -1;
bool serverOpen = true;
Queue storehouse;

int consumer(Message message){
  bool openfifo=false;
  int privFifoFD;

  // Assemble fifoname
  char privFifoName[FIFONAME_LEN];
  snprintf(privFifoName, FIFONAME_LEN, "/tmp/%d.%ld", message.pid, message.tid);

  // Open private fifo
  while (getRemaining() > 0 && openfifo == false) {
    privFifoFD = open(privFifoName, O_WRONLY | O_NONBLOCK);
    if (privFifoFD  == -1) {
      if (errno != EACCES && errno != ENOENT && errno != ENXIO) {
        printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message.rid,
          message.tskload, message.pid, message.tid, message.tskres);
        return 1;
      }
    }
    else{
      openfifo = true;
    }
  }

  if (getRemaining() == 0){
      printf("%ld ; %d ; %d ; %d ; %ld ; %d ; 2LATE\n", getTime(), message.rid,
         message.tskload, message.pid, message.tid, message.tskres);
      if (unlink(privFifoName) == -1) {
        perror("Error unlinking private fifo");
      }
      return 2;
  }

  // Set select
  int writeReady = 0;
  fd_set rfds;
  struct timeval timeout;
  FD_ZERO(&rfds);
  FD_SET(privFifoFD, &rfds);
  timeout.tv_sec = getRemaining();
  timeout.tv_usec = 0;

  writeReady = select(privFifoFD + 1, &rfds, NULL, NULL, &timeout);

  if (writeReady == -1) {
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; FAILD\n", getTime(), message.rid,
      message.tskload, message.pid, message.tid, message.tskres);
    if (close(privFifoFD) == -1) {
      perror("Error closing private fifo");
    }
    if (unlink(privFifoName) == -1) {
      perror("Error unlinking private fifo");
    }
    return 2;
  } else if (writeReady == 0) {
    printf("%ld ; %d ; %d ; %d ; %ld ; %d ; 2LATE\n", getTime(), message.rid,
           message.tskload, message.pid, message.tid, message.tskres);
    return 2;
  } else {
    if (write(privFifoFD, &message, sizeof(Message)) == -1) {
      perror("Error writing priv fifo");
      if (close(privFifoFD) == -1) {
        perror("Error closing private fifo");
      }
      if (unlink(privFifoName) == -1) {
        perror("Error unlinking private fifo");
      }
      return 1;
    }
    else{
          printf("%ld ; %d ; %d ; %d ; %ld ; %d ; TSKEX\n", getTime(), message.rid,
        message.tskload, message.pid, message.tid, message.tskres);
    }
  }
  return 0;
}

void cThreadFunc(void *arg){
  //queue buffer;
  int res;
  bool gotMessage = false;
  bool keepgoing = true;

  while(getRemaining>0 && keepgoing){
    //mutex/semaforo
    if(!buffer.isEmpty()){
      Message *m = buffer.dequeue(&buffer);
      gotMessage = true;
    }
    else{
      gotMessage = false;
    }
    //mutex/semaforo
    if(gotMessage){
      if(consumer(*m) != 0){
        keepgoing = false;
      }
    }
  }
  pthread_exit(0);
}


void pThreadFunc(void *msg)
{
  Message *rcvdMsg = (Message *) msg;

  int result = task(rcvdMsg->t);

  //semaphore
  //         cliente fica bloqueado quando o armazem está vazio 
  //         produtor fica bloqueado quando o armazem está cheio
  //mutex

  //meter result no buffer armazém

  //end mutex
  //end semaphore
  

  //notificar thread consumidora para que esta retire
  //  valores do armazém e enviar ao cliente

  //terminar thread produtora


  pthread_exit(0);




  /*
  // // Set message struct

  // struct argsThPrd *auxTaskArgs = (struct argsThPrd *)taskArgs; // var auxiliar

  // Message rep_message;
  // memset(&rep_message, 0, sizeof(rep_message));

  // rep_message.rid = auxTaskArgs->infoArgsThProSave.rid;
  // rep_message.tskload = auxTaskArgs->infoArgsThProSave.tskload;
  // rep_message.pid = getpid(); // ver melhor isto
  // rep_message.tid = pthread_self();
  // rep_message.tskres = task(auxTaskArgs->infoArgsThProSave.tskload); // ver melhor isto, biblioteca, certo?

  sem_t sem;
  sem_init(&sem, 0, 1);
  sem_wait(&sem);
  while (auxTaskArgs->q.isEmpty() || auxTaskArgs->q->size == auxTaskArgs->maxBfSize)
  {
    
  }

  sem_post(&sem);


  
  enqueue(auxTaskArgs->q, rep_message);
  */
}

void closePubFifo(void)
{
  if (close(pubFifoFD) == -1) {
    perror("Error closing public fifo");
  }
}

int main(int argc, char *const argv[]) {
  int nsecs = 0;
  int bufsz = 0;
  char *fifoname;
  if (cmdParser(argc, argv, &nsecs, &bufsz, &fifoname) != 0)
  {
    exit(EXIT_FAILURE);
  }

  //printf("nsecs: %d, bufsz: %d, fifoname: %s\n", nsecs, bufsz, fifoname);

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
  
  //create public fifo
  pubFifoFD = open(fifoname, O_RDONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
  if (pubFifoFD == -1) {
    perror("Error opening public fifo");
    exit(EXIT_FAILURE);
  }
  atexit(&closePubFifo);

  // Queue 
  initQueue(&storehouse);

  // Consumer thread creation
  if (pthread_create(&tid, &detatched, (void *)(&cThreadFunc), NULL) != 0) {
      perror("Error creating consumer thread");
      pthread_attr_destroy(&detatched);
      exit(EXIT_FAILURE);
  }


  int dataReady = 0;
  Message msg;
  memset(&msg, 0, sizeof(msg));
  while (getRemaining() > 0) {  // Time remaining
    // Read from public fifo
    fd_set rfds;
    struct timeval timeout;
    FD_ZERO(&rfds);
    FD_SET(pubFifoFD, &rfds);
    timeout.tv_sec = getRemaining();
    timeout.tv_usec = 0;
    memset(&msg, 0, sizeof(msg));
    dataReady = select(pubFifoFD + 1, &rfds, NULL, NULL, &timeout);

    if(dataReady == -1){
      perror("Error waiting for public FIFO");
      pthread_attr_destroy(&detatched);
      exit(EXIT_FAILURE); //also closes pub fifo
    }
    else if(dataReady == 0){
      break;
    }
    else{
      if(read(pubFifoFD, &msg, sizeof(msg)) == -1){
        perror("Error reading from pub fifo");
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE); //also closes pub fifo
      }

      if(pthread_create(&tid, &detatched, (void *)(&pThreadFunc), (void *) (&msg) != 0)){
        perror("Error creating producer thread");
        pthread_attr_destroy(&detatched);
        exit(EXIT_FAILURE);
      }
    }
  }

  // Destroy detached threads setup
  pthread_attr_destroy(&detatched);

  pthread_exit(0);
}
