#include "sahn.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("Usage: test1 <node-address>\n");
    exit(1);
  }

  uint16_t address = atoi(argv[1]), from;
  char* message = "Hello, world!";
  char buffer[64];

  sahn_init("examples/test2/nodes",address,NULL);

  if(address == 42){
    sahn_send(19,message,strlen(message));
    sahn_recv(&from,buffer,64);
    printf("Message from %d: %s\n",from,buffer);
  } else {
    sahn_recv(&from,buffer,64);
    printf("Message from %d: %s\n",from,buffer);
    sahn_send(42,message,strlen(message));    
  }

  sahn_cleanup();
  return 0;
}
