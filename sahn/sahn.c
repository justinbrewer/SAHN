#include "net.h"
#include "sahn.h"
#include "topo.h"
#include "udp.h"

#define SAHN_MAX_PACKET_SIZE 128

#define SAHN_EXPORT __attribute__((visibility("default")))

SAHN_EXPORT
int sahn_init(const char* topology_file, uint16_t node_address) {
  topo_init(topology_file,node_address);
  udp_init();
  net_init();
}

SAHN_EXPORT
int sahn_cleanup() {
  net_cleanup();
  udp_cleanup();
  topo_cleanup();
}

SAHN_EXPORT
int sahn_update_topology(const char* new_file) {

}

SAHN_EXPORT
int sahn_send(uint16_t destination, void* data, uint32_t data_size) {
  return net_send(destination,data,data_size);
}

SAHN_EXPORT
int sahn_recv(uint16_t* source, void* buffer, uint32_t buffer_size) {
  return net_recv(source,buffer,buffer_size);
}
