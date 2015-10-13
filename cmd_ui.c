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

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include "database.h"
#include "crypto.h"
#include "cmd_ui.h"

//cmd_ui.c implements simple interface for command line version
//of Steel. All functions in here are only used from main()

//This is called from main. Adds new entry to the database.
void add_new_entry(char *title, char *user, char *url, char *note)
{
	int id;
	//Should be enough...
	size_t pwdlen = 255;
	char pass[pwdlen];
	char *ptr = pass;
	char pass2[pwdlen];
	char *ptr2 = pass2;
	
	my_getpass(ENTRY_PWD_PROMPT, &ptr, &pwdlen, stdin);
	my_getpass(ENTRY_PWD_PROMPT_RETRY, &ptr2, &pwdlen, stdin);
	
	if(strcmp(pass, pass2) != 0) {
		fprintf(stderr, "Passphrases do not match.\n");
		return;
	}
	
	id = db_get_next_id();
		
	Entry_t *entry = list_create(title, user, pass, url, note, id, NULL);

	if(!db_add_entry(entry)) {
		fprintf(stderr, "Failed to add a new entry.\n");
		return;
	}
	
	list_free(entry);
}

//Initialize new database and encrypt it.
//Return false on failure, true on success.
//Path must be a path to a file that does not exists.
bool init_database(const char *path)
{
	size_t pwdlen = 255;
	char passphrase[pwdlen];
	char *ptr = passphrase;
	char pass2[pwdlen];
	char *ptr2 = pass2;
	
	my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
	my_getpass(MASTER_PWD_PROMPT_RETRY, &ptr2, &pwdlen, stdin);
	
	if(strcmp(passphrase, pass2) != 0) {
		fprintf(stderr, "Passphrases do not match.\n");
		return false;
	}
	
	if(!db_init(path, passphrase)) {
		fprintf(stderr, "Database initialization unsuccessful\n");
		return false;
	}
	
	return true;
}

//Decrypt database the database pointed by path.
//If decryption fails, function returns false.
bool open_database(const char *path)
{
	//Max passphrase length. Should be enough, really.
	size_t pwdlen = 255;
	char passphrase[pwdlen];
	char *ptr = passphrase;

	my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
	
	if(!db_open(path, passphrase)) {
		fprintf(stderr, "Database opening unsuccessful.\n");
		return false;
	}
		
	return true;
}

//Encrypt the database.
void close_database()
{
	size_t pwdlen = 255;
	char passphrase[pwdlen];
	char *ptr = passphrase;

	my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
	db_close(passphrase);
}

//Print all available entries to stdin.
//Database must not be encrypted.
void show_all_entries()
{
	Entry_t *list = db_get_all_entries();
	
	if(list != NULL) {
		list_print(list);
		list_free(list);
	}
}

//Print one entry by id to stdin, if found.
//Database must not be encrypted.
void show_one_entry(int id)
{
	Entry_t *entry = db_get_entry_by_id(id);
	Entry_t *head = entry;
	Entry_t *next;
	
	if(head != NULL) {
		//Skip the first one, it only has our initialization data.
		next = head->next;
		
		if(next != NULL) {
			list_print_one(next);
		}
		else {
			printf("No entry found with id %d.\n", id);
		}
	}
	
	list_free(entry);
}

//Delete entry by id from the database.
//Database must not be encrypted.
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

//Print all entries to stdin which has data matching with search.
//Database must not be encrypted.
void find_entries(const char *search)
{
	Entry_t *list = db_get_all_entries();
	Entry_t *new_head = list->next;
	
	while(new_head != NULL) {
		
		//Search for matching data	
		if(strstr(new_head->title, search) != NULL || 
			strstr(new_head->user, search) != NULL ||
			strstr(new_head->url, search) != NULL ||
			strstr(new_head->notes, search) != NULL) {
			
			list_print_one(new_head);
		}
				
		new_head = new_head->next;
	}
	
	list_free(list);
}

//Turns echo of from the terminal and asks for a passphrase.
//Usually stream is stdin. Returns length of the passphrase,
//passphrase is stored to lineptr. Lineptr must be allocated beforehand.
size_t my_getpass(char *prompt, char **lineptr, size_t *n, FILE *stream)
{
    struct termios old, new;
    int nread;

    //Turn terminal echoing off.
    if(tcgetattr(fileno(stream), &old) != 0)
        return -1;
    
    new = old;
    new.c_lflag &= ~ECHO;
    
    if(tcsetattr(fileno(stream), TCSAFLUSH, &new) != 0)
        return -1;

    if(prompt)
        printf("%s", prompt);

    //Read the password.
    nread = getline(lineptr, n, stream);

    if(nread >= 1 && (*lineptr)[nread - 1] == '\n')
    {
        (*lineptr)[nread - 1] = 0;
        nread--;
    }
    
    printf("\n");

    //Restore terminal echo.
    tcsetattr(fileno(stream), TCSAFLUSH, &old);

    return nread;
}

//Replace part of an entry pointed by id. "What" tells the function what to replace
//with the new data. What can be "passphrase", "user", "title", "url" or "notes".
//Database must not be encrypted.
void replace_part(int id, const char *what, const char *new_data)
{
	if(strcmp(what,"passphrase") != 0 && strcmp(what, "user") != 0
		&& strcmp(what, "title") != 0 && strcmp(what, "url") != 0
		&& strcmp(what, "notes") !=0) {
		
		fprintf(stderr, "Only title, user, passphrase, url or notes" \
		" can be replaced.\n");
	
		return;
	}
	
	Entry_t *entry = NULL;
	Entry_t *head = NULL;
	
	entry = db_get_entry_by_id(id);
	
	//Skip the initialization data
	head = entry->next;
	
	if(head == NULL) {
		fprintf(stderr, "No entry found with id %d.\n", id);
		return;
	}
	
	size_t pwdlen = 255;
	char pass[pwdlen];
	char *ptr = pass;
	char pass2[pwdlen];
	char *ptr2 = pass2;
	
	if(strcmp(what, "passphrase") == 0) {
		
		//Ok, user want's to replace passphrase.
		//Ask it, and verify it.
		my_getpass(ENTRY_PWD_PROMPT, &ptr, &pwdlen, stdin);
		my_getpass(ENTRY_PWD_PROMPT_RETRY, &ptr2, &pwdlen, stdin);
	
		if(strcmp(pass, pass2) != 0) {
			fprintf(stderr, "Passphrases do not match.\n");
			return;
		}
	}
	
	if(strcmp(what, "title") == 0) {
		free(head->title);
		head->title = strdup(new_data);
	}
	if(strcmp(what, "user") == 0) {
		free(head->user);
		head->user = strdup(new_data);
	}
	if(strcmp(what, "passphrase") == 0) {
		free(head->pwd);
		head->pwd = strdup(pass);
	}
	if(strcmp(what, "url") == 0) {
		free(head->url);
		head->url = strdup(new_data);
	}
	if(strcmp(what, "notes") == 0) {
		free(head->notes);
		head->notes = strdup(new_data);
	}
	
	db_update_entry(id, head);
	
	list_free(entry);
}

//Function generates new password and prints it to stdout.
//Does not use the database, so this function can be called
//even if the database is encrypted.
void generate_password(int count)
{
	if(count < 6) {
		fprintf(stderr, "Minimum length is 6 characters.\n");
		return;
	}
	
	char *pass = generate_pass(count);
	
	if(pass == NULL) {
		fprintf(stderr, "Generating new password failed.\n");
		return;
	}
	
	printf("%s\n", pass);
	
	free(pass);
}