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

  char* real_address;
  char* real_port;
};

int topo_init(const char* file, uint16_t local_address);
int topo_cleanup();

struct topo_node* topo_get_local_node();
struct topo_node* topo_get_node(uint16_t address);

struct topo_node* topo_alloc_node();
struct topo_node* topo_copy_node(struct topo_node* node);
int topo_free_node(struct topo_node* node);