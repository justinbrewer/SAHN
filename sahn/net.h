#pragma once

#include <stdint.h>

#define NET_MAX_DATA_SIZE 116

int net_init();

int net_load_file(const char* file);
int net_watch_file(const char* file);

int net_cleanup();

int net_send(uint16_t destination, uint8_t* data, uint32_t data_size);
int net_recv(uint16_t& source, uint8_t* buffer, uint32_t buffer_size);
