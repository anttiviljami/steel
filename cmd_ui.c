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
#include <string.h>
#include "database.h"
#include "cmd_ui.h"


bool add_new_entry(const char *title, const char *user, const char *pass,
		   const char *url, const char *note)
{
	int id = db_get_next_id();
		
	Entry_t *entry = list_create(title, user, pass, url, note, id, NULL);

	if(!db_add_entry(entry)) {
		fprintf(stderr, "Failed to add a new entry.\n");
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

void show_one_entry(int id)
{
	Entry_t *entry = db_get_entry_by_id(id);
	Entry_t *head = entry;
	Entry_t *next;
	
	if(head != NULL) {
		//Skip the first one, it only has our initialization data.
		next = head->next;
		
		if(next != NULL) {
			printf("%s\t%s\t%s\t%s\n",next->title, next->user,
			       next->pwd, next->url);
		}
		else {
			printf("No entry found with id %d.\n", id);
		}
	}
	
	list_free(entry);
}

void delete_entry(int id)
{
	bool success = false;
	
	if(!db_delete_entry_by_id(id, &success)) {
		fprintf(stderr, "Entry deletion failed.\n");
	}
	else {
		if(!success)
			fprintf(stderr, "No entry found with id %d.\n", id);
	}
}

void find_entries(const char *search)
{
	Entry_t *list = db_get_all_entries();
	Entry_t *new_head = list->next;
	
	while(new_head != NULL) {
				
		if(strstr(new_head->title, search) != NULL || 
			strstr(new_head->user, search) != NULL ||
			strstr(new_head->url, search) != NULL ||
			strstr(new_head->notes, search) != NULL) {
			
			/*printf("%s\t%s\t%s\t%s\t%d\n",new_head->title, 
				new_head->user, new_head->pwd, 
				new_head->url, new_head->id);*/
			
			list_print_one(new_head);
		}
				
		new_head = new_head->next;
	}
	
	list_free(list);
}