#include "./queue.h"
#include <stdio.h>
#include <stdlib.h>
#include "./common.h"

void init_queue(queue *q){
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

int isEmpty(queue* q)
{
    return (q->size == 0);
}
 

int enqueue(queue* q, Message* item)
{
    //create a new node
    node* newnode = malloc(sizeof(node));
    if (newnode == NULL) {return 1;}
    newnode->value = item;
    newnode->next = NULL;
    //if there is a tail, connect to new node
    if(q->tail != NULL){
        q->tail->next = newnode;
    }
    q->tail = newnode;
    //if q is empty newnode becomes the head
    if(q->head == NULL){
        q->head = newnode;
    }
    q->size++;
    return 0;
}
 

Message* dequeue(queue* q)
{
    //check if q is empty
    if(q->head == NULL ){return NULL;}
    //save the head    
    node * tmp = q->head;
    //save the result to return
    Message* result = tmp->value;
    //removing from list and updating values
    q->head = q->head->next;
    if(q->head == NULL) {
        q->tail = NULL;
    }
    free(tmp);
    q->size--;
    return result;
}