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

#define NET_MAX_PAYLOAD 116
#define NET_HEADER_SIZE 12

struct net_packet_t {
  uint16_t source;
  uint16_t destination;
  uint16_t prev_hop;
  uint16_t seq;
  uint8_t size;
  uint8_t route_control[3];
  uint8_t payload[NET_MAX_PAYLOAD];
} __attribute__((__packed__));

int net_init(struct sahn_config_t* config);
int net_cleanup();

int net_send(uint16_t destination, void* data, uint32_t data_size);
int net_recv(uint16_t* source, void* buffer, uint32_t buffer_size);

void net_hton(struct net_packet_t* packet);
void net_ntoh(struct net_packet_t* packet);
