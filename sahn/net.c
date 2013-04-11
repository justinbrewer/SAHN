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
#include "queue.h"
#include "seq.h"
#include "topo.h"
#include "udp.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define MAX_PAYLOAD 116
#define BUFFER_LEN 128
#define HEADER_SIZE 12

struct net_packet {
  uint16_t source;
  uint16_t destination;
  uint16_t prev_hop;
  uint16_t seq;
  uint8_t size;
  uint8_t __header_padding[3];
  uint8_t payload[MAX_PAYLOAD];
} __attribute__((__packed__));

uint16_t local_address;
uint16_t num_links;
uint16_t* links;

struct queue_t* net_recv_queue;

pthread_t net_run_thread;

void net__hton(struct net_packet* packet){
  packet->source = htons(packet->source);
  packet->destination = htons(packet->destination);
  packet->prev_hop = htons(packet->prev_hop);
  packet->seq = htons(packet->seq);
}

void net__ntoh(struct net_packet* packet){
  packet->source = ntohs(packet->source);
  packet->destination = ntohs(packet->destination);
  packet->prev_hop = ntohs(packet->prev_hop);
  packet->seq = ntohs(packet->seq);
}

struct net_packet* net__copy_packet(struct net_packet* in){
  struct net_packet* out = (struct net_packet*)malloc(sizeof(struct net_packet));
  memcpy(out,in,sizeof(struct net_packet));
  return out;
}

int net__dispatch_packet(struct net_packet* packet){
  int i;
  uint16_t prev_hop = packet->prev_hop, source = packet->source, destination = packet->destination, seq = packet->seq;
  uint8_t size = packet->size;
  packet->prev_hop = local_address;

  net__hton(packet);
  
  for(i=0;i<num_links;i++){
    if(links[i] != prev_hop && links[i] != source){
      if((rand() & TOPO_PMASK) >= topo_drop_rate(links[i])){
	printf("(%d) OUT:\t(%d)\t(%d)\t%d:%d\t-> %d\n",local_address,links[i],prev_hop,source,seq,destination);
	udp_send(links[i],packet,size);
      } else {
	printf("(%d) DROP:\t(%d)\t(%d)\t%d:%d\t-> %d\n",local_address,links[i],prev_hop,source,seq,destination);
      }
    }
  }

  return 0;
}

void* net__run(void* params){
  struct net_packet packet = {0};

  while(1){
    udp_recv(NULL,&packet,sizeof(struct net_packet));
    net__ntoh(&packet);

    if(!seq_check(packet.source,packet.seq)){
      printf("(%d) OLD:\t\t(%d)\t%d:%d\t-> %d\n",local_address,packet.prev_hop,packet.source,packet.seq,packet.destination);
      continue;
    }

    printf("(%d) IN: \t\t(%d)\t%d:%d\t-> %d\n",local_address,packet.prev_hop,packet.source,packet.seq,packet.destination);

    if(packet.destination == local_address){
      queue_push(net_recv_queue,net__copy_packet(&packet));
      continue;
    }

    net__dispatch_packet(&packet);
  }
}

int net_init(struct sahn_config_t* config){
  udp_init(config);

  net_recv_queue = queue_create();

  struct topo_node* node = topo_get_local_node();
  
  local_address = node->address;
  num_links = node->num_links;

  links = node->links;
  node->links = NULL;

  topo_free_node(node);

  seq_init(topo_get_num_nodes(),config);

  pthread_create(&net_run_thread,NULL,net__run,NULL);

  return 0;
}

int net_cleanup(){
  pthread_cancel(net_run_thread);
  pthread_join(net_run_thread,NULL);

  seq_cleanup();

  free(links);

  queue_destroy(net_recv_queue);

  udp_cleanup();

  return 0;
}

int net_send(uint16_t destination, void* data, uint32_t data_size){
  static uint16_t seq = 0;
  struct net_packet packet = {0};

  packet.source = local_address;
  packet.destination = destination;
  packet.seq = seq++;

  if(data_size > MAX_PAYLOAD){
    //TODO: What to do with it? For now we'll just drop :P
    return -1;
  }

  packet.size = data_size + HEADER_SIZE;
  memcpy(&packet.payload,data,data_size);

  net__dispatch_packet(&packet);
  return data_size;
}

int net_recv(uint16_t* source, void* buffer, uint32_t buffer_size){
  int size;
  struct net_packet* packet;

  packet = queue_pop(net_recv_queue);

  if(source != NULL){
    *source = packet->source;
  }

  //TODO: What should we do if the buffer is smaller than the packet?
  size = packet->size - HEADER_SIZE;

  memcpy(buffer,packet->payload,size);

  free(packet);

  return size;
}
