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
#include "./cmd_parser.h"
#include "./common.h" // message
#include "./timer.h"

#define FIFONAME_LEN 1000

struct argsThPrd
{
  struct Message *infoArgsThProSave;

  int maxBfSize;

  struct queue q;
};

int pubFifoFD = -1;
bool serverOpen = true;

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

void consumerThreadFunc(){
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


void pThreadFunc(void *taskArgs)
{
  
  // Set message struct

  struct argsThPrd *auxTaskArgs = (struct argsThPrd *)taskArgs; // var auxiliar

  Message rep_message;
  memset(&rep_message, 0, sizeof(rep_message));

  rep_message.rid = auxTaskArgs->infoArgsThProSave.rid;
  rep_message.tskload = auxTaskArgs->infoArgsThProSave.tskload;
  rep_message.pid = getpid(); // ver melhor isto
  rep_message.tid = pthread_self();
  rep_message.tskres = task(auxTaskArgs->infoArgsThProSave.tskload); // ver melhor isto

  sem_t sem;
  sem_init(&sem, 0, 1);
  while (auxTaskArgs->q.isEmpty() || auxTaskArgs->q->size == auxTaskArgs->maxBfSize)
  {
    sem_wait(&sem);
  }

  sem_post(&sem);

  
  enqueue(auxTaskArgs->q, rep_message);
  
  pthread_exit(0);
}

void closePubFifo(void)
{
  /*if (close(pubFifoFD) == -1) {
    perror("Error closing public fifo");
  }*/
}

int main(int argc, char *const argv[])
{
  int nsecs = 0;
  int bufsz = 0;
  char *fifoname;
  if (cmdParser(argc, argv, &nsecs, &bufsz, &fifoname) != 0)
  {
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


    os pedido do Cliente são recolhidos pelo thread principal s0 que cria threads
Produtores s1, …, sn e lhes passa os pedidos; cada Produtor invoca a
correspondente tarefa em B e coloca o resultado obtido no armazém; depois,
termina;

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
