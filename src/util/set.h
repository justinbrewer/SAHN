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

#pragma once

#include <stddef.h>
#include <stdint.h>

struct set_t {
  int* values;
  size_t num;
  size_t cap;
};

struct set_t* set_create();
struct set_t* set_wrap(int* values, size_t num);
int set_destroy(struct set_t* set);

int set_add(struct set_t* set, int value);
int set_remove(struct set_t* set, int value);

struct set_t* set_union(struct set_t* a, struct set_t* b);
size_t set_union_size(struct set_t* a, struct set_t* b);
size_t set_intersect_size(struct set_t* a, struct set_t* b);

int* set_union__raw(int* a, size_t a_size, int* b, size_t b_size, size_t* c_size);
size_t set_union_size__raw(int* a, size_t a_size, int* b, size_t b_size);
size_t set_intersect_size__raw(int* a, size_t a_size, int* b, size_t b_size);
