#pragma once

#include <stdint.h>

#define SAHN_MAX_DATA_SIZE 116

int sahn_init(const char* topology_file, uint16_t node_address);
int sahn_cleanup();

int sahn_update_topology(const char* new_file);

int sahn_send(uint16_t destination, void* data, uint32_t data_size);
int sahn_recv(uint16_t* source, void* buffer, uint32_t buffer_size);
