#include <sahn/net.h>
#include <sahn/topo.h>
#include <sahn/udp.h>

#define MAX_PAYLOAD 120

struct net_packet {
  uint16_t source;
  uint16_t destination;
  uint16_t prev_hop;
  uint8_t seq;
  uint8_t ttl;
  uint8_t payload[MAX_PAYLOAD];
} __attribute__((__packed__));

int net_init(){

}

int net_cleanup(){

}

int net_send(uint16_t destination, uint8_t* data, uint32_t data_size){

}

int net_recv(uint16_t* source, uint8_t* buffer, uint32_t buffer_size){

}
