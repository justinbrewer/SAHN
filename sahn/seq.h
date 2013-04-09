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

#include "sahn.h"

#include <stdint.h>

int seq_init(unsigned int size, struct sahn_config_t* config);
int seq_cleanup();

int seq_check(uint16_t addr, uint16_t seq);
