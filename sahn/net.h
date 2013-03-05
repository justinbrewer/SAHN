#pragma once

#include <stdint.h>

int net_init();
int net_cleanup();

int net_send(uint16_t destination, uint8_t* data, uint32_t data_size);
int net_recv(uint16_t* source, uint8_t* buffer, uint32_t buffer_size);
