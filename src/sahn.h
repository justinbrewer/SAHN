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

#pragma once

#include <stdint.h>

struct sahn_config_t {
  uint16_t node_range;

  uint16_t seq_threshold_low;
  uint16_t seq_threshold_high;

  double olsr_neighbor_timeout;
  double olsr_tc_timeout;
};

struct sahn_config_t* sahn_config_create();
int sahn_config_destroy(struct sahn_config_t* config);

int sahn_init(const char* topology_file, uint16_t node_address, struct sahn_config_t* config);
int sahn_cleanup();

int sahn_send(uint16_t destination, void* data, uint32_t data_size);
int sahn_recv(uint16_t* source, void* buffer, uint32_t buffer_size);
