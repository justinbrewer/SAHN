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

#include <stdint.h>

int* set_union(int* a, uint32_t a_size, int* b, uint32_t b_size, uint32_t* c_size);

uint32_t set_union_size(int* a, uint32_t a_size, int* b, uint32_t b_size);
uint32_t set_intersect_size(int* a, uint32_t a_size, int* b, uint32_t b_size);
