#pragma once

#include <stdint.h>

typedef enum { UDP_WOULDBLOCK=-1 } udp_ret;

int udp_init();
int udp_cleanup();

int udp_send(uint16_t destination, uint8_t* data, uint32_t data_size);
int udp_recv(uint16_t* source, uint8_t* buffer, uint32_t buffer_size);
