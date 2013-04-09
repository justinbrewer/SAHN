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

#include "sahn.h"

#include <stdint.h>

#define TOPO_PRANGE 256
#define TOPO_PMASK 0xFF

struct topo_coord {
  int16_t x;
  int16_t y;
};

struct topo_node {
  uint16_t address;

  struct topo_coord loc;

  uint16_t num_links;
  uint16_t* links;

  char* real_address;
  char* real_port;
};

int topo_init(const char* file, uint16_t local_address, struct sahn_config_t* config);
int topo_cleanup();

struct topo_node* topo_get_local_node();
struct topo_node* topo_get_node(uint16_t address);
unsigned int topo_get_num_nodes();

uint32_t topo_drop_rate(uint16_t remote_node);

struct topo_node* topo_alloc_node();
struct topo_node* topo_copy_node(struct topo_node* node);
int topo_free_node(struct topo_node* node);
