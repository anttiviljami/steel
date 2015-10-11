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
#include <getopt.h>
#include <termios.h>

#include "cmd_ui.h"

#define PWD_PROMPT "Master passphrase: "

size_t my_getpass (char *prompt, char **lineptr, size_t *n, FILE *stream)
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

int main(int argc, char *argv[])
{
	int option;
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
			{"find-regex",     required_argument, 0, 'F'}, //todo
			{"list-all",       no_argument,       0, 'l'},
			{0, 0, 0, 0}

		};

		int option_index = 0;

		option = getopt_long(argc, argv, "i:o:cs:ga:d:r:f:F:l", long_options,
				&option_index);

		if(option == -1)
			break;

		switch(option) {
		case 'i': {
			
			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(PWD_PROMPT, &ptr, &pwdlen, stdin);
			
			if(!init_database(optarg, passphrase))
				return 0;
		
			break;
		}
		case 'o': {
			
			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(PWD_PROMPT, &ptr, &pwdlen, stdin);
			
			if(!open_database(optarg, passphrase))
				return 0;
		
			break;
		}
		case 'c': {

			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(PWD_PROMPT, &ptr, &pwdlen, stdin);
			close_database(passphrase);
		
			break;
		}
		case 's':
			show_one_entry(atoi(optarg));
			break;
		case 'g':
			break;
		case 'a':
			add_new_entry("test","nikorosvall@foo.com","1q2w3ee3w2q1","http://byteptr.com","my notes sdasdasd sad");
			break;
		case 'd':
			delete_entry(atoi(optarg));
			break;
		case 'r':
			break;
		case 'f':
			find_entries(optarg);
			break;
		case 'F':
			break;
		case 'l':
			show_all_entries();
			break;
		}


	}
	
	return 0;
}
