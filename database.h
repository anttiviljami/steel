/*
 * Copyright (C) 2015 Niko Rosvall <niko@byteptr.com>
 *
 * This file is part of Steel.
 *
 * Steel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Steel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Steel.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __DATABASE_H
#define __DATABASE_H

#include "entries.h"

bool db_init(const char *path, const char *passphrase);
bool db_open(const char *path, const char *passphrase);
void db_close(const char *passphrase);

int db_get_next_id();
bool db_add_entry(Entry_t *entry);
Entry_t *db_get_all_entries();
Entry_t *db_get_entry_by_id(int id);
bool db_delete_entry_by_id(int id, bool *success);

#endif
