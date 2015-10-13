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

#ifndef __CMD_UI_H
#define __CMD_UI_H

#define MASTER_PWD_PROMPT "Master passphrase: "
#define ENTRY_PWD_PROMPT "Enter new passphrase: "
#define ENTRY_PWD_PROMPT_RETRY "Retype new passphrase: "

void add_new_entry(char *title, char *user, char *url, char *note);
bool init_database(const char *path, const char *passphrase);
bool open_database(const char *path, const char *passphrase);
void close_database(const char *passphrase);
void show_all_entries();
void show_one_entry(int id);
void delete_entry(int id);
void find_entries(const char *search);
size_t my_getpass(char *prompt, char **lineptr, size_t *n, FILE *stream);
void replace_part(int id, const char *what, const char *new_data);
void generate_password(int count);

#endif