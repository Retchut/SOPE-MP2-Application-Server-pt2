#ifndef SRC_QUEUE_H_
#define SRC_QUEUE_H_ 1

#include <stdbool.h>

#include "./common.h"

typedef struct Node {
  Message message;
  struct Node *next;
} Node;

typedef struct Queue {
  Node *head;
  Node *tail;
} Queue;

Queue *queue_init(unsigned int maxSize);

void queue_destroy(Queue *queue);

Node *queue_enqueue(Queue *queue, Message *message);

Message queue_dequeue(Queue *queue);

bool queue_isEmpty(Queue *queue);
#endif  // SRC_QUEUE_H_
