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
#include <stdbool.h>
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
  struct set_t* bidir;
} __attribute__((packed));

struct tc_entry_t {
  uint16_t destination;
  uint16_t last_hop;
  uint16_t seq;
  time_t last_seen;
};

struct rt_entry_t {
  uint16_t destination;
  uint16_t next_hop;
  uint16_t distance;
};

pthread_t route_thread;

uint16_t local_address;
uint16_t num_physical_links;
uint16_t* physical_links = NULL;

struct cache_t* neighbor_cache;

uint16_t mpr_seq;
bool mpr_changed;
struct set_t* mpr_selector;

time_t tc_last_sent;
struct cache_t* tc_table;

bool rt_update;
struct cache_t* rtable;

uint16_t seq_thr_high;
uint16_t seq_thr_low;
double neighbor_timeout;
double tc_timeout;

void route__free_neighbor(struct route_neighbor_t* n){
  if(n->bidir != NULL){
    set_destroy(n->bidir);
  }
  free(n);
}

int route__broadcast(struct net_packet_t* packet){
  int i;
  uint16_t destination = packet->destination, source = packet->source, prev_hop = packet->prev_hop;

  packet->prev_hop = local_address;

  net_hton(packet);
  for(i=0;i<num_physical_links;i++){
    if(physical_links[i] != source && physical_links[i] != prev_hop){
      udp_send(physical_links[i],packet,packet->size);
    }
  }
  net_ntoh(packet);
}

void route__check_expiry(){
  int i,len;
  struct route_neighbor_t** neighbor_list;
  struct tc_entry_t** tc_list;

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t**)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    if(difftime(time(NULL),neighbor_list[i]->last_heard) > neighbor_timeout){
      if(set_remove(mpr_selector,neighbor_list[i]->address)){
	mpr_seq++;
	mpr_changed = true;
	rt_update = true;
      }
      cache_delete__crit(neighbor_cache,neighbor_list[i]->address);
    }
  }

  free(neighbor_list);

  len = cache_len__crit(tc_table);
  tc_list = (struct tc_entry_t**)cache_get_list__crit(tc_table);

  for(i=0;i<len;i++){
    if(difftime(time(NULL),tc_list[i]->last_seen) > tc_timeout){
      cache_delete__crit(tc_table,tc_list[i]->destination);
      rt_update = true;
    }
  }

  free(tc_list);
}

void route__update_mpr(){
  int i,j,neighbor_len,max,max_i;
  struct route_neighbor_t** neighbor_list;
  struct set_t *two_hop, *two_hop_n;

  neighbor_len = cache_len__crit(neighbor_cache);

  if(neighbor_len == 0){
    return;
  }

  neighbor_list = (struct route_neighbor_t**)cache_get_list__crit(neighbor_cache);

  two_hop = set_create();

  for(i=0;i<neighbor_len;i++){
    if(neighbor_list[i]->state == NEIGHBOR_MPR){
      neighbor_list[i]->state = NEIGHBOR_BIDIRECTIONAL;
    }
  }

  while(1){
    max = two_hop->num;
    max_i = -1;

    for(i=0;i<neighbor_len;i++){
      if(neighbor_list[i]->state == NEIGHBOR_HEARD || neighbor_list[i]->state == NEIGHBOR_MPR){
	continue;
      }

      j = set_union_size(two_hop,neighbor_list[i]->bidir);

      if(j > max){
	max = j;
	max_i = i;
      }
    }

    if(max_i == -1){
      break;
    }

    neighbor_list[max_i]->state = NEIGHBOR_MPR;

    two_hop_n = set_union(two_hop,neighbor_list[max_i]->bidir);
    set_destroy(two_hop);
    two_hop = two_hop_n;
  }

  set_destroy(two_hop);
  free(neighbor_list);
}

void route__update_rtable(){
  bool done = false;
  int i,len,h=1;
  struct route_neighbor_t** neighbor_list;
  struct tc_entry_t **tc_list, *tc_entry;
  struct rt_entry_t *rt_entry, *rt_check;

  if(!rt_update){
    return;
  }
  rt_update = false;

  cache_flush__crit(rtable);

  len = cache_len__crit(neighbor_cache);
  neighbor_list = (struct route_neighbor_t**)cache_get_list__crit(neighbor_cache);

  for(i=0;i<len;i++){
    rt_entry = (struct rt_entry_t*)malloc(sizeof(struct rt_entry_t));
    rt_entry->destination = neighbor_list[i]->address;
    rt_entry->next_hop = neighbor_list[i]->address;
    rt_entry->distance = h;
    cache_set__crit(rtable,rt_entry->destination,rt_entry);
  }

  free(neighbor_list);

  len = cache_len__crit(tc_table);
  tc_list = (struct tc_entry_t**)cache_get_list__crit(tc_table);

  while(!done){
    for(i=0,done=true;i<len;i++){
      if(tc_list[i]->destination != local_address && cache_get__crit(rtable,tc_list[i]->destination) == NULL){
	rt_check = cache_get__crit(rtable,tc_list[i]->last_hop);
	if(rt_check != NULL && rt_check->distance == h){
	  done = false;

	  rt_entry = (struct rt_entry_t*)malloc(sizeof(struct rt_entry_t));
	  rt_entry->destination = tc_list[i]->destination;
	  rt_entry->next_hop = tc_list[i]->last_hop;
	  rt_entry->distance = h+1;
	  cache_set__crit(rtable,rt_entry->destination,rt_entry);
	}
      }
    }

    h++;
  }

  free(tc_list);
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

  route__broadcast(&packet);

  free(neighbor_list);
}

void route__send_tc(){
  int i;
  struct net_packet_t packet = {0};

  if(mpr_selector->num == 0){
    return;
  }

  if(!mpr_changed && difftime(time(NULL),tc_last_sent) < 2.0){
    return;
  }
  mpr_changed = false;
  tc_last_sent = time(NULL);

  packet.source = local_address;
  packet.destination = 0xFFFF;
  packet.route_control[0] = ROUTE_TC;
  packet.seq = mpr_seq;

  for(i=0;i<mpr_selector->num;i++){
    *(uint16_t*)(&packet.payload[i*2]) = mpr_selector->values[i];
  }

  *(uint16_t*)(&packet.payload[i++*2]) = 0;
  packet.size = NET_HEADER_SIZE + i*2;

  route__broadcast(&packet);
}

void* route__run(void* params){
  while(1){
    cache_lock(neighbor_cache);
    cache_lock(tc_table);
    cache_lock(rtable);

    route__check_expiry();
    route__update_mpr();
    route__update_rtable();
    route__send_hello();
    route__send_tc();

    cache_unlock(rtable);
    cache_unlock(tc_table);
    cache_unlock(neighbor_cache);

    sleep(1);
  }

  return NULL;
}

int route_init(struct sahn_config_t* config){
  route_update_links();
  neighbor_cache = cache_create((cache_free_t)route__free_neighbor);

  mpr_seq = 0;
  mpr_changed = false;
  mpr_selector = set_create();

  tc_table = cache_create(free);
  rtable = cache_create(free);

  seq_thr_high = config->seq_threshold_high;
  seq_thr_low = config->seq_threshold_low;
  neighbor_timeout = config->olsr_neighbor_timeout;
  tc_timeout = config->olsr_tc_timeout;

  pthread_create(&route_thread,NULL,route__run,NULL);

  return 0;
}

int route_cleanup(){
  pthread_cancel(route_thread);
  pthread_join(route_thread,NULL);

  cache_destroy(rtable);
  cache_destroy(tc_table);
  set_destroy(mpr_selector);
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
  int i, cap, addr, len;
  bool old;
  struct route_neighbor_t *neighbor, j;
  struct tc_entry_t **tc_list, *tc_entry;

  cache_lock(neighbor_cache);
  cache_lock(tc_table);

  switch(packet->route_control[0]){
  case ROUTE_HELLO:
    
    neighbor = cache_get__crit(neighbor_cache,packet->source);

    if(neighbor == NULL){
      neighbor = (struct route_neighbor_t*)malloc(sizeof(struct route_neighbor_t));
      memset(neighbor,0,sizeof(struct route_neighbor_t));
      neighbor->address = packet->source;
      neighbor->state = NEIGHBOR_HEARD;
      cache_set__crit(neighbor_cache,packet->source,neighbor);
    }

    i=0;

    if(neighbor->bidir != NULL){
      set_destroy(neighbor->bidir);
    }

    neighbor->bidir = set_create();

    while((*(uint32_t*)(&j) = *(uint32_t*)(&packet->payload[i++*4])) != 0){
      if(j.address == local_address){
	neighbor->state = NEIGHBOR_BIDIRECTIONAL;

	switch(j.state){
	case NEIGHBOR_MPR:
	  if(set_add(mpr_selector,packet->source)){
	    mpr_seq++;
	    mpr_changed = true;
	    rt_update = true;
	  }
	  break;
	case NEIGHBOR_BIDIRECTIONAL:
	  if(set_remove(mpr_selector,packet->source)){
	    mpr_seq++;
	    mpr_changed = true;
	    rt_update = true;
	  }
	  break;
	}
      }else{
	set_add(neighbor->bidir,j.address);
      }
    }

    for(i=0;i<packet->size-NET_HEADER_SIZE;i+=4){
      if(*(uint16_t*)(&packet->payload[i]) == local_address){
	neighbor->state = NEIGHBOR_BIDIRECTIONAL;
      }
    }

    neighbor->last_heard = time(NULL);
    break;

  case ROUTE_TC:

    len = cache_len__crit(tc_table);
    tc_list = (struct tc_entry_t**)cache_get_list__crit(tc_table);

    for(i=0,old=false;i<len;i++){
      if(tc_list[i]->last_hop == packet->source){
        if(!(tc_list[i]->seq < packet->seq || (tc_list[i]->seq > seq_thr_high && packet->seq < seq_thr_low))){
	  old = true;
	  break;
	}
      }
    }

    if(old){
      break;
    }

    for(i=0;i<len;i++){
      if(tc_list[i]->last_hop == packet->source){
	if(tc_list[i]->seq < packet->seq){
	  cache_delete__crit(tc_table,tc_list[i]->destination);
	}
      }
    }

    free(tc_list);

    i=0;
    while((addr = *(uint16_t*)(&packet->payload[i++*2]))){
      tc_entry = (struct tc_entry_t*)malloc(sizeof(struct tc_entry_t*));
      tc_entry->destination = addr;
      tc_entry->last_hop = packet->source;
      tc_entry->seq = packet->seq;
      tc_entry->last_seen = time(NULL);
      cache_set__crit(tc_table,(((uint32_t)tc_entry->destination)<<16)|(tc_entry->last_hop),tc_entry);
    }

    rt_update = true;

    if(mpr_selector->num > 0){
      route__broadcast(packet);
    }

    break;

  default:
    break;
  }

  cache_unlock(tc_table);
  cache_unlock(neighbor_cache);

  return 0;
}

int route_dispatch_packet(struct net_packet_t* packet){
  struct rt_entry_t* rt_entry = (struct rt_entry_t*)cache_get(rtable,packet->destination);

  if(rt_entry == NULL){
    //No route, drop =(
    return -1;
  }

  packet->prev_hop = local_address;

  net_hton(packet);
  udp_send(rt_entry->next_hop,packet,packet->size);

  return 0;
}
