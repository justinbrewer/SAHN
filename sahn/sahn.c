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
#include "sahn.h"
#include "topo.h"
#include "udp.h"

#include <stdbool.h>
#include <stdlib.h>

#define SAHN_EXPORT __attribute__((visibility("default")))

SAHN_EXPORT
struct sahn_config_t* sahn_config_create(){
  struct sahn_config_t* config = (struct sahn_config_t*)malloc(sizeof(struct sahn_config_t));

  config->node_range = 128;
  config->seq_threshold_low = 16;
  config->seq_threshold_high = 65520;

  return config;
}

SAHN_EXPORT
int sahn_config_destroy(struct sahn_config_t* config){
  free(config);
}

SAHN_EXPORT
int sahn_init(const char* topology_file, uint16_t node_address, struct sahn_config_t* config) {
  bool free_config = false;
  if(config == NULL){
    config = sahn_config_create();
    free_config = true;
  }

  topo_init(topology_file,node_address,config);
  udp_init(config);
  net_init(config);

  if(free_config){
    sahn_config_destroy(config);
  }

  return 0; //TODO: Proper error checking
}

SAHN_EXPORT
int sahn_cleanup() {
  net_cleanup();
  udp_cleanup();
  topo_cleanup();

  return 0;
}

SAHN_EXPORT
int sahn_send(uint16_t destination, void* data, uint32_t data_size) {
  return net_send(destination,data,data_size);
}

SAHN_EXPORT
int sahn_recv(uint16_t* source, void* buffer, uint32_t buffer_size) {
  return net_recv(source,buffer,buffer_size);
}
