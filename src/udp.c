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

#include "topo.h"
#include "udp.h"
#include "util/cache.h"

#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

int udp_socket;
struct cache_t* addr_cache;

int udp_init(struct sahn_config_t* config){
  struct sockaddr_in addr = {0};
  struct topo_node_t* local_node = topo_get_local_node();

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(atoi(local_node->real_port));

  udp_socket = socket(AF_INET,SOCK_DGRAM,0);
  bind(udp_socket,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));

  topo_free_node(local_node);

  addr_cache = cache_create((cache_free_t)freeaddrinfo);
  return 0;
}

int udp_cleanup(){
  cache_destroy(addr_cache);
  close(udp_socket);
  return 0;
}

int udp_send(uint16_t destination, void* data, uint32_t data_size){
  struct addrinfo hints = {0}, *addr;
  struct topo_node_t* node;

  addr = cache_get(addr_cache,destination);

  if(addr == NULL){
    node = topo_get_node(destination);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
  
    getaddrinfo(node->real_address,node->real_port,&hints,&addr);

    cache_set(addr_cache,destination,addr);

    topo_free_node(node);
  }

  return sendto(udp_socket,data,data_size,0,addr->ai_addr,addr->ai_addrlen);
}

int udp_recv(uint16_t* source, void* buffer, uint32_t buffer_size){
  //TODO determine source simulated address
  return recvfrom(udp_socket,buffer,buffer_size,0,NULL,NULL);
}
