#include <sahn/topo.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NODES 128

char* topo_file;

struct topo_node* topo_nodes;
unsigned int topo_num_nodes;

uint16_t topo_local_address;

int topo__node_compare(const void* a, const void* b){
  return ((struct topo_node*)a)->address - ((struct topo_node*)b)->address;
}

int topo_init(const char* file, uint16_t local_address){
  int i=0;
  char addr_buf[64], links_buf[64];
  FILE* fp;

  topo_local_address = local_address;

  topo_nodes = (struct topo_node*)malloc(MAX_NODES*sizeof(struct topo_node));
  memset(topo_nodes, 0, MAX_NODES*sizeof(struct topo_node));
  topo_num_nodes = 0;

  topo_file = strdup(file);
  fp = fopen(topo_file,"r");

  while(!feof(fp)){
    fscanf(fp,"Node %d %s, %d %d %d links %s\n",
	   &topo_nodes[i].address,
	   &addr_buf,
	   &topo_nodes[i].real_port,
	   &topo_nodes[i].loc.x,
	   &topo_nodes[i].loc.y,
	   &links_buf);

    topo_nodes[i].real_address = strdup(addr_buf);
    
    //TODO process links

    i++;
  }
  topo_num_nodes = i;
  fclose(fp);

  qsort(topo_nodes,topo_num_nodes,sizeof(struct topo_node),topo__node_compare);

  return 0;
}

int topo_cleanup(){
  int i;

  free(topo_file);

  for(i=0;i<topo_num_nodes;i++){
    free(topo_nodes[i].links);
    free(topo_nodes[i].real_address);
  }

  return 0;
}

struct topo_node* topo_get_local_node(){
  return topo_get_node(topo_local_address);
}

struct topo_node* topo_get_node(uint16_t address){
  struct topo_node key = {.address = address}, *match;
  match = bsearch(&key,topo_nodes,topo_num_nodes,sizeof(struct topo_node),topo__node_compare);
  return topo_copy_node(match);
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
  
  return res;
}

int topo_free_node(struct topo_node* node){
  if(node->links != NULL){
    free(node->links);
  }
  if(node->real_address != NULL){
    free(node->real_address);
  }
  free(node);
  return 0;
}
