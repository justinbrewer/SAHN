#pragma once

#include <stdint.h>

int seq_init(unsigned int size);
int seq_cleanup();

int seq_check(uint16_t addr, uint16_t seq);
