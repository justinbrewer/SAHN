#pragma once

#include <stdint.h>

int net_init();
int net_cleanup();

int net_update();

int net_send(uint16_t destination, void* data, uint32_t data_size);
int net_recv(uint16_t* source, void* buffer, uint32_t buffer_size);
