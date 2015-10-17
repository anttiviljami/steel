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
#include <stdbool.h>
#include "backup.h"
#include "crypto.h"
#include "database.h"
#include "status.h"

//Function to copy a file from source to destination.
//Assumes that source exists and destionation does not exist.
//Returns true on success, false on failure.
static bool copy_file(const char *source, const char *dest)
{
	FILE *in = NULL;
	FILE *out = NULL;
	char block;
	
	in = fopen(source, "r");
	
	if(in == NULL) {
		fprintf(stderr, "Failed to open %s.\n", source);
		return false;
	}
	
	out = fopen(dest, "w");
	
	if(out == NULL) {
		fprintf(stderr, "Failed to open %s.\n", dest);
		fclose(in);
		return false;
	}

	while(fread(&block, 1, 1, in) == 1)
		fwrite(&block, 1, 1, out);
	
	fclose(in);
	fclose(out);
	
	return true;
}
//Take a backup of database file pointed by source and copy it
//to the path pointed by dest. If source is not encrypted function
//will abort and ask user to encrypt the file first. This is simply
//to enforce security. It's not a good idea to backup open passphrase
//databases. Returns true on success, false on failure.
bool backup_export(const char *source, const char *dest)
{
	if(!db_file_exists(source)) {
		fprintf(stderr, "%s does not exist.\n", source);
		return false;
	}
	
	if(db_file_exists(dest)) {
		fprintf(stderr, "%s already exists.\n", dest);
		return false;
	}
	
	if(!is_file_encrypted(source)) {
		fprintf(stdout, "Encrypt the file first before taking a backup.\n");
		return false;
	}
	
	if(!copy_file(source, dest)) {
		fprintf(stderr, "Backing up %s failed.\n", source);
		return false;
	}
	
	return true;
}

//Import backup database pointed by path. Function also adds
//the database into the steel_dbs file Steel to track it's status.
bool backup_import(const char *path)
{
	
	return true;
}
