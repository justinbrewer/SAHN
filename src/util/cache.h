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

#include <string.h>
#include <stdint.h>

struct cache_t;
typedef void (*cache_free_t)(void*);

struct cache_t* cache_create(cache_free_t free_callback);
int cache_destroy(struct cache_t* cache);

int cache_enable_sort(struct cache_t* cache);
int cache_disable_sort(struct cache_t* cache);

void* cache_get(struct cache_t* cache, uint32_t key);
int cache_set(struct cache_t* cache, uint32_t key, void* val);
uint32_t cache_len(struct cache_t* cache);

int cache_lock(struct cache_t* cache);
int cache_unlock(struct cache_t* cache);
int cache_flush(struct cache_t* cache);
