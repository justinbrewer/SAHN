#pragma once

#include "sahn.h"

#include <stdint.h>

int net_init(struct sahn_config_t* config);
int net_cleanup();

int net_send(uint16_t destination, void* data, uint32_t data_size);
int net_recv(uint16_t* source, void* buffer, uint32_t buffer_size);
