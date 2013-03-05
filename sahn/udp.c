#include <sahn/topo.h>
#include <sahn/udp.h>

#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int udp_socket;

int udp_init(){
  struct sockaddr_in addr = {0};
  struct topo_node* local_node = topo_get_local_node();

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(atoi(local_node->real_port));

  udp_socket = socket(AF_INET,SOCK_DGRAM,0);
  bind(udp_socket,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));

  topo_free_node(local_node);
  return 0;
}

int udp_cleanup(){
  close(udp_socket);
}

int udp_send(uint16_t destination, uint8_t* data, uint32_t data_size){

}

int udp_recv(uint16_t* source, uint8_t* buffer, uint32_t buffer_size){

}
