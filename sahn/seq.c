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

#include "seq.h"

#include <stdlib.h>
#include <string.h>

struct seq_entry {
  uint16_t addr;
  uint16_t seq;
};

unsigned int seq_cap;
unsigned int seq_num;

struct seq_entry* seq_table;

uint16_t seq_thr_high;
uint16_t seq_thr_low;

int seq__compare(const void* a, const void* b){
  return ((const struct seq_entry*)a)->addr - ((const struct seq_entry*)b)->addr;
}

int seq_init(unsigned int size, struct sahn_config_t* config){
  seq_cap = size;
  seq_num = 0;

  seq_table = (struct seq_entry*)malloc(seq_cap*sizeof(struct seq_entry));
  memset(seq_table,0,seq_cap*sizeof(struct seq_entry));

  seq_thr_high = config->seq_threshold_high;
  seq_thr_low = config->seq_threshold_low;

  return 0;
}

int seq_cleanup(){
  free(seq_table);
  return 0;
}

int seq_check(uint16_t addr, uint16_t seq){
  struct seq_entry key = {.addr = addr}, *match;

  if(seq_num == 0){
    seq_table[0].addr = addr;
    seq_table[0].seq = seq;
    seq_num++;
    return 1;
  }

  match = bsearch(&key,seq_table,seq_num,sizeof(struct seq_entry),seq__compare);

  if(match == NULL){
    if(seq_num == seq_cap){
      seq_cap <<= 1;
      seq_table = (struct seq_entry*)realloc(seq_table,seq_cap*sizeof(struct seq_entry));
    }

    seq_table[seq_num].addr = addr;
    seq_table[seq_num].seq = seq;
    seq_num++;
    qsort(seq_table,seq_num,sizeof(struct seq_entry),seq__compare);
    return 1;
  }

  if(match->seq < seq || (match->seq > seq_thr_high && seq < seq_thr_low)){
    match->seq = seq;
    return 1;
  }

  return 0;
}
