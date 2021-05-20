#include "./queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t queueMutex;

Queue *queue_init(unsigned int maxSize) {
  pthread_mutex_init(&queueMutex, NULL);

  Queue *q = malloc(sizeof(Queue));
  if (q == NULL) {
    perror("Failed creating queue");
    return NULL;
  }

  q->head = NULL;
  q->tail = NULL;
  q->size = 0;
  q->maxSize = maxSize;

  return q;
}

void queue_destroy(Queue *queue) {
  Message *msg = NULL;
  while (queue->head != NULL) {
    msg = queue_dequeue(queue);
    free(msg);
  }

  free(queue);
  pthread_mutex_destroy(&queueMutex);
}

Node *queue_initNode(Message *msg) {
  Node *node = malloc(sizeof(Node));
  if (node == NULL) {
    perror("Failed creating node");
    return NULL;
  }

  node->msg = msg;
  node->next = NULL;

  return node;
}

Node *queue_enqueue(Queue *q, Message *msg) {
  unsigned tmpsz = q->size;
  pthread_mutex_lock(&queueMutex);
  if (q->size + 1 <= q->maxSize) {
    q->size++;
  }
  pthread_mutex_unlock(&queueMutex);
  if (q->size == tmpsz) {
    fprintf(stderr, "Queue is full\n");
    return NULL;
  }

  // create a new node
  Node *node = NULL;
  if ((node = queue_initNode(msg)) == NULL) {
    return NULL;
  }

  // if there is a tail, connect to new node
  if (q->tail != NULL) {
    q->tail->next = node;
  }

  q->tail = node;

  // if q is empty newnode becomes the head
  if (q->head == NULL) {
    q->head = node;
  }

  return node;
}

Message *queue_dequeue(Queue *q) {
  // check if q is empty
  if (q->head == NULL) {
    return NULL;
  }

  // save the head
  Node *tmp = q->head;

  // save the result to return
  Message *result = tmp->msg;

  // removing from list and updating values
  q->head = q->head->next;

  if (q->head == NULL) {
    q->tail = NULL;
  }

  free(tmp);

  pthread_mutex_lock(&queueMutex);
  q->size--;
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
