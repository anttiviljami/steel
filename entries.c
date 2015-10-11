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

void list_print(Entry_t *list)
{
	//Take copies of the head pointer.
	Entry_t *tmp = list;
	Entry_t *tmp2 = list;
	
	int len;
	
	//Calculate the longest string in the list.
	//It's used to align the output.
	while(tmp != NULL) {

		len = strlen(tmp->title);
		
		if(len < strlen(tmp->user))
			len = strlen(tmp->user);
		if(len < strlen(tmp->pwd))
			len = strlen(tmp->pwd);
		if(len < strlen(tmp->url))
			len = strlen(tmp->url);
		
		tmp = tmp->next;
	}
	
	while(tmp2 != NULL) {
		
		//If id is -1 it's our information column with column names
		//Print them. Otherwise print the actual id number instead of
		//the column name.
		if(tmp2->id == -1) {
			printf("%s\t%-*s %-*s %-*s\t%-*s\n",
			tmp2->title,len, tmp2->user,len, tmp2->pwd, len, tmp2->url, 
			len, tmp2->notes);
		}
		else {
			printf("%s\t%-*s %-*s %-*s\t%-*d\n",
			tmp2->title,len, tmp2->user,len, tmp2->pwd, len, tmp2->url, 
			len, tmp2->id);
		}
		
		tmp2 = tmp2->next;
	}
}

//Method calculates longest string from
//current list cursor and returns it.
//If the cursor is null, -1 is returned.
static int list_calculate_longest_str_cursor(Entry_t *cursor)
{
	int len;
	
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

void list_print_one(Entry_t *cursor)
{
	int len = list_calculate_longest_str_cursor(cursor);
	
	if(len == -1)
		return;
	
	
	printf("%s\t%-*s %-*s %-*s\t%-*d\n", cursor->title, len, cursor->user,len, 
	       cursor->pwd, len, cursor->url, len, cursor->id);
}
