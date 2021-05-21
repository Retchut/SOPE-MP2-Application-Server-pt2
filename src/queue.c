#include "./queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t queueMutex;

Queue *queue_init(unsigned int maxSize) {
  pthread_mutex_init(&queueMutex, NULL);

  Queue *q = malloc(sizeof(Queue));

  if (q != NULL) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->maxSize = maxSize;
    return q;
  }

  perror("Failed creating queue");
  return q;
}

void queue_destroy(Queue *queue) {
  Message *msg = NULL;
  while (!queue_isEmpty(queue)) {
    printf("looping destroy queue");
    msg = queue_dequeue(queue);
    free(msg);
  }

  free(queue);
  queue = NULL;
  pthread_mutex_destroy(&queueMutex);
}

Node *queue_initNode(Message *msg) {
  Node *node = malloc(sizeof(Node));

  if (node != NULL) {
    node->message = msg;
    node->next = NULL;
    return node;
  }

  perror("Failed creating node");
  return node;
}

Node *queue_enqueue(Queue *queue, Message *msg) {
  // if (queue->size >= queue->maxSize) {
  //   fprintf(stderr, "Queue is full\n");
  //   return NULL;
  // }
  pthread_mutex_lock(&queueMutex);
  queue->size++;
  pthread_mutex_unlock(&queueMutex);

  // create a new node
  Node *node = NULL;
  if ((node = queue_initNode(msg)) == NULL) {
    return NULL;
  }

  // if there is a tail, connect to new node
  if (queue->tail != NULL) {
    queue->tail->next = node;
  }

  queue->tail = node;

  // if q is empty newnode becomes the head
  if (queue->head == NULL) {
    queue->head = node;
  }

  return node;
}

Message *queue_dequeue(Queue *queue) {
  // check if q is empty
  if (queue->head == NULL) {
    return NULL;
  }

  // save the head
  Node *tmp = queue->head;

  // save the result to return
  Message *result = malloc(sizeof(Message));
  (*result) = (*(tmp->message));
  // Message *result = tmp->message;

  // removing from list and updating values
  queue->head = queue->head->next;

  if (queue->head == NULL) {
    queue->tail = NULL;
  }

  free(tmp);

  pthread_mutex_lock(&queueMutex);
  queue->size--;
  pthread_mutex_unlock(&queueMutex);

  return result;
}

bool queue_isFull(Queue *queue) {
  bool ret = true;
  pthread_mutex_lock(&queueMutex);
  ret = queue->size >= queue->maxSize;
  pthread_mutex_unlock(&queueMutex);
  return ret;
}

bool queue_isEmpty(Queue *queue) {
  bool ret = false;
  pthread_mutex_lock(&queueMutex);
  ret = queue->size == 0;
  pthread_mutex_unlock(&queueMutex);
  return ret;
}
