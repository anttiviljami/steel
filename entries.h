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
        int id;

	struct Entry *next;

} Entry_t;

Entry_t *list_create(const char *title, const char *user,
			const char *pass, const char *url, const char *notes,
                        int id, Entry_t *next);
Entry_t *list_add(Entry_t *list, const char *title, const char *user,
			const char *pass, const char *url, const char *notes,
                        int id);

Entry_t *list_search_by_title(Entry_t *list, const char *title);
Entry_t *list_search_by_id(Entry_t *list, int id);

Entry_t *list_delete_by_id(Entry_t *list, int id);

Entry_t *list_remove(Entry_t *list, Entry_t *nd);
void list_free(Entry_t *list);
void list_print(Entry_t *list);





#endif
