#pragma once

#include "sahn.h"

#include <stdint.h>

int seq_init(unsigned int size, struct sahn_config_t* config);
int seq_cleanup();

int seq_check(uint16_t addr, uint16_t seq);
