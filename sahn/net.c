#include <sahn/net.h>
#include <sahn/topo.h>
#include <sahn/udp.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PAYLOAD 116
#define BUFFER_LEN 128
#define HEADER_SIZE 12
#define TTL_START 32

struct net_packet {
  uint16_t source;
  uint16_t destination;
  uint16_t prev_hop;
  uint8_t ttl;
  uint8_t size;
  uint16_t seq;
  uint16_t __header_padding;
  uint8_t payload[MAX_PAYLOAD];
} __attribute__((__packed__));

uint16_t local_address;
uint16_t num_links;
uint16_t* links;

volatile unsigned int net_recv_front;
volatile unsigned int net_recv_back;
struct net_packet net_recv_buffer[BUFFER_LEN];

pthread_mutex_t net_recv_lock;
pthread_cond_t net_recv_avail;

int net__dispatch_packet(struct net_packet* packet){
  int i;
  uint16_t prev_hop = packet->prev_hop;
  packet->prev_hop = local_address;
  
  for(i=0;i<num_links;i++){
    if(links[i] != prev_hop){
      udp_send(links[i],packet,packet->size);
    }
  }
}

int net_init(){
  net_recv_front = 0;
  net_recv_back = 0;
  memset(net_recv_buffer,0,BUFFER_LEN*sizeof(struct net_packet));

  struct topo_node* node = topo_get_local_node();
  
  local_address = node->address;
  num_links = node->num_links;

  links = node->links;
  node->links = NULL;

  topo_free_node(node);

  pthread_mutex_init(&net_recv_lock,NULL);
  pthread_cond_init(&net_recv_avail,NULL);
}

int net_cleanup(){
  free(links);
  pthread_mutex_destroy(&net_recv_lock);
  pthread_cond_destroy(&net_recv_avail);
}

int net_update(){
  int r;
  struct net_packet packet = {0};
  while((r = udp_recv(NULL,&packet,sizeof(struct net_packet))) != UDP_WOULDBLOCK){
    if(packet.destination = local_address){
      memcpy(&net_recv_buffer[net_recv_back++],&packet,sizeof(struct net_packet));
      net_recv_back %= BUFFER_LEN;
      
      //TODO: We might need to lock first
      pthread_cond_signal(&net_recv_avail);
      
      continue;
    }
    
    if(--packet.ttl == 0){
      continue;
    }

    net__dispatch_packet(&packet);
  }
}

int net_send(uint16_t destination, void* data, uint32_t data_size){
  static uint16_t seq = 0;
  struct net_packet packet = {0};

  packet.source = local_address;
  packet.destination = destination;
  packet.ttl = TTL_START;
  packet.seq = seq++; //TODO: Figure out sequencing

  if(data_size > MAX_PAYLOAD){
    //TODO: What to do with it? For now we'll just drop :P
    return -1;
  }

  packet.size = data_size + HEADER_SIZE;
  memcpy(&packet.payload,data,data_size);

  net__dispatch_packet(&packet);
  return data_size;
}

int net_recv(uint16_t* source, void* buffer, uint32_t buffer_size){
  int size;
  struct net_packet* packet;

  pthread_mutex_lock(&net_recv_lock);
  
  while(net_recv_front == net_recv_back){
    pthread_cond_wait(&net_recv_avail,&net_recv_lock);
  }

  packet = &net_recv_buffer[net_recv_front++];

  pthread_mutex_unlock(&net_recv_lock);

  if(source != NULL){
    *source = packet->source;
  }

  //TODO: What should we do if the buffer is smaller than the packet?
  size = packet->size - HEADER_SIZE;

  memcpy(buffer,packet->payload,size);

  return size;
}
