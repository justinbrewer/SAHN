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

  pthread_mutex_t lock;
  pthread_cond_t avail;
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
  return 0;
}

struct queue_t* queue_create(){
  struct queue_t* queue = (struct queue_t*)malloc(sizeof(struct queue_t));
  memset(queue,0,sizeof(struct queue_t));

  pthread_mutex_init(&queue->lock,NULL);
  pthread_cond_init(&queue->avail,NULL);

  return queue;
}

int queue_destroy(struct queue_t* queue){
  queue__destroy_node(queue->front);

  pthread_mutex_destroy(&queue->lock);
  pthread_cond_destroy(&queue->avail);

  free(queue);
  return 0;
}

unsigned int queue_push(struct queue_t* queue, void* element){
  unsigned int len;
  struct queue_node_t* node = queue__create_node(element);

  pthread_mutex_lock(&queue->lock);

  if(queue->len == 0){
    queue->front = node;
    queue->back = node;
  } else {
    queue->back->next = node;
    queue->back = node;
  }

  len = ++queue->len;

  pthread_cond_signal(&queue->avail);
  pthread_mutex_unlock(&queue->lock);

  return len;
}

void* queue_pop(struct queue_t* queue){
  struct queue_node_t* node;
  void* element = NULL;

  pthread_mutex_lock(&queue->lock);

  while((node = queue->front) == NULL){
    pthread_cond_wait(&queue->avail,&queue->lock);
  }

  element = node->element;
  queue->front = node->next;
  queue->len--;

  pthread_mutex_unlock(&queue->lock);

  free(node);
  return element;
}

unsigned int queue_len(struct queue_t* queue){
  unsigned int len;

  pthread_mutex_lock(&queue->lock);
  len = queue->len;
  pthread_mutex_unlock(&queue->lock);

  return len;
}
