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

int set__compare(const void* a, const void* b){
  return *((const int*)a) - *((const int*)b);
}

struct set_t* set_create(){
  struct set_t* set = (struct set_t*)malloc(sizeof(struct set_t));
  set->num = 0;
  set->cap = 16;
  set->values = (int*)malloc(set->cap*sizeof(int));
  return set;
}

struct set_t* set_wrap(int* values, size_t num){
  struct set_t* set = (struct set_t*)malloc(sizeof(struct set_t));
  set->values = values;
  set->num = num;
  set->cap = num;
  return set;
}

int set_destroy(struct set_t* set){
  if(set->values != NULL){
    free(set->values);
  }
  free(set);
  return 0;
}

int set_add(struct set_t* set, int value){
  int* pos = bsearch(&value,set->values,set->num,sizeof(int),set__compare);

  if(pos == NULL){
    if(set->cap == set->num){
      set->cap = set->cap*2;
      set->values = (int*)realloc(set->values,set->cap*sizeof(int));
    }
    set->values[set->num++] = value;
    qsort(set->values,set->num,sizeof(int),set__compare);
    return 1;
  }

  return 0;
}

int set_remove(struct set_t* set, int value){
  int index;
  int* pos = bsearch(&value,set->values,set->num,sizeof(int),set__compare);

  if(pos != NULL){
    index = pos - set->values;
    if(index == set->num-1){
      *pos = 0;
      set->num--;
    } else {
      memmove(pos,pos+1,(set->num - index)*sizeof(int));
    }
    return 1;
  }

  return 0;
}

struct set_t* set_union(struct set_t* a, struct set_t* b){
  int* values;
  size_t num;

  values = set_union__raw(a->values,a->num,b->values,b->num,&num);

  return set_wrap(values,num);
}

size_t set_union_size(struct set_t* a, struct set_t* b){
  return set_union_size__raw(a->values,a->num,b->values,b->num);
}

size_t set_intersect_size(struct set_t* a, struct set_t* b){
  return set_intersect_size__raw(a->values,a->num,b->values,b->num);
}

int* set_union__raw(int* a, size_t a_size, int* b, size_t b_size, size_t* c_size){
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

  (*c_size) = set_union_size__raw(a,a_size,b,b_size);
  c = (int*)malloc((*c_size)*sizeof(int));

  for(k=0;k<(*c_size);k++){
    if(j == b_size || a[i] < b[j]){
      c[k] = a[i++];
      continue;
    }

    if(i == a_size || a[i] > b[j]){
      c[k] = b[j++];
      continue;
    }

    c[k] = a[i++];
    j++;
  }

  return c;
}

size_t set_union_size__raw(int* a, size_t a_size, int* b, size_t b_size){
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

  return a_size + b_size - set_intersect_size__raw(a,a_size,b,b_size);
}

size_t set_intersect_size__raw(int* a, size_t a_size, int* b, size_t b_size){
  size_t c=0,i,j;

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
