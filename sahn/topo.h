#pragma once

#include <stdint.h>

struct topo_coord {
  uint16_t x;
  uint16_t y;
};

struct topo_node {
  uint16_t address;
  struct topo_coord loc;
  uint16_t num_links;
  uint16_t* links;
};

int topo_init(const char* file, uint16_t local_node);
int topo_cleanup();

int topo_get_local_node(struct topo_node* res);
int topo_get_node(uint16_t address, struct topo_node* res);
