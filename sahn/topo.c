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
#include "topo.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NODES 128
#define MAX_LINKS 32

char* topo_file;

struct cache_t* node_cache;
struct topo_node* topo_local_node;

unsigned int topo_drop_divisor;

int topo_init(const char* file, uint16_t local_address, struct sahn_config_t* config){
  char addr_buf[64], port_buf[8], links_buf[64], *link;
  FILE* fp;
  struct topo_node* node;

  node_cache = cache_create(free);
  cache_disable_sort(node_cache);

  topo_file = strdup(file);
  fp = fopen(topo_file,"r");

  while(!feof(fp)){
    node = (struct topo_node*)malloc(sizeof(struct topo_node));

    fscanf(fp,"Node %hu %64[^,], %8s %hd %hd links %64[^\n]\n",
	   &node->address,
	   addr_buf,
	   port_buf,
	   &node->loc.x,
	   &node->loc.y,
	   links_buf);

    node->real_address = strdup(addr_buf);
    node->real_port = strdup(port_buf);
    
    node->links = (uint16_t*)malloc(MAX_LINKS*sizeof(uint16_t));
    link = strtok(links_buf," ");
    while(link != NULL){
      node->links[node->num_links++] = atoi(link);
      link = strtok(NULL," ");
    }
    node->links = (uint16_t*)realloc(node->links,node->num_links*sizeof(uint16_t));

    cache_set(node_cache,node->address,node);
  }
  fclose(fp);

  cache_enable_sort(node_cache);

  topo_local_node = cache_get(node_cache,local_address);

  topo_drop_divisor = config->node_range;
  topo_drop_divisor *= topo_drop_divisor;
  topo_drop_divisor >>= 4;
  topo_drop_divisor *= topo_drop_divisor;

  return 0;
}

int topo_cleanup(){
  free(topo_file);
  cache_destroy(node_cache);
  return 0;
}

struct topo_node* topo_get_local_node(){
  return topo_copy_node(topo_local_node);
}

struct topo_node* topo_get_node(uint16_t address){
  return topo_copy_node(cache_get(node_cache,address));
}

unsigned int topo_get_num_nodes(){
  return cache_len(node_cache);
}

uint32_t topo_drop_rate(uint16_t remote_node){
  int x, y;
  const struct topo_coord* a = &topo_local_node->loc;
  const struct topo_coord* b = &((struct topo_node*)cache_get(node_cache,remote_node))->loc;

  x = a->x - b->x;
  x *= x;
  y = a->y - b->y;
  y *= y;
  x += y; 
  x *= x;

  return x / topo_drop_divisor;
}

struct topo_node* topo_alloc_node(){
  struct topo_node* node = (struct topo_node*)malloc(sizeof(struct topo_node));
  memset(node,0,sizeof(struct topo_node));
  return node;
}

struct topo_node* topo_copy_node(struct topo_node* node){
  if(node == NULL){
    return NULL;
  }

  struct topo_node* res = topo_alloc_node();
  memcpy(res,node,sizeof(struct topo_node));

  res->links = (uint16_t*)malloc(res->num_links*sizeof(uint16_t));
  memcpy(res->links,node->links,res->num_links*sizeof(uint16_t));

  res->real_address = strdup(node->real_address);
  res->real_port = strdup(node->real_port);
  
  return res;
}

int topo_free_node(struct topo_node* node){
  if(node->links != NULL){
    free(node->links);
  }
  if(node->real_address != NULL){
    free(node->real_address);
  }
  if(node->real_port != NULL){
    free(node->real_port);
  }
  free(node);
  return 0;
}
