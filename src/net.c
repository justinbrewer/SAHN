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

#include "net.h"
#include "seq.h"
#include "topo.h"
#include "udp.h"
#include "util/queue.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

uint16_t local_address;

struct queue_t* net_recv_queue;

pthread_t net_run_thread;

void net_hton(struct net_packet_t* packet){
  packet->source = htons(packet->source);
  packet->destination = htons(packet->destination);
  packet->prev_hop = htons(packet->prev_hop);
  packet->seq = htons(packet->seq);
}

void net_ntoh(struct net_packet_t* packet){
  packet->source = ntohs(packet->source);
  packet->destination = ntohs(packet->destination);
  packet->prev_hop = ntohs(packet->prev_hop);
  packet->seq = ntohs(packet->seq);
}

struct net_packet_t* net__copy_packet(struct net_packet_t* in){
  struct net_packet_t* out = (struct net_packet_t*)malloc(sizeof(struct net_packet_t));
  memcpy(out,in,sizeof(struct net_packet_t));
  return out;
}

void* net__run(void* params){
  struct net_packet_t packet = {0};

  while(1){
    udp_recv(NULL,&packet,sizeof(struct net_packet_t));
    net_ntoh(&packet);

    //TODO: Should route control packets use seq table?
    if(packet.route_control[0] != 0){
      route_control_packet(&packet);
      continue;
    }

    if(!seq_check(packet.source,packet.seq)){
      continue;
    }

    if(packet.destination == local_address){
      queue_push(net_recv_queue,net__copy_packet(&packet));
      continue;
    }

    route_dispatch_packet(&packet);
  }
}

int net_init(struct sahn_config_t* config){
  udp_init(config);

  net_recv_queue = queue_create();

  struct topo_node_t* node = topo_get_local_node();
  local_address = node->address;
  topo_free_node(node);

  seq_init(topo_get_num_nodes(),config);
  route_init(config);

  pthread_create(&net_run_thread,NULL,net__run,NULL);

  return 0;
}

int net_cleanup(){
  pthread_cancel(net_run_thread);
  pthread_join(net_run_thread,NULL);

  route_cleanup();
  seq_cleanup();

  queue_destroy(net_recv_queue);

  udp_cleanup();

  return 0;
}

int net_send(uint16_t destination, void* data, uint32_t data_size){
  static uint16_t seq = 0;
  struct net_packet_t packet = {0};

  packet.source = local_address;
  packet.destination = destination;
  packet.seq = seq++;

  if(data_size > NET_MAX_PAYLOAD){
    //TODO: What to do with it? For now we'll just drop :P
    return -1;
  }

  packet.size = data_size + NET_HEADER_SIZE;
  memcpy(&packet.payload,data,data_size);

  route_dispatch_packet(&packet);
  return data_size;
}

int net_recv(uint16_t* source, void* buffer, uint32_t buffer_size){
  int size;
  struct net_packet_t* packet;

  packet = queue_pop(net_recv_queue);

  if(source != NULL){
    *source = packet->source;
  }

  //TODO: What should we do if the buffer is smaller than the packet?
  size = packet->size - NET_HEADER_SIZE;

  memcpy(buffer,packet->payload,size);

  free(packet);

  return size;
}
