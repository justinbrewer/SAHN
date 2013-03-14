#pragma once

#include <stdlib.h>

struct queue_t;

struct queue_t* queue_create();
int queue_destroy(struct queue_t* queue);

unsigned int queue_push(struct queue_t* queue, void* element);
void* queue_pop(struct queue_t* queue);
unsigned int queue_len(struct queue_t* queue);
