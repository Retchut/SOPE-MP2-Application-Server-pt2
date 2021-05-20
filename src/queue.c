#include "./queue.h"
#include <stdio.h>
#include <stdlib.h>

Queue *initQueue(unsigned int maxSize) {
  Queue *q = malloc(sizeof(Queue));
  if (q == NULL) {
    perror("Failed creating queue");
    return NULL;
  }

  q->head = NULL;
  q->tail = NULL;
  q->size = 0;
  q->maxSize = maxSize;
}

Node *initNode(Message *msg) {
  Node *node = malloc(sizeof(Node));
  if (node == NULL) {
    perror("Failed creating node");
    return NULL;
  }

  node->msg = msg;
  node->next = NULL;
}

Node *enqueue(Queue *q, Message *msg) {
  // Needs mutex
  if (q->maxSize == q->size + 1) {
    fprintf(stderr, "Queue is full\n");
  }
  q->size++;

  // create a new node
  Node *node = NULL;
  if ((node = initNode(msg)) == NULL) {
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

Message* dequeue(Queue *q) {
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

  // Needs mutex
  q->size--;
  return result;
}

int isEmpty(Queue *q) { return (q->size == 0); }

