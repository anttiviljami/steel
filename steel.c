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
#include <getopt.h>

#include "database.h"

int main(int argc, char *argv[])
{
	int option;

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
		case 'i':
			if(argv[optind]) {
				if(!db_init(optarg, argv[optind]))
					return 0;
			}
			else {
				fprintf(stderr, "Missing passphrase, see -h\n");
			}
			break;
		case 'o':
			if(argv[optind]) {
				if(!db_open(optarg, argv[optind]))
					return 0;
			}
			else {
				fprintf(stderr, "Missing passphrase, see -h\n");
			}
			break;
		case 'e':
			break;
		case 'c':
			break;
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
