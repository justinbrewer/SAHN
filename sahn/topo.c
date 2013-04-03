#include "topo.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NODES 128
#define MAX_LINKS 32

#define DROP_RANGE 128

char* topo_file;

struct topo_node* topo_nodes;
unsigned int topo_num_nodes;

struct topo_node* topo_local_node;

int topo__node_compare(const void* a, const void* b){
  return (int)((struct topo_node*)a)->address - (int)((struct topo_node*)b)->address;
}

struct topo_node* topo__get_node(uint16_t address){
  struct topo_node key = {.address = address};
  return bsearch(&key,topo_nodes,topo_num_nodes,sizeof(struct topo_node),topo__node_compare);
}

int topo_init(const char* file, uint16_t local_address){
  int i=0;
  char addr_buf[64], port_buf[8], links_buf[64], *link;
  FILE* fp;

  topo_nodes = (struct topo_node*)malloc(MAX_NODES*sizeof(struct topo_node));
  memset(topo_nodes, 0, MAX_NODES*sizeof(struct topo_node));
  topo_num_nodes = 0;

  topo_file = strdup(file);
  fp = fopen(topo_file,"r");

  while(!feof(fp)){
    fscanf(fp,"Node %hu %64[^,], %8s %hd %hd links %64[^\n]\n",
	   &topo_nodes[i].address,
	   addr_buf,
	   port_buf,
	   &topo_nodes[i].loc.x,
	   &topo_nodes[i].loc.y,
	   links_buf);

    topo_nodes[i].real_address = strdup(addr_buf);
    topo_nodes[i].real_port = strdup(port_buf);
    
    topo_nodes[i].links = (uint16_t*)malloc(MAX_LINKS*sizeof(uint16_t));
    link = strtok(links_buf," ");
    while(link != NULL){
      topo_nodes[i].links[topo_nodes[i].num_links++] = atoi(link);
      link = strtok(NULL," ");
    }
    topo_nodes[i].links = (uint16_t*)realloc(topo_nodes[i].links,topo_nodes[i].num_links*sizeof(uint16_t));

    i++;
  }
  topo_num_nodes = i;
  fclose(fp);

  qsort(topo_nodes,topo_num_nodes,sizeof(struct topo_node),topo__node_compare);

  topo_local_node = topo__get_node(local_address);

  return 0;
}

int topo_cleanup(){
  int i;

  free(topo_file);

  for(i=0;i<topo_num_nodes;i++){
    free(topo_nodes[i].links);
    free(topo_nodes[i].real_address);
    free(topo_nodes[i].real_port);
  }

  free(topo_nodes);

  return 0;
}

struct topo_node* topo_get_local_node(){
  return topo_copy_node(topo_local_node);
}

struct topo_node* topo_get_node(uint16_t address){
  return topo_copy_node(topo__get_node(address));
}

unsigned int topo_get_num_nodes(){
  return topo_num_nodes;
}

uint32_t topo_drop_rate(uint16_t remote_node){
  int x, y;
  const struct topo_coord* a = &topo_local_node->loc;
  const struct topo_coord* b = &topo__get_node(remote_node)->loc;

  x = a->x - b->x;
  x *= x;
  y = a->y - b->y;
  y *= y;
  x += y; 
  x *= x;

  return x >> 20; //TODO: This makes drop range unadjustable (must be 128)
}

struct topo_node* topo_alloc_node(){
  struct topo_node* node = (struct topo_node*)malloc(sizeof(struct topo_node));
  memset(node,0,sizeof(struct topo_node));
  return node;
}

struct topo_node* topo_copy_node(struct topo_node* node){
  if(node == NULL){
    return NULL;
  }

  int i;
  struct topo_node* res = topo_alloc_node();
  memcpy(res,node,sizeof(struct topo_node));

  res->links = (uint16_t*)malloc(res->num_links*sizeof(uint16_t));
  memcpy(res->links,node->links,res->num_links*sizeof(uint16_t));

  res->real_address = strdup(node->real_address);
  res->real_port = strdup(node->real_port);
  
  return res;
}

int topo_free_node(struct topo_node* node){
  if(node->links != NULL){
    free(node->links);
  }
  if(node->real_address != NULL){
    free(node->real_address);
  }
  if(node->real_port != NULL){
    free(node->real_port);
  }
  free(node);
  return 0;
}
