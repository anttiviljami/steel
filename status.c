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

//Needed for getline() 
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Steel uses .steel_dbs file to track existing databases in the system.
//This system is really simple, database paths are added into .steel_dbs
//file and removed from there. If file path is in the file, Steel "knows"
//it, if not, Steel does not know anything about it. Really, it's that simple.

//Get count file lines by new line characters.
//Returns -1 on error, -2 when file has no lines.
//Caller must close the file pointer after it's no longer
//needed.
int status_count_file_lines(FILE *fp)
{
	int count = 0;
	int ch = 0;

	if (!fp)
		return -1;
	
	while (!feof(fp)) {
		ch = fgetc(fp);

		if (ch == '\n')
			count++;
	}

	//Return the count, ignoring the last empty line
	if (count == 0)
		return -2;
	else
		return count - 1;
}

//Return one line from the file.
//NULL is returned on failure. Caller must close the file pointer
//when it's no longer needed.
char *status_read_file_line(FILE *fp)
{
	if(fp == NULL)
		return NULL;

	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	static char *retval = NULL;
	
	if( (read = getline(&line, &len, fp) == -1) )
		return NULL;

	//We don't want the trailing new line
	retval = strtok(line, "\n");

	return retval;
}

//Returns FILE pointer, file opened in mode
//defined by param mode. Returns NULL on failure.
//Called must close the FILE pointer after it no longer
//needed.
FILE *status_get_file_ptr(char *mode)
{
	char *path = NULL;
	char *env = NULL;
	FILE *fp = NULL;
	
	env = getenv("HOME");

	if(env == NULL) {
		fprintf(stderr, "Failed to get env.\n");
		return NULL;
	}

	//+12 for /.steel_dbs
	path = calloc(1, (strlen(env) + 12) * sizeof(char));

	if(path == NULL) {
		fprintf(stderr, "Malloc failed.\n");
		return NULL;
	}

	strcpy(path, env);
	strcat(path, "/.steel_dbs");

	fp = fopen(path, mode);

	if(fp == NULL) {
		fprintf(stderr, "Failed to open %s.\n", path);
		free(path);
		return NULL;
	}

	free(path);
	
	return fp;
}

//Set database path to be "tracked".
void status_set_tracking(const char *path)
{
	FILE *fp = NULL;

	fp = status_get_file_ptr("a");

	if(fp == NULL)
		return;

	fprintf(fp, "%s\n", path);
	
	fclose(fp);
}

//Deletes given path from the status file, if found.
//Returns -1 on error or line number where the line
//was removed.
int status_del_tracking(const char *path)
{
	FILE *fp = NULL;
	int count;
	int size;
	char *line = NULL;
	int current = 0;
	int linefound = 0;
	
	fp = status_get_file_ptr("r");

	if(fp == NULL)
		return -1;

	count = status_count_file_lines(fp);

	if(count == -2) {
		fclose(fp);
		return -1;
	}

	//Move the file pointer back to the beginning of the file.
	rewind(fp);
	
	size = count;
	char *lines[size];

	//Read each line to lines.
	while(count >= 0) {

		line = status_read_file_line(fp);

		if(line == NULL) {
			fprintf(stderr, "Error reading line.\n");
			fclose(fp);
			return -1;
		}

		//Skip line that matches the one we want to remove.
		if(strcmp(line, path) != 0) {
			lines[current] = line;
			linefound = current;
		}

		free(line);
		
		current++;
		count--;
	}

	fclose(fp);

	//Open the file again with write access and write all lines
	//except the one which was deleted.
	fp = status_get_file_ptr("w");

	for(int i = 0; i < current; i++)
		fprintf(fp, "%s\n", lines[i]);

	fclose(fp);

	return linefound;
}
