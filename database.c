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
#include <sqlite3.h>
#include <sys/stat.h>


#include "database.h"
#include "crypto.h"


static int cb_get_entries(void *list, int argc, char **argv, char **column_name);
static int cb_get_next_id(void *id, int argc, char **argv, char **column_name);
static int cb_get_by_id(void *list, int argc, char **argv, char **column_name);

//Returns true is file exists and false if not.
//Function should be portable.
static bool file_exists(const char *path)
{
	bool retval = false;
	struct stat buffer;

	if (stat(path, &buffer) == 0)
		retval = true;

	return retval;
}

//Get Steel lockfile path.
//Lock file is a simple file which has the currently opened
//database path. Returns NULL on failure. Caller must free the return
//value.
static char *get_lockfile_path()
{
	char *path = NULL;
	char *env = NULL;
	size_t len = 0;
	
	env = getenv("HOME");

	if(env == NULL) {
		fprintf(stderr, "Failed to get home path\n");
		return NULL;
	}

	len = strlen(env);
	
	// +13 for /.steel_open filename
	path = malloc((len + 13) * sizeof(char));
	
	if(path == NULL) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}

	strcpy(path, env);
	strcat(path, "/.steel_open");

	return path;
}

//Remove the lock file. Called on db close.
static void remove_lockfile()
{
	char *path = get_lockfile_path();

	if(path == NULL)
		return;
	
	if(file_exists(path)) {
		remove(path);
	}

	free(path);
}

//Creates the lock file and writes content to it.
//Content should be the currently opened database path.
static void create_lockfile(const char *content)
{
	FILE *fp = NULL;
	char *path = NULL;

	path = get_lockfile_path();

	if(path == NULL)
		return;
	
	fp = fopen(path,"w");

	if(fp == NULL) {
		free(path);
		fprintf(stderr, "Failed to create lock file\n");
		return;
	}

	fprintf(fp, "%s", content);
	fclose(fp);
	free(path);
}

//Reads the currently opened database path from the
//lock file. Returns NULL on failure, the path on success.
//Caller must free the return value.
static char *read_path_from_lockfile()
{
	char *lockfile = NULL;
	FILE *fp = NULL;
	static char *data = NULL;
	size_t len = 0;
	
	lockfile = get_lockfile_path();

	if(lockfile == NULL)
		return NULL;

	fp = fopen(lockfile, "r");

	if(!fp) {
		free(lockfile);
		return NULL;
	}

	//We only need to read the first line of the file,
	//it contains the path to the currently open db file
	getline(&data, &len, fp);
	
	fclose(fp);
	free(lockfile);

	return data;
}

//Initializes new Steel database to the path given.
//After creation, the database is encrypted with the passphrase.
//Returns true on success, false on failure. Function does not override
//existing database in the path.
bool db_init(const char *path, const char *passphrase)
{
	sqlite3 *db;
	char *error = NULL;
	int retval;
	char *sql;

	if(file_exists(path)) {
		fprintf(stderr, "File already exists\n");
		return false;
	}

	retval = sqlite3_open(path, &db);

	if(retval) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		return false;
	}

	sql = "create table entries(" \
		"title TEXT," \
		"user  TEXT," \
		"passphrase TEXT," \
		"url TEXT," \
		"notes TEXT," \
		"id INTEGER PRIMARY KEY AUTOINCREMENT);";

	retval = sqlite3_exec(db, sql, NULL, 0, &error);

	if(retval != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", error);
		sqlite3_free(error);
		sqlite3_close(db);
		return false;
	}

	sqlite3_close(db);

	if(!encrypt_file(path, passphrase)) {
		fprintf(stderr, "Encryption failed\n");
		return false;
	}
	
	return true;
}

//Decrypt the encrypted database pointed by path.
//Returns true on success, false on failure.
//Path is also written to the lock file.
bool db_open(const char *path, const char *passphrase)
{
	if(!file_exists(path)) {
		fprintf(stderr, "%s: does not exists\n", path);
		return false;
	}
	
	if(!decrypt_file(path, passphrase)) {
		fprintf(stderr, "Decryption failed\n");
		return false;
	}

	//Write path as content to our lock file
	//to determine what db file is open.
	create_lockfile(path);
	
	return true;
}

//Encrypt database file with passphrase and
//remove lock file.
void db_close(const char *passphrase)
{
	//get db path from lock file
	char *path = NULL;

	path = read_path_from_lockfile();
	
	if(path == NULL) {
		fprintf(stderr, "Failed not read the path\n");
		return;
	}

	if(!file_exists(path)) {
		fprintf(stderr, "%s: does not exists\n", path);
		free(path);
		return;
	}
	
	if(!encrypt_file(path, passphrase)) {
		fprintf(stderr, "Encryption failed\n");
		free(path);
		return;
	}
	
	remove_lockfile();

	free(path);
}

static bool db_make_sanity_check(char *path)
{	
	if(path == NULL) {
		fprintf(stderr, "Database is encrypted or does not exists.\n");
		return false;
	}

	if(!file_exists(path)) {
		fprintf(stderr, "%s: does not exists\n", path);
		free(path);
		return false;
	}

	if(is_file_encrypted(path)) {
		//This should not happen, ever
		//If we can get get the path from the lockfile and it's encrypted
		//there's something wrong. Lock file should not even exists when
		//database is encrypted.
		fprintf(stderr, "%s: is encrypted.\n", path);
		free(path);
		return false;
	}
	
	return true;
}

bool db_add_entry(Entry_t *entry)
{
	int rc;
	sqlite3 *db;
	char *path = NULL;
	char *error = NULL;
	char *sql;

	path = read_path_from_lockfile();
	
	if(!db_make_sanity_check(path)) {
		
		if(path != NULL)
			free(path);
		
		return false;
	}

	rc = sqlite3_open(path, &db);

	if(rc) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		free(path);
		return false;
	}
	
	sql =
	sqlite3_mprintf("insert into entries (title, user, passphrase, url, notes)" \
			"values('%q','%q','%q','%q','%q')", entry->title, entry->user, 
			entry->pwd, entry->url, entry->notes);
	
	rc = sqlite3_exec(db, sql, NULL, 0, &error);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", error);
		sqlite3_free(error);
		free(sql);
		free(path);
		return false;
	}

	free(sql);
	sqlite3_free(error);

	sqlite3_close(db);
	free(path);
	
	return true;
}

Entry_t *db_get_all_entries()
{
	char *path = NULL;
	sqlite3 *db;
	int rc;
	char *sql;
	char *error = NULL;
	Entry_t *list = NULL;

	path = read_path_from_lockfile();
	
	if(!db_make_sanity_check(path)) {
		
		if(path != NULL)
			free(path);
		
		return NULL;
	}

	rc = sqlite3_open(path, &db);

	if(rc) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		free(path);
		return NULL;
	}
	
	//First item in our list will be the column names. This makes there
	//formatting easier during the output.
	list = list_create("Title", "User", "Passphrase", "Address", "Id", -1, NULL);
	
	sql = "select * from entries;";
	rc = sqlite3_exec(db, sql, cb_get_entries, list, &error);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", error);
		sqlite3_free(error);
		free(path);
		return NULL;
	}

	sqlite3_close(db);
	free(path);

	return list;
}

int db_get_next_id()
{
	char *path = NULL;
	sqlite3 *db;
	int rc;
	char *sql;
	char *error = NULL;
	int id = -1;

	path = read_path_from_lockfile();
	
	if(!db_make_sanity_check(path)) {
		
		if(path != NULL)
			free(path);
		
		return -1;
	}

	rc = sqlite3_open(path, &db);

	if(rc) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		free(path);
		return NULL;
	}
	
	//This will get us the last available auto increment id
	sql = "select * from sqlite_sequence where name='entries';";
	
	rc = sqlite3_exec(db, sql, cb_get_next_id, &id, &error);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", error);
		sqlite3_free(error);
		free(path);
		return NULL;
	}

	sqlite3_close(db);
	free(path);

	//Plus one to get the next one, not the last one.
	return id + 1;
}

Entry_t *db_get_entry_by_id(int id)
{
	char *path = NULL;
	sqlite3 *db;
	int rc;
	char *sql;
	char *error = NULL;
	Entry_t *list = NULL;
	
	path = read_path_from_lockfile();
	
	if(!db_make_sanity_check(path)) {
		
		if(path != NULL)
			free(path);
		
		return NULL;
	}
	
	rc = sqlite3_open(path, &db);

	if(rc) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		free(path);
		return NULL;
	}
	
	sql =
	sqlite3_mprintf("select * from entries where id=%d;", id);

	list = list_create("Title", "User", "Passphrase", "Address", "Id", -1, NULL);
	
	rc = sqlite3_exec(db, sql, cb_get_by_id, list, &error);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", error);
		sqlite3_free(error);
		free(path);
		return NULL;
	}

	sqlite3_close(db);
	free(path);
	
	return list;
}

bool db_delete_entry_by_id(int id, bool *success)
{
	char *path = NULL;
	sqlite3 *db;
	int rc;
	char *sql;
	char *error = NULL;
	int count;
	
	path = read_path_from_lockfile();
	
	if(!db_make_sanity_check(path)) {
		
		if(path != NULL)
			free(path);
		
		return false;
	}
	
	rc = sqlite3_open(path, &db);

	if(rc) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		free(path);
		return false;
	}
	
	sql =
	sqlite3_mprintf("delete from entries where id=%d;", id);

	rc = sqlite3_exec(db, sql, NULL, 0, &error);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", error);
		sqlite3_free(error);
		free(path);
		return false;
	}

	count = sqlite3_changes(db);
	
	if(count > 0)
		*success = true;
	
	sqlite3_close(db);
	free(path);
	
	return true;
	
}

//Database actions callback functions
static int cb_get_entries(void *list, int argc, char **argv, char **column_name)
{	
	//Insert entries into the list
	list_add(list, argv[0], argv[1], argv[2], argv[3], argv[4], atoi(argv[5]));

	return 0;	
}

static int cb_get_next_id(void *id, int argc, char **argv, char **column_name)
{	
	for(int i = 0; i < argc; i++) {
		if(strcmp(column_name[i], "seq") == 0) {
			//Assing the value(int) of the column to our void pointer.
			(*(int*)id) = atoi(argv[i]);
		}
	}
	
	return 0;	
}

static int cb_get_by_id(void *list, int argc, char **argv, char **column_name)
{
	
	list_add(list, argv[0], argv[1], argv[2], argv[3], argv[4], atoi(argv[5]));
	
	return 0;
}

//End database callback functions