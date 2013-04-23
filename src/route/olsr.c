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

int route_init(struct sahn_config_t* config){
  return 0;
}

int route_cleanup(){
  return 0;
}

int route_update_links(){
  return 0;
}

int route_control_packet(struct net_packet_t* packet){
  return 0;
}

int route_dispatch_packet(struct net_packet_t* packet){
  return 0;
}
