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

#include "route.h"
#include "topo.h"
#include "util/cache.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>

#define EVENT_BUFFER_LEN (1024*(sizeof(struct inotify_event)+16))
#define MAX_LINKS 32

char* topo_file;

struct cache_t* node_cache;
struct topo_node_t* topo_local_node;
uint16_t topo_local_address;

unsigned int topo_drop_divisor;

pthread_t topo_node_thread;
int topo_inotify_fd;

int topo__update_nodes(){
  char links_buf[64], *link;
  FILE* fp;
  struct topo_node_t* node;

  cache_lock(node_cache);
  cache_flush__crit(node_cache);
  cache_disable_sort__crit(node_cache);
  
  fp = fopen(topo_file,"r");
  
  while(!feof(fp)){
    node = (struct topo_node_t*)malloc(sizeof(struct topo_node_t));
    
    fscanf(fp,"Node %hu %64[^,], %8s %hd %hd links %64[^\n]\n",
	   &node->address,
	   &node->real_address,
	   &node->real_port,
	   &node->loc.x,
	   &node->loc.y,
	   links_buf);
    
    node->links = (uint16_t*)malloc(MAX_LINKS*sizeof(uint16_t));
    link = strtok(links_buf," ");
    while(link != NULL){
      node->links[node->num_links++] = atoi(link);
      link = strtok(NULL," ");
    }
    node->links = (uint16_t*)realloc(node->links,node->num_links*sizeof(uint16_t));

    cache_set__crit(node_cache,node->address,node);
  }
  fclose(fp);

  cache_enable_sort__crit(node_cache);
  cache_unlock(node_cache);

  topo_local_node = cache_get(node_cache,topo_local_address);

  return 0;
}

void* topo__run(void* params){
  int len;
  uint8_t event_buffer[EVENT_BUFFER_LEN];
  while(1){
    len = read(topo_inotify_fd,event_buffer,EVENT_BUFFER_LEN);

    if(len < 0){
      //TODO: Error
      continue;
    }

    topo__update_nodes();
    route_update_links();
  }
}

int topo_init(const char* file, uint16_t local_address, struct sahn_config_t* config){
  topo_file = (char*)malloc(strlen(file)+1);
  strcpy(topo_file,file);

  topo_local_address = local_address;

  node_cache = cache_create(free);
  topo__update_nodes();

  topo_drop_divisor = config->node_range;
  topo_drop_divisor *= topo_drop_divisor;
  topo_drop_divisor >>= 4;
  topo_drop_divisor *= topo_drop_divisor;

  topo_inotify_fd = inotify_init();
  inotify_add_watch(topo_inotify_fd,topo_file,IN_MODIFY);
  pthread_create(&topo_node_thread,NULL,topo__run,NULL);

  return 0;
}

int topo_cleanup(){
  pthread_cancel(topo_node_thread);
  pthread_join(topo_node_thread,NULL);
  close(topo_inotify_fd);

  free(topo_file);
  cache_destroy(node_cache);
  return 0;
}

struct topo_node_t* topo_get_local_node(){
  return topo_copy_node(topo_local_node);
}

struct topo_node_t* topo_get_node(uint16_t address){
  return topo_copy_node(cache_get(node_cache,address));
}

unsigned int topo_get_num_nodes(){
  return cache_len(node_cache);
}

uint32_t topo_drop_rate(uint16_t remote_node){
  int x, y;
  const struct topo_coord_t* a = &topo_local_node->loc;
  const struct topo_coord_t* b = &((struct topo_node_t*)cache_get(node_cache,remote_node))->loc;

  x = a->x - b->x;
  x *= x;
  y = a->y - b->y;
  y *= y;
  x += y; 
  x *= x;

  return x / topo_drop_divisor;
}

struct topo_node_t* topo_alloc_node(){
  struct topo_node_t* node = (struct topo_node_t*)malloc(sizeof(struct topo_node_t));
  memset(node,0,sizeof(struct topo_node_t));
  return node;
}

struct topo_node_t* topo_copy_node(struct topo_node_t* node){
  if(node == NULL){
    return NULL;
  }

  struct topo_node_t* res = topo_alloc_node();
  memcpy(res,node,sizeof(struct topo_node_t));

  res->links = (uint16_t*)malloc(res->num_links*sizeof(uint16_t));
  memcpy(res->links,node->links,res->num_links*sizeof(uint16_t));
  
  return res;
}

int topo_free_node(struct topo_node_t* node){
  if(node->links != NULL){
    free(node->links);
  }
  free(node);
  return 0;
}
