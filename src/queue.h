#include <stdbool.h>

#include "common.h"

typedef struct Node {
  Message *msg;
  struct Node *next;
} Node;

typedef struct Queue {
  Node *head;
  Node *tail;
  unsigned size;
  unsigned int maxSize;
} Queue;

Queue *queue_init(unsigned int maxSize);

void queue_destroy(Queue *queue);

Node *queue_initNode(Message *msg);

Node *queue_enqueue(Queue *q, Message *msg);

Message *queue_dequeue(Queue *q);

bool queue_isFull(Queue *q);

bool queue_isEmpty(Queue *q);
