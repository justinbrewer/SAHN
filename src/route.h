#pragma once

#include "sahn.h"
#include "net.h"

int route_init(struct sahn_config_t* config);
int route_cleanup();

int route_dispatch_packet(struct net_packet_t* packet);
