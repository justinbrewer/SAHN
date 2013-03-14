#include "queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct queue_node_t {
  void* element;
  struct queue_node_t* next;
};

struct queue_t {
  unsigned int len;

  struct queue_node_t* front;
  struct queue_node_t* back;
};

struct queue_node_t* queue__create_node(void* element){
  struct queue_node_t* node = (struct queue_node_t*)malloc(sizeof(struct queue_node_t));

  node->element = element;
  node->next = NULL;

  return node;
}

int queue__destroy_node(struct queue_node_t* node){
  if(node != NULL){
    queue__destroy_node(node->next);
    free(node);
  }
}

struct queue_t* queue_create(){
  struct queue_t* queue = (struct queue_t*)malloc(sizeof(struct queue_t));
  memset(queue,0,sizeof(struct queue_t));
  return queue;
}

int queue_destroy(struct queue_t* queue){
  queue__destroy_node(queue->front);
  free(queue);
}

unsigned int queue_push(struct queue_t* queue, void* element){
  struct queue_node_t* node = queue__create_node(element);

  if(queue->len == 0){
    queue->front = node;
    queue->back = node;
  } else {
    queue->back->next = node;
    queue->back = node;
  }

  queue->len++;

  return queue->len;
}

void* queue_pop(struct queue_t* queue){
  struct queue_node_t* node = queue->front;
  void* element = NULL;

  if(node != NULL){
    element = node->element;
    queue->front = node->next;
    free(node);
    queue->len--;
  }

  return element;
}

unsigned int queue_len(struct queue_t* queue){
  return queue->len;
}
