/* This file is part of libsahn
 * Copyright (c) 2013 Justin Brewer
 *
 * libsahn is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/set.h"

#include <stdlib.h>
#include <string.h>

int* set_union(int* a, uint32_t a_size, int* b, uint32_t b_size, uint32_t* c_size){
  int* c;
  int i=0,j=0,k;

  if(a_size == 0){
    if(b_size == 0){
      (*c_size) = 0;
      return NULL;
    } else {
      (*c_size) = b_size;
      c = (int*)malloc(b_size*sizeof(int));
      memcpy(c,b,b_size*sizeof(int));
      return c;
    }
  } else {
    if(b_size == 0){
      (*c_size) = a_size;
      c = (int*)malloc(a_size*sizeof(int));
      memcpy(c,a,a_size*sizeof(int));
      return c;
    }
  }

  (*c_size) = set_union_size(a,a_size,b,b_size);
  c = (int*)malloc((*c_size)*sizeof(int));

  for(k=0;k<(*c_size);k++){
    if(a[i] < b[j]){
      c[k] = a[i++];
      continue;
    }

    if(a[i] > b[j]){
      c[k] = b[j++];
      continue;
    }

    c[k] = a[i++];
    j++;
  }

  return c;
}

uint32_t set_union_size(int* a, uint32_t a_size, int* b, uint32_t b_size){
  if(a_size == 0){
    if(b_size == 0){
      return 0;
    } else {
      return b_size;
    }
  } else {
    if(b_size == 0){
      return a_size;
    }
  }

  return a_size + b_size - set_intersect_size(a,a_size,b,b_size);
}

uint32_t set_intersect_size(int* a, uint32_t a_size, int* b, uint32_t b_size){
  uint32_t c=0,i,j;

  if(a_size == 0 || b_size == 0){
    return 0;
  }

  //TODO: This algorithm sucks
  for(i=0,j=0;i<a_size;i++){
    for(j=0;j<b_size;j++){
      if(a[i] == b[j]){
	c++;
	break;
      }
    }
  }

  return c;
}
