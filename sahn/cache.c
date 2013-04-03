#include "cache.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct cache_entry_t {
  uint32_t key;
  void* val;
};

struct cache_t {
  struct cache_entry_t* entries;
  uint32_t num;
  uint32_t cap;
  
  bool sort;
  cache_free_t free_callback;
};

int cache__compare(const void* a, const void* b){
  return ((const struct cache_entry_t*)a)->key - ((const struct cache_entry_t*)b)->key;
}

struct cache_entry_t* cache__find(struct cache_t* cache, uint32_t key){
  int i;
  struct cache_entry_t _key = {.key=key};
  
  if(cache->sort){
    return bsearch(&_key,cache->entries,cache->num,sizeof(struct cache_entry_t),cache__compare);
  } else {
    for(i=0;i<cache->num;i++){
      if(cache__compare(&_key,&cache->entries[i]) == 0){
	return &cache->entries[i];
      }
    }
  }

  return NULL;
} 

struct cache_t* cache_create(cache_free_t free_callback){
  struct cache_t* cache = (struct cache_t*)malloc(sizeof(struct cache_t));

  cache->num = 0;
  cache->cap = 16;

  cache->entries = (struct cache_entry_t*)malloc(cache->cap*sizeof(struct cache_entry_t));
  memset(cache->entries,0,cache->cap*sizeof(struct cache_entry_t));

  cache->sort = true;
  cache->free_callback = free_callback;

  return cache;
}

int cache_destroy(struct cache_t* cache){
  int i;
  
  if(cache->free_callback != NULL){
     for(i=0;i<cache->num;i++){
       cache->free_callback(cache->entries[i].val);
     }
  }

  free(cache->entries);
  free(cache);
  return 0;
}

int cache_enable_sort(struct cache_t* cache){
  if(!cache->sort){
    cache->sort = true;
    qsort(cache->entries,cache->num,sizeof(struct cache_entry_t),cache__compare);
  }

  return 0;
}

int cache_disable_sort(struct cache_t* cache){
  cache->sort = false;
  return 0;
}

void* cache_get(struct cache_t* cache, uint32_t key){
  struct cache_entry_t* entry = cache__find(cache,key);

  if(entry == NULL){
    return NULL;
  }

  return entry->val;
}

int cache_set(struct cache_t* cache, uint32_t key, void* val){
  struct cache_entry_t* entry = cache__find(cache,key);

  if(entry == NULL){
    if(cache->num == cache->cap){
      cache->cap = (cache->cap*3)>>1;
      cache->entries = (struct cache_entry_t*)realloc(cache->entries,cache->cap*sizeof(struct cache_entry_t));
    }
    entry = &cache->entries[cache->num++];

    entry->key = key;
    entry->val = val;

    if(cache->sort){
      //TODO: An almost sorted list is a bad case for quicksort, use something else?
      qsort(cache->entries,cache->num,sizeof(struct cache_entry_t),cache__compare);
    }
  } else {
    if(cache->free_callback != NULL){
      cache->free_callback(entry->val);
    }

    entry->val = val;
  }

  return 0;
}
