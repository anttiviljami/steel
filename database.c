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
#include "entries.h"
#include "crypto.h"

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
		"id INTEGER PRIMARY KEY);";

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

void db_export_text(const char *path)
{

}

void db_list_all()
{

}

bool db_add_entry(const char *title, const char *user,
		const char *pass, const char *url, const char *note)
{
	int retval;
	sqlite3 *db;
	Entry_t *entry;
	char *path;

	path = read_path_from_lockfile();
	
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

	entry = create_new_entry(title, user, pass, url, note);

	if(entry == NULL) {
		fprintf(stderr, "Failed to create note\n");
		free(path);
		return false;
	}
	
	retval = sqlite3_open(path, &db);

	if(retval) {
		fprintf(stderr, "Can't initialize: %s\n", sqlite3_errmsg(db));
		free(path);
		return false;
	}
	
	if(!entry_add(db, entry))
		fprintf(stderr, "Adding new entry failed\n");
	
	
	entry_free(entry);

	sqlite3_close(db);
	free(path);
	
	return true;
}
