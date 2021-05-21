#include "../src/queue.h"

#include <stdio.h>
#include <stdlib.h>


Queue *queue_init(unsigned int maxSize) {
  Queue *q = malloc(sizeof(Queue));

  if (q != NULL) {
    q->head = NULL;
    q->tail = NULL;
    return q;
  }

  perror("Failed creating queue");
  return q;
}

void queue_destroy(Queue *queue) {
  while (!queue_isEmpty(queue)) {
    printf("looping destroy queue");
    queue_dequeue(queue);
  }

  free(queue);
  queue = NULL;
}

Node *queue_initNode(Message *msg) {
  Node *node = (Node *) malloc(sizeof(Node));

  if (node != NULL) {
    node->message = *msg;
    node->next = NULL;
    return node;
  }

  perror("Failed creating node");
  return node;
}

Node *queue_enqueue(Queue *queue, Message *msg) {
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

Message queue_dequeue(Queue *queue) {
  Message result;

  if (queue->head == NULL) {
    result.tskres = -2;
    return result;
  }
  // save the head
  Node *tmp = queue->head;

  // save the result to return
  result = tmp->message;
  // Message *result = tmp->message;

  // removing from list and updating values
  queue->head = queue->head->next;

  if (queue->head == NULL) {
    queue->tail = NULL;
  }

  free(tmp);

  return result;
}


bool queue_isEmpty(Queue *queue) {
  return queue->head == NULL;
}
