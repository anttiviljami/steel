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
#include <getopt.h>
#include "cmd_ui.h"

#define VERSION 1.0

static void usage()
{
	
	
}

//Program entry point.
int main(int argc, char *argv[])
{
	int option;
	//Max passphrase lenght. Should be enough, really.
	size_t pwdlen = 255;

	while(true) {

		static struct option long_options[] =
		{
			{"init-new",       required_argument, 0, 'i'},
			{"open",           required_argument, 0, 'o'},
			{"close",          no_argument,       0, 'c'},
			{"show",           required_argument, 0, 's'},
			{"gen-pass",       no_argument,       0, 'g'}, //todo
			{"add",            required_argument, 0, 'a'},
			{"delete",         required_argument, 0, 'd'},
			{"replace",        required_argument, 0, 'r'}, //todo
			{"find",           required_argument, 0, 'f'},
			{"list-all",       no_argument,       0, 'l'},
			{"version",        no_argument,       0, 'V'},
			{0, 0, 0, 0}

		};

		int option_index = 0;

		option = getopt_long(argc, argv, "i:o:cs:ga:d:r:f:lV", long_options,
				&option_index);

		if(option == -1)
			break;

		switch(option) {
		case 'i': {
			
			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
			
			if(!init_database(optarg, passphrase))
				return 0;
		
			break;
		}
		case 'o': {
			
			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
			
			if(!open_database(optarg, passphrase))
				return 0;
		
			break;
		}
		case 'c': {

			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(MASTER_PWD_PROMPT, &ptr, &pwdlen, stdin);
			close_database(passphrase);
		
			break;
		}
		case 's':
			show_one_entry(atoi(optarg));
			break;
		case 'g':
			break;
		case 'a': {
			//Has user?
			if(!argv[optind]) {
				fprintf(stderr, "Missing option user.\n");
				return 0;
			}
			//Has url?
			if(!argv[optind + 1]) {
				fprintf(stderr, "Missing option url.\n");
				return 0;
			}
			//Has note?
			if(!argv[optind + 2]) {
				fprintf(stderr, "Missing option note\n");
				return 0;
			}
			
			char *title = optarg;
			char *user = argv[optind];
			char *url = argv[optind + 1];
			char *note = argv[optind + 2];
			add_new_entry(title, user, url, note);
			break;
		}
		case 'd':
			delete_entry(atoi(optarg));
			break;
		case 'r': {
			if(!argv[optind]) {
				fprintf(stderr, "Missing option, see -h for help\n");
				return 0;
			}
			
			//Replacing passphrase does not need third argument
			//It will be asked separately by replace_part()
			if(strcmp(argv[optind], "passphrase") != 0) {
				if(!argv[optind + 1]) {
					fprintf(stderr, "Missing option, see -h for help\n");
					return 0;
				}
			}
			
			int id = atoi(optarg);
			char *what = argv[optind];
			char *content = argv[optind + 1];
			replace_part(id, what, content);
			break;
		}
		case 'f':
			find_entries(optarg);
			break;
		case 'l':
			show_all_entries();
			break;
		case 'V':
			printf("Steel v%.1f Copyright (c) Niko Rosvall\n", VERSION);
			break;
		}


	}
	
	return 0;
}
