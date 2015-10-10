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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "database.h"
#include "cmd_ui.h"


bool add_new_entry(const char *title, const char *user, const char *pass,
		   const char *url, const char *note)
{
	int id = db_get_next_id();
	
	printf("ID = %d\n", id);
	
	Entry_t *entry = list_create(title, user, pass, url, note, id, NULL);

	if(!db_add_entry(entry)) {
		fprintf(stderr, "Failed to add new entry\n");
		return false;
	}
	
	list_free(entry);
	
	return true;
}

bool init_database(const char *path, const char *passphrase)
{
	return db_init(path, passphrase);	
}

bool open_database(const char *path, const char *passphrase)
{
	return db_open(path, passphrase);
}

void close_database(const char *passphrase)
{
	db_close(passphrase);
}

void show_all_entries()
{
	Entry_t *list = db_get_all_entries();
	list_print(list);
	
	list_free(list);
}