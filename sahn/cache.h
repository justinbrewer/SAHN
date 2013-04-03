#pragma once

#include <string.h>
#include <stdint.h>

struct cache_t;
typedef void (*cache_free_t)(void*);

struct cache_t* cache_create(cache_free_t free_callback);
int cache_destroy(struct cache_t* cache);

int cache_enable_sort(struct cache_t* cache);
int cache_disable_sort(struct cache_t* cache);

void* cache_get(struct cache_t* cache, uint32_t key);
int cache_set(struct cache_t* cache, uint32_t key, void* val);
uint32_t cache_len(struct cache_t* cache);
