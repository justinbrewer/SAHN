#include <sahn/topo.h>

#include <stdlib.h>
#include <string.h>

char* topology_file;

int topo_init(const char* file, uint16_t local_node){
  topology_file = strdup(file);
}

int topo_cleanup(){
  free(topology_file);
}

int topo_get_local_node(struct topo_node* res){

}

int topo_get_node(uint16_t address, struct topo_node* res){

}

