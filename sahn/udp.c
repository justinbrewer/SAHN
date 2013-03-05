#include <sahn/topo.h>
#include <sahn/udp.h>

#include <stdlib.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netdb.h>
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
  int size_sent;
  struct addrinfo hints = {0}, *addr;
  struct topo_node* node = topo_get_local_node(destination);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  
  getaddrinfo(node->real_address,node->real_port,&hints,&addr);

  size_sent = sendto(udp_socket,data,data_size,0,addr->ai_addr,addr->ai_addrlen);

  freeaddrinfo(addr);

  topo_free_node(node);
  return size_sent;
}

int udp_recv(uint16_t* source, uint8_t* buffer, uint32_t buffer_size){
  //TODO determine source simulated address
  int r = recvfrom(udp_socket,buffer,buffer_size,MSG_DONTWAIT,NULL,NULL);
  if(r & (EAGAIN|EWOULDBLOCK)){
      return UDP_WOULDBLOCK;
  }
  return r;
}
