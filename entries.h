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

#ifndef __ENTRIES_H
#define __ENTRIES_H

typedef struct Entry {

	char *title;
	char *user;
	char *pwd;
	char *url;
	char *notes;

} Entry_t;

Entry_t *create_new_entry(const char *title, const char *user,
			const char *pass, const char *url, const char *notes);
void entry_free(Entry_t *entry);

bool entry_add(sqlite3 *db, Entry_t *entry);
bool entry_remove(sqlite3 *db, int id);
Entry_t *entry_find(const char *searchterm);
void entry_display_by_title(sqlite3 *db, const char *title);
void entry_display_pwd_by_title(sqlite3 *db, const char *title);
void entry_display_by_id(sqlite3 *db, int id);
void entry_display_pwd_by_id(sqlite3 *db, int id);
Entry_t entry_get_by_id(sqlite3 *db, int id);
const char * entry_get_pwd_by_id(sqlite3 *db, int id);


#endif
