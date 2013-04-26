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

#include <stdbool.h>
#include <stdlib.h>

#define SAHN_EXPORT __attribute__((visibility("default")))

/** \brief Initialize a config struct
 *
 *  Allocates a new config struct and populates it with default values.
 */
SAHN_EXPORT
struct sahn_config_t* sahn_config_create(){
  struct sahn_config_t* config = (struct sahn_config_t*)malloc(sizeof(struct sahn_config_t));

  config->node_range = 128;
  config->seq_threshold_low = 16;
  config->seq_threshold_high = 65520;
  config->olsr_neighbor_timeout = 10.0;
  config->olsr_tc_timeout = 10.0;

  return config;
}

/** \brief Free config struct
 */
SAHN_EXPORT
int sahn_config_destroy(struct sahn_config_t* config){
  free(config);
}

/** \brief Initialise simulator
 *
 *  Initialises simulator modules and starts network routing thread.
 *
 *  \param topology_file Text file describing the simulated network layout
 *  \param node_address Address of the node that this instance should represent
 *  \parma config Configuration options. If NULL, defaults are assumed.
 */
SAHN_EXPORT
int sahn_init(const char* topology_file, uint16_t node_address, struct sahn_config_t* config) {
  bool free_config = false;
  if(config == NULL){
    config = sahn_config_create();
    free_config = true;
  }

  topo_init(topology_file,node_address,config);
  net_init(config);

  if(free_config){
    sahn_config_destroy(config);
  }

  return 0; //TODO: Proper error checking
}

/** \brief Shutdown simulator
 *
 *  Stops simulator and cleans up modules
 */
SAHN_EXPORT
int sahn_cleanup() {
  net_cleanup();
  topo_cleanup();

  return 0;
}

/** \brief Send a packet
 *
 *  Sends a packet over the simulated network
 *
 *  \param destination Address of destination node
 *  \param data Packet to send
 *  \param data_size Size of packet in bytes
 */
SAHN_EXPORT
int sahn_send(uint16_t destination, void* data, uint32_t data_size) {
  return net_send(destination,data,data_size);
}

/** \brief Receive a packet
 *
 *  Blocks until a packet arrives on the simulated network.
 *
 *  \param source Set to the address the packet originated from
 *  \param buffer Buffer to store incoming packet in
 *  \param buffer_size Size of buffer in bytes
 */
SAHN_EXPORT
int sahn_recv(uint16_t* source, void* buffer, uint32_t buffer_size) {
  return net_recv(source,buffer,buffer_size);
}
