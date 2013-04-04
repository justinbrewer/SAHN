#include "sahn.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
  if(argc != 3) {
    printf("Usage: test1 <topology-file> <node-address>\n");
    exit(1);
  }

  sahn_init(argv[1],atoi(argv[2]),NULL);

  sahn_cleanup();
  return 0;
}
