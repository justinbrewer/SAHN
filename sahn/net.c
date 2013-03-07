#include <sahn/net.h>
#include <sahn/topo.h>
#include <sahn/udp.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PAYLOAD 116
#define BUFFER_LEN 128
#define HEADER_SIZE 12

struct net_packet {
  uint16_t source;
  uint16_t destination;
  uint16_t prev_hop;
  uint16_t seq;
  uint8_t size;
  uint8_t __header_padding[3];
  uint8_t payload[MAX_PAYLOAD];
} __attribute__((__packed__));

struct net_seq_entry {
  uint16_t addr;
  uint16_t seq;
};

uint16_t local_address;
uint16_t num_links;
uint16_t* links;

unsigned int seq_cap;
unsigned int seq_num;
struct net_seq_entry* seq_table;

volatile unsigned int net_recv_front;
volatile unsigned int net_recv_back;
struct net_packet net_recv_buffer[BUFFER_LEN];

pthread_mutex_t net_recv_lock;
pthread_cond_t net_recv_avail;

pthread_t net_run_thread;

int net__seq_compare(const void* a, const void* b){
  return ((const struct net_seq_entry*)a)->addr - ((const struct net_seq_entry*)b)->addr;
}

int net__seq_check(uint16_t addr, uint16_t seq){
  struct net_seq_entry key = {.addr = addr}, *match;

  if(seq_num == 0){
    seq_table[0].addr = addr;
    seq_table[0].seq = seq;
    seq_num++;
    return 1;
  }

  match = bsearch(&key,seq_table,seq_num,sizeof(struct net_seq_entry),net__seq_compare);

  if(match == NULL){
    if(seq_num == seq_cap){
      seq_cap <<= 1;
      seq_table = (struct net_seq_entry*)realloc(seq_table,seq_cap*sizeof(struct net_seq_entry));
    }

    seq_table[seq_num].addr = addr;
    seq_table[seq_num].seq = seq;
    seq_num++;
    qsort(seq_table,seq_num,sizeof(struct net_seq_entry),net__seq_compare);
    return 1;
  }

  if(match->seq < seq){
    match->seq = seq;
    return 1;
  }

  return 0;
}

int net__dispatch_packet(struct net_packet* packet){
  int i;
  uint16_t prev_hop = packet->prev_hop;
  packet->prev_hop = local_address;
  
  for(i=0;i<num_links;i++){
    if(links[i] != prev_hop && links[i] != packet->source){
      udp_send(links[i],packet,packet->size);
    }
  }
}

void* net__run(void* params){
  struct net_packet packet = {0};

  while(1){
    udp_recv(NULL,&packet,sizeof(struct net_packet));

    if(!net__seq_check(packet.source,packet.seq)){
      continue;
    }

    if(packet.destination == local_address){
      memcpy(&net_recv_buffer[net_recv_back++],&packet,sizeof(struct net_packet));
      net_recv_back %= BUFFER_LEN;
      
      //TODO: We might need to lock first
      pthread_cond_signal(&net_recv_avail);
      
      continue;
    }

    net__dispatch_packet(&packet);
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

  seq_cap = topo_get_num_nodes();
  seq_table = (struct net_seq_entry*)malloc(seq_cap*sizeof(struct net_seq_entry));
  memset(seq_table,0,seq_cap*sizeof(struct net_seq_entry));
  seq_num = 0;

  pthread_mutex_init(&net_recv_lock,NULL);
  pthread_cond_init(&net_recv_avail,NULL);

  pthread_create(&net_run_thread,NULL,net__run,NULL);
}

int net_cleanup(){
  pthread_cancel(net_run_thread);

  free(seq_table);
  free(links);
  pthread_mutex_destroy(&net_recv_lock);
  pthread_cond_destroy(&net_recv_avail);
}

int net_send(uint16_t destination, void* data, uint32_t data_size){
  static uint16_t seq = 0;
  struct net_packet packet = {0};

  packet.source = local_address;
  packet.destination = destination;
  packet.seq = seq++;

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
  net_recv_front %= BUFFER_LEN;

  pthread_mutex_unlock(&net_recv_lock);

  if(source != NULL){
    *source = packet->source;
  }

  //TODO: What should we do if the buffer is smaller than the packet?
  size = packet->size - HEADER_SIZE;

  memcpy(buffer,packet->payload,size);

  return size;
}
