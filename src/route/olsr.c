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
#include "udp.h"
#include "util/cache.h"
#include "util/set.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { ROUTE_HELLO=1, ROUTE_TC=2 } route_control_id_t;
typedef enum { NEIGHBOR_HEARD=1, NEIGHBOR_BIDIRECTIONAL=2, NEIGHBOR_MPR=3 } route_link_state_t;

struct route_neighbor_t {
  uint16_t address;
  uint8_t state;
  uint8_t __zero;
  time_t last_heard;
  int* bidir;
  uint32_t bidir_count;
} __attribute__((packed));

pthread_t route_thread;

uint16_t local_address;
uint16_t num_physical_links;
uint16_t* physical_links = NULL;

struct cache_t* neighbor_cache;

void route__check_expiry(){
  int i,len;
  struct route_neighbor_t** neighbor_list;

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t**)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    if(difftime(time(NULL),neighbor_list[i]->last_heard) > 10.0){
      cache_delete__crit(neighbor_cache,neighbor_list[i]->address);
    }
  }

  free(neighbor_list);
}

void route__update_mpr(){
  int i,j,neighbor_len,max,max_i,two_hop_len=0,*two_hop=NULL,*two_hop_n;
  struct route_neighbor_t** neighbor_list;

  neighbor_len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t**)cache_get_list__crit(neighbor_cache);

  if(neighbor_len == 0){
    return;
  }

  for(i=0;i<neighbor_len;i++){
    if(neighbor_list[i]->state == NEIGHBOR_MPR){
      neighbor_list[i]->state = NEIGHBOR_BIDIRECTIONAL;
    }
  }

  while(1){
    max = two_hop_len;
    max_i = -1;

    for(i=0;i<neighbor_len;i++){
      if(neighbor_list[i]->state == NEIGHBOR_HEARD){
	continue;
      }

      j = set_union_size(two_hop,two_hop_len,neighbor_list[i]->bidir,neighbor_list[i]->bidir_count);

      if(j > max){
	max = j;
	max_i = i;
      }
    }

    if(max_i == -1){
      break;
    }

    neighbor_list[max_i]->state = NEIGHBOR_MPR;

    two_hop_n = set_union(two_hop,two_hop_len,neighbor_list[max_i]->bidir,neighbor_list[max_i]->bidir_count,&two_hop_len);
    if(two_hop != NULL){
      free(two_hop);
    }
    two_hop = two_hop_n;
  }

  free(neighbor_list);
}

void route__send_hello(){
  int i,j=0,len;
  struct route_neighbor_t** neighbor_list;
  struct net_packet_t packet = {0};
  
  packet.source = local_address;
  packet.destination = 0xFFFF;
  packet.route_control[0] = ROUTE_HELLO;

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t**)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    if(neighbor_list[i]->state == NEIGHBOR_BIDIRECTIONAL || neighbor_list[i]->state == NEIGHBOR_MPR){
      *(uint32_t*)(&packet.payload[j++*4]) = *(uint32_t*)(neighbor_list[i]);
    }
  }

  *(uint32_t*)(&packet.payload[j++*4]) = 0;

  for(i=0;i<len;i++){
    if(neighbor_list[i]->state == NEIGHBOR_HEARD){
      *(uint32_t*)(&packet.payload[j++*4]) = *(uint32_t*)(neighbor_list[i]);
    }
  }

  packet.size = NET_HEADER_SIZE + j*4;

  net_hton(&packet);
  for(i=0;i<num_physical_links;i++){
    udp_send(physical_links[i],&packet,packet.size);
  }

  free(neighbor_list);
}

void* route__run(void* params){
  while(1){
    cache_lock(neighbor_cache);

    route__check_expiry();
    route__update_mpr();
    route__send_hello();

    cache_unlock(neighbor_cache);

    sleep(1);
  }

  return NULL;
}

int route_init(struct sahn_config_t* config){
  route_update_links();
  neighbor_cache = cache_create(free);

  pthread_create(&route_thread,NULL,route__run,NULL);

  return 0;
}

int route_cleanup(){
  pthread_cancel(route_thread);
  pthread_join(route_thread,NULL);

  cache_destroy(neighbor_cache);
  free(physical_links);

  return 0;
}

int route_update_links(){
  struct topo_node_t* node = topo_get_local_node();

  if(physical_links != NULL){
    free(physical_links);
  }

  local_address = node->address;
  num_physical_links = node->num_links;

  physical_links = node->links;
  node->links = NULL;

  topo_free_node(node);

  return 0;
}

int route_control_packet(struct net_packet_t* packet){
  int i, cap, addr;
  struct route_neighbor_t *neighbor;

  switch(packet->route_control[0]){
  case ROUTE_HELLO:
    cache_lock(neighbor_cache);
    neighbor = cache_get__crit(neighbor_cache,packet->source);

    if(neighbor == NULL){
      neighbor = (struct route_neighbor_t*)malloc(sizeof(struct route_neighbor_t));
      memset(neighbor,0,sizeof(struct route_neighbor_t));
      neighbor->address = packet->source;
      neighbor->state = NEIGHBOR_HEARD;
      cache_set__crit(neighbor_cache,packet->source,neighbor);
    }

    i=0;
    cap=16;

    if(neighbor->bidir != NULL){
      free(neighbor->bidir);
    }

    neighbor->bidir = (int*)malloc(cap*sizeof(int));
    neighbor->bidir_count = 0;

    while((addr = *(uint16_t*)(&packet->payload[i*4])) != 0){
      neighbor->bidir[i++] = addr;
      neighbor->bidir_count++;

      if(addr == local_address){
	neighbor->state = NEIGHBOR_BIDIRECTIONAL;
      }

      if(cap == neighbor->bidir_count){
	cap = (cap*3)/2;
	neighbor->bidir = (int*)realloc(neighbor->bidir,cap*sizeof(int));
      }
    }

    for(i=0;i<packet->size-NET_HEADER_SIZE;i+=4){
      if(*(uint16_t*)(&packet->payload[i]) == local_address){
	neighbor->state = NEIGHBOR_BIDIRECTIONAL;
      }
    }

    neighbor->last_heard = time(NULL);
    cache_unlock(neighbor_cache);
    break;

  case ROUTE_TC:
    break;
  default:
    break;
  }

  return 0;
}

int route_dispatch_packet(struct net_packet_t* packet){
  return 0;
}
