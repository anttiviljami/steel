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
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include "database.h"
#include "crypto.h"
#include "cmd_ui.h"
#include "status.h"
#include "backup.h"

//cmd_ui.c implements simple interface for command line version
//of Steel. All functions in here are only called from main()

//Function works like strstr, but ignores the case.
//There's a function strcasestr, but it's nonstandard
//GNU extension, so let's not use that.
//Return value must be freed by the caller.
static char *my_strcasestr(const char *str, const char *str2)
{
	char *tmp1 = NULL;
	char *tmp2 = NULL;
	char *tmp3 = NULL;
	char *retval = NULL;

	tmp1 = strdup(str);

	if (tmp1 == NULL) {
		fprintf(stderr, "Strdup failed\n");
		return NULL;
	}

	tmp2 = strdup(str2);

	if (tmp2 == NULL) {
		free(tmp1);
		fprintf(stderr, "Strdup failed\n");
		return NULL;
	}

	for (int i = 0; i < strlen(tmp1); i++)
		tmp1[i] = tolower(tmp1[i]);

	for (int i = 0; i < strlen(tmp2); i++)
		tmp2[i] = tolower(tmp2[i]);

	tmp3 = strstr(tmp1, tmp2);

	if (tmp3 != NULL) {
		retval = strdup(tmp3);
		/* Sanity check
		 * Inform the user that something went wrong
		 * even the search term was found. Probably never happens.
		 */
		if (retval == NULL)
			fprintf(stderr,"Search term found, but strdup failed.\n");
	}

	free(tmp1);
	free(tmp2);

	return retval;
}

//Simple helper function to check if there's an open database
//available.
static bool open_db_exist(const char *message)
{
	char *old = NULL;
	old = read_path_from_lockfile();
	
	if(old != NULL) {
		
		if(db_file_exists(old)) {
			fprintf(stderr, "An open database exists. To improve security only one\n" \
			"passphrase database can be open at once.\n");
			fprintf(stderr, "Close %s first before %s another"\
			" database.\n", old, message);
			free(old);
			return true;
		}
		
		free(old);
	}
	
	return false;
}

//Simple helper function to check if the steel_dbs file used for
//tracking databases exists.
static bool steel_tracker_file_exists()
{
	char *dbs = NULL;

	dbs = status_get_file_path();

	if(dbs == NULL) {
		fprintf(stderr, "Error getting status file path.\n");
		return false;
	}
	
	//We can use db_file_exists function to check any file existence.
	//In the end, it's just simple check, not related to databases.
	if(!db_file_exists(dbs)) {
		fprintf(stdout, "No databases found.\n");
		free(dbs);
		return false;
	}
	
	free(dbs);
	
	return true;	
}

//Initialize new database and encrypt it.
//Return false on failure, true on success.
//Path must be a path to a file that does not exists.
bool init_database(const char *path)
{
	if(open_db_exist("creating"))
		return false;
	
	if(!db_init(path)) {
		fprintf(stderr, "Database initialization unsuccessful\n");
		return false;
	}
	
	status_set_tracking(path);
	
	return true;
}

//Decrypt database the database pointed by path.
//If decryption fails, function returns false.
bool open_database(const char *path)
{
	if(!steel_tracker_file_exists())
		return false;
	
	//Max passphrase length. Should be enough, really.
	size_t pwdlen = 255;
	char passphrase[pwdlen];
	char *ptr = passphrase;
	
	if(open_db_exist("opening"))
		return false;

	my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
	
	if(!db_open(path, passphrase)) {
		fprintf(stderr, "Database opening unsuccessful.\n");
		return false;
	}
		
	return true;
}

//Encrypt the database. We don't need the path of the database,
//as it's read from the steel_open file. Only one database can be
//open at once.
void close_database()
{
	if(!steel_tracker_file_exists())
		return;
	
	size_t pwdlen = 255;
	char passphrase[pwdlen];
	char *ptr = passphrase;
	char pass2[pwdlen];
	char *ptr2 = pass2;
	
	my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
	my_getpass(MASTER_PWD_PROMPT_RETRY, &ptr2, &pwdlen, stdin);
	
	if(strcmp(passphrase, pass2) != 0) {
		fprintf(stderr, "Passphrases do not match.\n");
		return;
	}
	
	db_close(passphrase);
}

//This is called from main. Adds new entry to the database.
void add_new_entry(char *title, char *user, char *url, char *note)
{
	if(!steel_tracker_file_exists())
		return;
	
	int id;
	//Should be enough...
	size_t pwdlen = 255;
	char pass[pwdlen];
	char *ptr = pass;
	char pass2[pwdlen];
	char *ptr2 = pass2;
	
	id = db_get_next_id();
	
	if(id == -1) {
		fprintf(stderr, "Failed to add a new entry.\n");
		return;
	}
	
	my_getpass(ENTRY_PWD_PROMPT, &ptr, &pwdlen, stdin);
	my_getpass(ENTRY_PWD_PROMPT_RETRY, &ptr2, &pwdlen, stdin);
	
	if(strcmp(pass, pass2) != 0) {
		fprintf(stderr, "Passphrases do not match.\n");
		return;
	}
		
	Entry_t *entry = list_create(title, user, pass, url, note, id, NULL);

	if(!db_add_entry(entry)) {
		fprintf(stderr, "Failed to add a new entry.\n");
		return;
	}
	
	list_free(entry);
}

//Print all available entries to stdin.
//Database must not be encrypted.
void show_all_entries()
{
	if(!steel_tracker_file_exists())
		return;
	
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
	if(!steel_tracker_file_exists())
		return;
	
	Entry_t *entry = db_get_entry_by_id(id);
	
	if(entry == NULL) {
		fprintf(stderr, "Cannot show entry with id %d.\n", id);
		return;
	}
	
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
	if(!steel_tracker_file_exists())
		return;
	
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
	if(!steel_tracker_file_exists())
		return;
	
	Entry_t *list = db_get_all_entries();
	char *title = NULL;
	char *user = NULL;
	char *url = NULL;
	char *notes = NULL;
	
	if(list == NULL) {
		fprintf(stderr, "Cannot perform the search operation.\n");
		return;
	}
	
	Entry_t *new_head = list->next;
	
	while(new_head != NULL) {
		
		//Search for matching data
		title = my_strcasestr(new_head->title, search);
		user = my_strcasestr(new_head->user, search);
		url = my_strcasestr(new_head->url, search);
		notes = my_strcasestr(new_head->notes, search);
		
		//Check if we found something
		if(title != NULL || user != NULL || url != NULL || 
			notes != NULL) {
			
			list_print_one(new_head);
		}
		
		if(title != NULL)
			free(title);
		
		if(user != NULL)
			free(user);
		
		if(url != NULL)
			free(url);
		
		if(notes != NULL)
			free(notes);
		
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
	if(!steel_tracker_file_exists())
		return;
	
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
	
	if(entry == NULL) {
		fprintf(stderr, "Cannot replace %s from entry %d.\n",
			what, id);
		return;
	}
	
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
void generate_password(int length, int count)
{
	if(length < 6) {
		fprintf(stderr, "Minimum length is 6 characters.\n");
		return;
	}
	
	for(int i = 0; i < count; i++) {
		
		char *pass = generate_pass(length);
		
		if(pass == NULL) {
			fprintf(stderr, "Generating new password failed.\n");
			return;
		}
		
		printf("%s\n", pass);
		
		free(pass);
	}
}

//Show all tracked databases, including their encryption status and last
//modified date.
void show_database_statuses()
{
	int count;
	FILE *fp = NULL;
	//char *line = NULL;
	
	if(!steel_tracker_file_exists())
		return;

	fp = status_get_file_ptr("r");

	if(fp == NULL)
		return;

	count = status_count_file_lines(fp);

	if(count == -2) {
		fprintf(stdout, "No databases found.\n");
		fclose(fp);
		return;
	}

	rewind(fp);
	
	while(count >= 0) {
		char *line = NULL;
		line = status_read_file_line(fp);

		if(line == NULL) {
			fprintf(stderr, "Error reading line. Corrupted .steel_dbs file?\n");
			fclose(fp);
			return;
		}
		
		if(!db_file_exists(line)) {
			fprintf(stderr, "Database file %s does not exist.\n", 
				line);
			fprintf(stderr, "Will disable tracking for it.\n");
			//Move to the entry of the steel_dbs
			status_del_tracking(line);
			count--;
			free(line);
			continue;
		}

		if(is_file_encrypted(line))
			fprintf(stdout, "%s\t%s\t%s\n", "[Encrypted]", 
				db_last_modified(line), line);
		else
			fprintf(stdout, "%s\t%s\t%s\n", "[Decrypted]", 
				db_last_modified(line), line);
		
		free(line);
		count--;
	}
	
	fclose(fp);
}

//Shreds the database file pointed by path.
//If the database is decrypted (currently the open one)
//function will also remove .steel_open file.
//Method will also remove entry from steel_dbs tracker file.
void remove_database(const char *path)
{
	if(!steel_tracker_file_exists())
		return;
	
	bool encrypted = false;
	char ch;
	
	encrypted = is_file_encrypted(path);
	
	fprintf(stdout, "Are you sure? (y/N) ");
	
	ch = getc(stdin);
	
	if(ch == 'y' || ch == 'Y') {
	
		if(db_shred(path)) {
			status_del_tracking(path);

			if(!encrypted)
				db_remove_lockfile();
		}
		else {
			fprintf(stderr, "Unable to shred the database.\n");
		}
	}
	else {
		fprintf(stdout, "Aborted.\n");
	}
}

void backup_database(const char *source, const char *dest)
{
	if(!steel_tracker_file_exists())
		return;
	
	if(!backup_export(source, dest)) {
		fprintf(stderr, "Unable to backup the database.\n");
	}
}

//Function does not check the existence of existing databases,
//As we of course want to allow the first database to be imported one.
//Function also sets tracking status for the imported database.
void backup_import_database(const char *source, const char *dest)
{
	if(!backup_import(source, dest)) {
		fprintf(stderr, "Unable to import the backup.\n");
		return;
	}
}

void show_passphrase_only(int id)
{
	if(!steel_tracker_file_exists())
		return;
	
	Entry_t *entry = db_get_entry_by_id(id);
	
	if(entry == NULL) {
		fprintf(stderr, "Cannot process entry with id %d.\n", id);
		return;
	}
	
	//Skip the first one, it's initialization data.
	Entry_t *next = entry->next;
	
	if(next != NULL)
		fprintf(stdout, "%s\n", next->pwd);
	else
		printf("No entry found with id %d.\n", id);

	list_free(entry);	
}

void show_username_only(int id)
{
	if(!steel_tracker_file_exists())
		return;
	
	Entry_t *entry = db_get_entry_by_id(id);
	
	if(entry == NULL) {
		fprintf(stderr, "Cannot process entry with id %d.\n", id);
		return;
	}
	
	//Skip the first one, it's initialization data.
	Entry_t *next = entry->next;
	
	if(next != NULL)
		fprintf(stdout, "%s\n", next->user);
	else
		printf("No entry found with id %d.\n", id);

	list_free(entry);
}

void show_url_only(int id)
{
	if(!steel_tracker_file_exists())
		return;
	
	Entry_t *entry = db_get_entry_by_id(id);
	
	if(entry == NULL) {
		fprintf(stderr, "Cannot process entry with id %d.\n", id);
		return;
	}
	
	//Skip the first one, it's initialization data.
	Entry_t *next = entry->next;
	
	if(next != NULL)
		fprintf(stdout, "%s\n", next->url);
	else
		printf("No entry found with id %d.\n", id);
	
	list_free(entry);
}

void show_notes_only(int id)
{
	if(!steel_tracker_file_exists())
		return;
	
	Entry_t *entry = db_get_entry_by_id(id);
	
	if(entry == NULL) {
		fprintf(stderr, "Cannot process entry with id %d.\n", id);
		return;
	}
	
	//Skip the first one, it's initialization data.
	Entry_t *next = entry->next;
	
	if(next != NULL)
		fprintf(stdout, "%s\n", next->notes);
	else
		printf("No entry found with id %d.\n", id);
	
	list_free(entry);
}