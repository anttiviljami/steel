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

//XOPEN_SOURCE is needed to get strdup function
#define _XOPEN_SOURCE 700 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sqlite3.h>

#include "entries.h"

Entry_t *create_new_entry(const char *title, const char *user,
			const char *pass, const char *url, const char *notes)
{
	Entry_t *entry;

	entry = malloc(sizeof(Entry_t));
	
	if(entry == NULL) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}

	entry->title = strdup(title);
	entry->user = strdup(user);
	entry->pwd = strdup(pass);
	entry->url = strdup(url);
	entry->notes = strdup(notes);

	return entry;
}

void entry_free(Entry_t *entry)
{
	free(entry->title);
	free(entry->user);
	free(entry->pwd);
	free(entry->url);
	free(entry->notes);

	free(entry);
}

bool entry_add(sqlite3 *db, Entry_t *entry)
{
	char *error = NULL;
	char *sql;
	int rc;

	sql =
	sqlite3_mprintf("insert into entries (title, user, passphrase, url, notes)" \
			"values('%q','%q','%q','%q','%q')", entry->title, entry->user, entry->pwd, entry->url,
			entry->notes);


	rc = sqlite3_exec(db, sql, NULL, 0, &error);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", error);
		sqlite3_free(error);
		free(sql);
		return false;
	}

	free(sql);
	sqlite3_free(error);
	
	return true;
}

bool entry_remove(sqlite3 *db, int id)
{
	char *error;
	int rc;
	char *sql;

	
	
	return false;
}

Entry_t *entry_find(const char *searchterm)
{

	return NULL;
}

void entry_display_by_title(sqlite3 *db, const char *title)
{


}

void entry_display_pwd_by_title(sqlite3 *db, const char *title)
{


}

void entry_display_by_id(sqlite3 *db, int id)
{


}

void entry_display_pwd_by_id(sqlite3 *db, int id)
{


}

Entry_t entry_get_by_id(sqlite3 *db, int id)
{
	Entry_t t;
	return t;

}

const char * entry_get_pwd_by_id(sqlite3 *db, int id)
{

	return NULL;
}
