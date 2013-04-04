#pragma once

#include <stdint.h>

struct sahn_config_t {
  uint16_t node_range;

  uint16_t seq_threshold_low;
  uint16_t seq_threshold_high;
};

struct sahn_config_t* sahn_config_create();
int sahn_config_destroy(struct sahn_config_t* config);

int sahn_init(const char* topology_file, uint16_t node_address, struct sahn_config_t* config);
int sahn_cleanup();

int sahn_send(uint16_t destination, void* data, uint32_t data_size);
int sahn_recv(uint16_t* source, void* buffer, uint32_t buffer_size);
