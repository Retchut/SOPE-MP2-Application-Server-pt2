 typedef struct node {
    Message* value;
    struct node *next;
} node;

typedef struct {
    node *head;
    node *tail;
    unsigned size;
} queue;


void init_queue(queue *q);

int isEmpty(queue* q);
 
int enqueue(queue* q, Message* item);
 
Message* dequeue(queue* q);
