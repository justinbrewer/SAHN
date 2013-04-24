/* This file is part of libsahn
 * Copyright (c) 2013 Justin Brewer
 *
 * libsahn is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/>.
 */

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
  pthread_rwlock_t lock;
};

int cache__compare(const void* a, const void* b){
  return ((const struct cache_entry_t*)a)->key - ((const struct cache_entry_t*)b)->key;
}

struct cache_entry_t* cache__find(struct cache_t* cache, uint32_t key){
  unsigned int i;
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

  pthread_rwlock_init(&cache->lock,NULL);

  return cache;
}

int cache_destroy(struct cache_t* cache){
  unsigned int i;

  pthread_rwlock_destroy(&cache->lock);
  
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
  int ret;

  pthread_rwlock_wrlock(&cache->lock);

  ret = cache_enable_sort__crit(cache);

  pthread_rwlock_unlock(&cache->lock);

  return ret;
}

int cache_disable_sort(struct cache_t* cache){
  int ret;

  pthread_rwlock_wrlock(&cache->lock);

  ret = cache_disable_sort__crit(cache);

  pthread_rwlock_unlock(&cache->lock);

  return ret;
}

void* cache_get(struct cache_t* cache, uint32_t key){
  void* val = NULL;

  pthread_rwlock_rdlock(&cache->lock);

  val = cache_get__crit(cache,key);

  pthread_rwlock_unlock(&cache->lock);

  return val;
}

int cache_set(struct cache_t* cache, uint32_t key, void* val){
  int ret;

  pthread_rwlock_wrlock(&cache->lock);

  ret = cache_set__crit(cache,key,val);

  pthread_rwlock_unlock(&cache->lock);

  return ret;
}

int cache_delete(struct cache_t* cache, uint32_t key){
  int ret;

  pthread_rwlock_wrlock(&cache->lock);

  ret = cache_delete__crit(cache,key);

  pthread_rwlock_unlock(&cache->lock);

  return ret;
}

uint32_t cache_len(struct cache_t* cache){
  uint32_t len;

  pthread_rwlock_rdlock(&cache->lock);

  len = cache_len__crit(cache);

  pthread_rwlock_unlock(&cache->lock);

  return len;
}

void** cache_get_list(struct cache_t* cache){
  void** ret;

  pthread_rwlock_rdlock(&cache->lock);

  ret = cache_get_list__crit(cache);

  pthread_rwlock_unlock(&cache->lock);

  return ret;
}

int cache_lock(struct cache_t* cache){
  pthread_rwlock_wrlock(&cache->lock);
  return 0;
}

int cache_unlock(struct cache_t* cache){
  pthread_rwlock_unlock(&cache->lock);
  return 0;
}

int cache_enable_sort__crit(struct cache_t* cache){
  if(!cache->sort){
    cache->sort = true;
    qsort(cache->entries,cache->num,sizeof(struct cache_entry_t),cache__compare);
  }

  return 0;
}

int cache_disable_sort__crit(struct cache_t* cache){
  cache->sort = false;
  return 0;
}

int cache_flush__crit(struct cache_t* cache){
  int i;

  if(cache->free_callback != NULL){
     for(i=0;i<cache->num;i++){
       cache->free_callback(cache->entries[i].val);
     }
  }

  free(cache->entries);

  cache->entries = (struct cache_entry_t*)malloc(cache->cap*sizeof(struct cache_entry_t));
  memset(cache->entries,0,cache->cap*sizeof(struct cache_entry_t));

  cache->num = 0;
  cache->cap = 16;

  return 0;
}

void* cache_get__crit(struct cache_t* cache, uint32_t key){
  void* val = NULL;
  struct cache_entry_t* entry;

  entry = cache__find(cache,key);

  if(entry != NULL){
    val = entry->val;
  }

  return val;
}

int cache_set__crit(struct cache_t* cache, uint32_t key, void* val){
  struct cache_entry_t* entry;

  entry = cache__find(cache,key);

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

int cache_delete__crit(struct cache_t* cache, uint32_t key){
  int index;
  struct cache_entry_t* entry;

  entry = cache__find(cache,key);

  if(entry == NULL){
    return -1;
  }

  if(cache->free_callback != NULL){
    cache->free_callback(entry->val);
  }

  index = (entry - cache->entries)/sizeof(struct cache_entry_t);

  if(index != cache->num - 1){
    memmove(entry,entry+sizeof(struct cache_entry_t),(cache->num - index)*sizeof(struct cache_entry_t));
  } else {
    memset(entry,0,sizeof(struct cache_entry_t));
  }

  cache->num--;

  return 0;
}

uint32_t cache_len__crit(struct cache_t* cache){
  return cache->num;
}

void** cache_get_list__crit(struct cache_t* cache){
  int i;
  void** list = (void**)malloc(cache->num * sizeof(void*));

  for(i=0;i<cache->num;i++){
    list[i] = cache->entries[i].val;
  }

  return list;
}
