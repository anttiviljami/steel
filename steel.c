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
#include <string.h>
#include <getopt.h>
#include "cmd_ui.h"

#define VERSION 0.9

static void usage()
{
#define HELP "\
SYNOPSIS\n\
\n\
steel [options]\n\
\n\
OPTIONS\n\
\n\
-i, --init-new          <path>                        Create a new database\n\
-o, --open              <path>                        Open an existing database\n\
-c, --close                                           Close open database\n\
-a, --add               <title> <user> <url> <notes>  Add new entry to database\n\
-s, --show              <id>                          Show entry by id\n\
-g, --gen-pass                                        Generate secure password\n\
-d, --delete            <id>                          Delete an entry by id\n\
-r, --replace           <id> <what> [content]         Replace an entry data\n\
						      <what> can be either \"user\",\n\
						      \"title\", \"url\", \"notes\" or\n\
					              \"passphrase\".\n\
-R, --shred-db          <path>                        Shred database\n\
-f, --find              <search>                      Search database\n\
-l, --list-all                                        Show all entries\n\
-S, --show-status                                     Show database statuses\n\
-b, --backup            <source> <destination>        Backup database\n\
-B, --import-backup     <source> <destination>        Import database backup\n\
-V, --version                                         Show program version\n\
-p, --show-passphrase   <id>                          Show an entry passphrase\n\
-u, --show-username     <id>                          Show an entry username\n\
-U, --show-url          <id>                          Show an entry url\n\
-n, --show-notes        <id>                          Show an entry notes\n\
-h, --help                                            Show short help and exit.\n\
\n\
For more information and examples see man steel(1).\n\
\n\
AUTHORS\n\
Copyright (C) 2015 Niko Rosvall <niko@byteptr.com>\n\
\n\
Released under license GPL-3+. For more information, see\n\
http://www.gnu.org/licenses\n\
"
	printf(HELP);
}

//Program entry point.
int main(int argc, char *argv[])
{
	int option;

	if(argc == 1) {
		usage();
		return 0;
	}
		
	while(true) {

		static struct option long_options[] =
		{
			{"init-new",               required_argument, 0, 'i'},
			{"backup",                 required_argument, 0, 'b'},
			{"import-backup",          required_argument, 0, 'B'},
			{"open",                   required_argument, 0, 'o'},
			{"close",                  no_argument,       0, 'c'},
			{"show",                   required_argument, 0, 's'},
			{"gen-pass",               required_argument, 0, 'g'},
			{"add",                    required_argument, 0, 'a'},
			{"delete",                 required_argument, 0, 'd'},
			{"replace",                required_argument, 0, 'r'},
			{"shred-db",               required_argument, 0, 'R'},
			{"find",                   required_argument, 0, 'f'},
			{"list-all",               no_argument,       0, 'l'},
			{"show-status",            no_argument,       0, 'S'},
			{"version",                no_argument,       0, 'V'},
			{"help",                   no_argument,       0, 'h'},
			{"show-passphrase",        required_argument, 0, 'p'},
			{"show-username",          required_argument, 0, 'u'},
			{"show-url",               required_argument, 0, 'U'},
			{"show-notes",             required_argument, 0, 'n'},
			{0, 0, 0, 0}

		};

		int option_index = 0;

		option = getopt_long(argc, argv, "i:b:B:o:cs:g:a:d:r:f:lR:SVp:u:U:n:h", 
				     long_options, &option_index);

		if(option == -1)
			break;

		switch(option) {
		case 'i':
			init_database(optarg);
			break;
		case 'b':
			if(!argv[optind]) {
				fprintf(stderr, "Missing option <destination>.\n");
				return 0;
			}
			backup_database(optarg, argv[optind]);
			break;
		case 'B':
			if(!argv[optind]) {
				fprintf(stderr, "Missing option <destination>.\n");
				return 0;
			}
			backup_import_database(optarg, argv[optind]);
			break;
		case 'o':
			open_database(optarg);
			break;
		case 'c':
			close_database();
			break;
		case 's':
			show_one_entry(atoi(optarg));
			break;
		case 'g':
			generate_password(atoi(optarg));
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
		case 'p':
			show_passphrase_only(atoi(optarg));
			break;
		case 'u':
			show_username_only(atoi(optarg));
			break;
		case 'U':
			show_url_only(atoi(optarg));
			break;
		case 'n':
			show_notes_only(atoi(optarg));
			break;
		case 'l':
			show_all_entries();
			break;
		case 'R':
			remove_database(optarg);
			break;
		case 'S':
			show_database_statuses();
			break;
		case 'V':
			printf("Steel v%.1f Copyright (c) Niko Rosvall\n", VERSION);
			break;
		case 'h':
			usage();
			break;
		}

	}
	
	return 0;
}
