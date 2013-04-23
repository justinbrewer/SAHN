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

#include <pthread.h>
#include <stdlib.h>

pthread_t route_thread;

uint16_t local_address;
uint16_t num_physical_links;
uint16_t* physical_links = NULL;

void* route__run(void* params){
  return NULL;
}

int route_init(struct sahn_config_t* config){
  route_update_links();

  pthread_create(&route_thread,NULL,route__run,NULL);

  return 0;
}

int route_cleanup(){
  pthread_cancel(route_thread);
  pthread_join(route_thread,NULL);

  free(physical_links);

  return 0;
}

int route_update_links(){
  struct topo_node* node = topo_get_local_node();

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
  return 0;
}

int route_dispatch_packet(struct net_packet_t* packet){
  return 0;
}
