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

  sahn_init("examples/test3/nodes",address);

  switch(address){
  case 19:
    sahn_recv(&from,buffer,64);
    printf("Message from %d: %s\n",from,buffer);
    sahn_send(42,message,strlen(message)+1);
    sahn_send(23,message,strlen(message)+1);
    break;

  case 23:
    sahn_recv(&from,buffer,64);
    printf("Message from %d: %s\n",from,buffer);
    break;

  case 42:
    sahn_send(19,message,strlen(message)+1);
    sahn_recv(&from,buffer,64);
    printf("Message from %d: %s\n",from,buffer);
    break;
  }

  sahn_cleanup();
  return 0;
}
