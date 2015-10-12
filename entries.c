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
#include <string.h>
#include "entries.h"

//This implements a simple, single linked list.
//Not all of the functions are used in command line
//version of Steel, but they are implemented for
//later implementation of gui client.

//Create and return new list.
Entry_t *list_create(const char *title, const char *user,
			const char *pass, const char *url, const char *notes,
                        int id, Entry_t *next)
{
	Entry_t *list = NULL;

	list = malloc(sizeof(Entry_t));
	
	if(list == NULL) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}
	
	list->title = strdup(title);
	list->user = strdup(user);
	list->pwd = strdup(pass);
	list->url = strdup(url);
	list->notes = strdup(notes);
	list->id = id;
	
        list->next = next;

	return list;
}

//Add an entry to the list. If the list is NULL, new list is created.
//Returns the list with an added entry.
Entry_t *list_add(Entry_t *list, const char *title, const char *user,
			const char *pass, const char *url, const char *notes,
                        int id)
{
	if(list == NULL)
		return list_create(title, user, pass, url, notes, id, NULL);
	
	Entry_t *cursor = list;
	
	while(cursor->next != NULL)
		cursor = cursor->next;
		
	Entry_t *newlist = list_create(title, user, pass, url, notes, id, NULL);
	cursor->next = newlist;
        
	return list;
}

//Search and return entry with wanted title. Returns NULL if title
//was not found.
Entry_t *list_search_by_title(Entry_t *list, const char *title)
{
	Entry_t *cursor = list;
	
	while(cursor != NULL) {
        
		if(strcmp(cursor->title, title) == 0)
			return cursor;
		
		cursor = cursor->next;
	}

	return NULL;
}

//Returns Entry_t that has matching id.
//If the list does not contain an entry with wanted
//id, return NULL
Entry_t *list_search_by_id(Entry_t *list, int id)
{
	Entry_t *cursor = list;
	
	while(cursor != NULL) {
        
		if(cursor->id == id)
			return cursor;
		
		cursor = cursor->next;
	}

	return NULL;
}

//Delete entry from list which has the wanted id.
//Returns a list without the entry, or NULL if id
//was not found.
Entry_t *list_delete_by_id(Entry_t *list, int id)
{
	Entry_t *del = NULL;
	
	del = list_search_by_id(list, id);
	
	if(del == NULL) {
		return NULL;
	}
	
	list_remove(list, del);
	
	return list;
}

//Remove first entry from the list.
static Entry_t* remove_front(Entry_t* list)
{
	if(list == NULL)
		return NULL;
	
	Entry_t *front = list;
	list = list->next;
	front->next = NULL;
	
	/* is this the last node in the list */
	if(front == list)
		list = NULL;
	
	free(front);
	
	return list;
}

//Remove last entry from the list.
static Entry_t *remove_back(Entry_t* list)
{
	if(list == NULL)
		return NULL;
	
	Entry_t *cursor = list;
	Entry_t *back = NULL;
	
	while(cursor->next != NULL)
	{
		back = cursor;
		cursor = cursor->next;
	}
	if(back != NULL)
		back->next = NULL;
	
	/* if this is the last node in the list*/
	if(cursor == list)
		list = NULL;
	
	free(cursor);
	
	return list;
}

//Remove Entry nd from list.
//Returns list without the element that was removed.
Entry_t *list_remove(Entry_t *list, Entry_t *nd)
{
	if(list == nd) {
		list = remove_front(list);
		return list;
	}
	
	if(nd->next == NULL) {
		list = remove_back(list);
		return list;
	}
	
	Entry_t *cursor = list;
	
	while(cursor != NULL) {
		
		if(cursor->next == nd)
			break;
		
		cursor = cursor->next;
	}
	
	if(cursor != NULL) {
		
		Entry_t *tmp = cursor->next;
		cursor->next = tmp->next;
		tmp->next = NULL;
		free(tmp);
	}
	
	return list;
}

void list_free(Entry_t *list)
{	
	Entry_t *cursor;
	
	while(list != NULL) {
		cursor = list;
		list = list->next;
		free(cursor->title);
		free(cursor->user);
		free(cursor->pwd);
		free(cursor->url);
		free(cursor->notes);
		free(cursor);
	}
}

//Method calculates longest string from
//current list cursor and returns it.
//If the cursor is null, -1 is returned.
static int list_calculate_longest_str_cursor(Entry_t *entry)
{
	int len;
	Entry_t *cursor = entry;
	
	if(cursor == NULL)
		return -1;
	
	len = strlen(cursor->title);
		
	if(len < strlen(cursor->user))
		len = strlen(cursor->user);
	if(len < strlen(cursor->pwd))
		len = strlen(cursor->pwd);
	if(len < strlen(cursor->url))
		len = strlen(cursor->url);
	
	return len;
}

//Calculate longest string in the list.
static int list_calculate_longest_str(Entry_t *list)
{
	int len = 0;
	int cursorlen = 0;
	
	Entry_t *cursor = list;
	
	while(cursor != NULL) {
		
		cursorlen = list_calculate_longest_str_cursor(cursor);
		
		if(len < cursorlen)
			len = cursorlen;
		
		cursor = cursor->next;
	}
	
	return len;
}

//Print whole list from the cursor pointed by list.
//Print is formatted with a nice output and should be easy to
//read.
void list_print(Entry_t *list)
{
	//Take copies of the head pointer.
	Entry_t *tmp = list->next;
	int len = list_calculate_longest_str(tmp) + 18;
	
	printf("\n");
	
	while(tmp != NULL) {
	
		printf("%s\t\t%d\n", "Id", tmp->id);
		printf("%s\t\t%s\n", "Title", tmp->title);
		printf("%s\t%s\n", "Username", tmp->user);
		printf("%s\t%s\n", "Passphrase", tmp->pwd);
		printf("%s\t\t%s\n", "Address", tmp->url);
		
		//Print separator line as long as the longest string
		//in the list.
		for(int i = 0; i < len; i++)
			printf("-");
		
		printf("\n");
		
		tmp = tmp->next;
	}
}

//Print current cursor.
void list_print_one(Entry_t *cursor)
{
	int len = list_calculate_longest_str_cursor(cursor);
	
	len += 18;
	
	if(len == -1)
		return;
	
	printf("\n");
	printf("%s\t\t%d\n", "Id", cursor->id);
	printf("%s\t\t%s\n", "Title", cursor->title);
	printf("%s\t%s\n", "Username", cursor->user);
	printf("%s\t%s\n", "Passphrase", cursor->pwd);
	printf("%s\t\t%s\n", "Address", cursor->url);
	
	//Print separator line as long as the longest string in the current
	//list cursor
	for(int i = 0; i < len; i++)
		printf("-");
	
	printf("\n");
}
