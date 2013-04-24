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
  int bidir[32];
  uint32_t bidir_count;
} __attribute__((packed));

pthread_t route_thread;

uint16_t local_address;
uint16_t num_physical_links;
uint16_t* physical_links = NULL;

struct cache_t* neighbor_cache;

void route__check_expiry(){
  int i,len;
  struct route_neighbor_t* neighbor_list;

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t*)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    if(difftime(time(NULL),neighbor_list[i].last_heard) > 10.0){
      cache_delete__crit(neighbor_cache,neighbor_list[i].address);
    }
  }

  free(neighbor_list);
}

void route__update_mpr(){
  int i,j,len,max,max_i,two_hops_len=0,done=0;
  struct route_neighbor_t* neighbor_list;
  int *two_hops=NULL, *two_hops_n;

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t*)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    if(neighbor_list[i].state == NEIGHBOR_MPR){
      ((struct route_neighbor_t*)cache_get__crit(neighbor_cache,neighbor_list[i].address))->state = NEIGHBOR_BIDIRECTIONAL;
    }
  }

  while(1){
    max = two_hops_len;
    max_i = -1;
    for(i=0;i<len;i++){
      if(neighbor_list[i].state == NEIGHBOR_HEARD){
	continue;
      }

      j = set_union_size(two_hops,two_hops_len,neighbor_list[i].bidir,neighbor_list[i].bidir_count);
      if(j > max){
	max = j;
	max_i = i;
      }
    }

    if(max_i == -1){
      break;
    }

    two_hops_n = set_union(two_hops,two_hops_len,neighbor_list[max_i].bidir,neighbor_list[max_i].bidir_count,&two_hops_len);

    if(two_hops != NULL){
      free(two_hops);
    }

    two_hops = two_hops_n;

    neighbor_list[max_i].state = NEIGHBOR_MPR;
  }

  free(neighbor_list);
}

void route__send_hello(){
  int i,j=0,len;
  struct route_neighbor_t* neighbor_list;
  struct net_packet_t packet = {0};
  
  packet.source = local_address;
  packet.destination = 0xFFFF;
  packet.route_control[0] = ROUTE_HELLO;

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t*)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    if(neighbor_list[i].state == NEIGHBOR_BIDIRECTIONAL || neighbor_list[i].state == NEIGHBOR_MPR){
      *(uint32_t*)(&packet.payload[j++*4]) = *(uint32_t*)(&neighbor_list[i]);
    }
  }

  *(uint32_t*)(&packet.payload[j++*4]) = 0;

  for(i=0;i<len;i++){
    if(neighbor_list[i].state == NEIGHBOR_HEARD){
      *(uint32_t*)(&packet.payload[j++*4]) = *(uint32_t*)(&neighbor_list[i]);
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
  neighbor_cache = cache_create(sizeof(struct route_neighbor_t),free);

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
  int i;
  struct route_neighbor_t *neighbor, hop_neighbor;

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

    for(i=0,neighbor->bidir_count=0;(neighbor->bidir[i] = *(uint16_t*)(&packet->payload[i]));i+=4){
      neighbor->bidir_count++;
      if(neighbor->bidir[i] == local_address){
	neighbor->state = NEIGHBOR_BIDIRECTIONAL;
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
