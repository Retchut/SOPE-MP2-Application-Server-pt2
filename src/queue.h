 typedef struct Node {
    Message* value;
    struct Node *next;
} Node;

typedef struct Queue{
    Node *head;
    Node *tail;
    unsigned size;
} Queue;


void initQueue(Queue *q);

void initNode(Node* newNode, Message *item);

int isEmpty(Queue* q);
 
int enqueue(Queue* q, Message* item);
 
Message* dequeue(Queue* q);
