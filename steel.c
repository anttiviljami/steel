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

#include "database.h"

#define PWD_PROMPT "Master passphrase: "

size_t my_getpass (char *prompt, char **lineptr, size_t *n, FILE *stream)
{
    struct termios _old, _new;
    int nread;

    /* Turn echoing off and fail if we can’t. */
    if(tcgetattr(fileno(stream), &_old) != 0)
        return -1;
    
    _new = _old;
    _new.c_lflag &= ~ECHO;
    
    if(tcsetattr(fileno(stream), TCSAFLUSH, &_new) != 0)
        return -1;

    if(prompt)
        printf("%s", prompt);

    /* Read the password. */
    nread = getline(lineptr, n, stream);

    if(nread >= 1 && (*lineptr)[nread - 1] == '\n')
    {
        (*lineptr)[nread - 1] = 0;
        nread--;
    }
    
    printf("\n");

    /* Restore terminal. */
    tcsetattr(fileno(stream), TCSAFLUSH, &_old);

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
			{"export",         required_argument, 0, 'e'},
			{"close",          required_argument, 0, 'c'},
			{"copy",           required_argument, 0, 'C'},
			{"show",           required_argument, 0, 's'},
			{"gen-pass",       no_argument,       0, 'g'},
			{"add",            required_argument, 0, 'a'},
			{"delete",         required_argument, 0, 'd'},
			{"replace",        required_argument, 0, 'r'},
			{"find",           required_argument, 0, 'f'},
			{"find-regex",     required_argument, 0, 'F'},
			{0, 0, 0, 0}

		};

		int option_index = 0;

		option = getopt_long(argc, argv, "i:o:e:c:C:s:ga:d:r:f:F:", long_options,
				&option_index);

		if(option == -1)
			break;

		switch(option) {
		case 'i': {
			
			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(PWD_PROMPT, &ptr, &pwdlen, stdin);
			
			if(!db_init(optarg, passphrase))
				return 0;
		
			break;
			
		}
		case 'o': {
			
			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(PWD_PROMPT, &ptr, &pwdlen, stdin);
			
			if(!db_open(optarg, passphrase))
				return 0;
		
			break;
			
		}
		case 'e':
			break;
		case 'c': {

			char passphrase[pwdlen];
			char *ptr = passphrase;

			my_getpass(PWD_PROMPT, &ptr, &pwdlen, stdin);
			db_close(passphrase);
		
			break;
		}
		case 'C':
			break;
		case 's':
			break;
		case 'g':
			break;
		case 'a':
			break;
		case 'd':
			break;
		case 'r':
			break;
		case 'f':
			break;
		case 'F':
			break;
		}


	}
	
	return 0;
}
