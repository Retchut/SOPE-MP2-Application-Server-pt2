#include "common.h"

typedef struct Node {
    Message* msg;
    Node* next;
} Node;

typedef struct Queue{
    Node *head;
    Node *tail;
    unsigned size;
    unsigned int maxSize;
} Queue;


Queue* initQueue(unsigned int maxSize);

Node* initNode(Message *msg);

Node *enqueue(Queue *q, Message *msg);
 
Message* dequeue(Queue* q);

bool isEmpty(Queue* q);
