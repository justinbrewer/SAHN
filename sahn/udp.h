#pragma once

#include <stdint.h>

int udp_init();
int udp_cleanup();

int udp_send(uint16_t destination, void* data, uint32_t data_size);
int udp_recv(uint16_t* source, void* buffer, uint32_t buffer_size);
