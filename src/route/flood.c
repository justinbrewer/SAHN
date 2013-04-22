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
#include "route.h"
#include "topo.h"
#include "udp.h"

#include <stdlib.h>

uint16_t local_address;
uint16_t num_links;
uint16_t* links;

int route_init(struct sahn_config_t* config){
  struct topo_node* node = topo_get_local_node();
  
  local_address = node->address;
  num_links = node->num_links;

  links = node->links;
  node->links = NULL;

  topo_free_node(node);

  return 0;
}

int route_cleanup(){
  free(links);

  return 0;
}

int route_update_links(){
  struct topo_node* node = topo_get_local_node();

  free(links);

  num_links = node->num_links;

  links = node->links;
  node->links = NULL;

  topo_free_node(node);

  return 0;
}

int route_dispatch_packet(struct net_packet_t* packet){
  int i;
  uint16_t prev_hop = packet->prev_hop, source = packet->source;
  uint8_t size = packet->size;
  packet->prev_hop = local_address;

  net_hton(packet);
  
  for(i=0;i<num_links;i++){
    if(links[i] != prev_hop && links[i] != source){
      if((rand() & TOPO_PMASK) >= topo_drop_rate(links[i])){
	udp_send(links[i],packet,size);
      }
    }
  }

  return 0;
}
