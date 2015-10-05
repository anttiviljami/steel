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
#include <mcrypt.h>
#include <stdint.h>
#include <mhash.h>

#include "crypto.h"

static const int KEY_SIZE = 32; //256 bits
static const int IV_SIZE = 32; //256 bits
static const int SALT_SIZE = 8; //64 bits

typedef struct Key
{
	//C99 does not support variable size
	//arrays in file scope
	
	char data[32]; //KEY_SIZE
	char salt[8];  //SALT_SIZE

} Key_t;

static char *get_output_filename(const char *orig, const char *ext)
{
	char *path = NULL;
	size_t len;

	len = strlen(orig) + strlen(ext) + 1;

	path = malloc(len * sizeof(char));

	if(!path) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}

	strcpy(path, orig);
	strcat(path, ext);

	return path;
}

static char *generate_random_data(int size)
{
	char *data = NULL;
	FILE *frnd = NULL;

	data = malloc(size);

	if(data == NULL) {
		fprintf(stderr, "Malloc failed\n");		
		return NULL;
	}
	
	frnd = fopen("/dev/urandom", "r");

	if(!frnd) {
		fprintf(stderr, "Cannot open /dev/urandom\n");
		free(data);
		return NULL;
	}

	fread(data, 1, size, frnd);
	fclose(frnd);
	
	return data;
}

static Key_t generate_key(const char *passphrase, bool *success)
{
	int ret;
	char *keybytes = NULL;
	char *saltbytes = NULL;
	Key_t key;
	
	keybytes = calloc(1, KEY_SIZE);

	if(!keybytes) {
		fprintf(stderr, "Calloc failed\n");
		*success = false;
		return key;
	}

	saltbytes = generate_random_data(SALT_SIZE);

	if(saltbytes == NULL) {
		fprintf(stderr, "Could not generate salt\n");
		free(keybytes);
		*success = false;
		return key;
	}
	
	ret = mhash_keygen(KEYGEN_MCRYPT, MHASH_SHA256, 0, keybytes,
			KEY_SIZE, saltbytes, SALT_SIZE, (uint8_t *)passphrase,
			(uint32_t)strlen(passphrase));

	if(ret < 0) {
		fprintf(stderr, "Key generation failed\n");
		free(keybytes);
		free(saltbytes);
		*success = false;
		return key;
	}
	
	strcpy(key.data, keybytes);
	strcpy(key.salt, saltbytes);

	free(keybytes);
	free(saltbytes);
	
	*success = true;
	
	return key;
}

static Key_t generate_key_salt(const char *passphrase, char *salt, bool *success)
{
	int ret;
	char *keybytes = NULL;
	Key_t key;
	
	keybytes = calloc(1, KEY_SIZE);

	if(!keybytes) {
		fprintf(stderr, "Calloc failed\n");
		*success = false;
		return key;
	}

	ret = mhash_keygen(KEYGEN_MCRYPT, MHASH_SHA256, 0, keybytes,
			KEY_SIZE, salt, SALT_SIZE, (uint8_t *)passphrase,
			(uint32_t)strlen(passphrase));

	if(ret < 0) {
		fprintf(stderr, "Key generation failed\n");
		free(keybytes);
		*success = false;
		return key;
	}
	
	strcpy(key.data, keybytes);
	strcpy(key.salt, salt);

	free(keybytes);
	
	*success = true;
	
	return key;
}

bool encrypt_file(const char *path, const char *passphrase)
{
	MCRYPT td;
	Key_t key;
	char block;
	char *IV = NULL;
	int ret;
	FILE *fIn = NULL;
	FILE *fOut = NULL;
	char *output_filename = NULL;
	bool success;
	
	key = generate_key(passphrase, &success);

	if(!success) {
		fprintf(stderr, "Failed to get new key\n");
		return false;
	}
	
	td = mcrypt_module_open("rijndael-256", NULL, "cfb", NULL);

	if(td == MCRYPT_FAILED) {
		fprintf(stderr, "Opening mcrypt module failed\n");
		return false;
	}

	IV = generate_random_data(IV_SIZE);

	if(IV == NULL) {
		fprintf(stderr, "Could not create IV\n");
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);

		return false;
	}
	
	ret = mcrypt_generic_init(td, key.data, KEY_SIZE, IV);

	if(ret < 0) {
		mcrypt_perror(ret);
		free(IV);
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);
		
		return false;
	}

	fIn = fopen(path, "r");

	if(!fIn) {
		fprintf(stderr, "Failed to open file\n");
		free(IV);
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);
		
		return false;
	}

	output_filename = get_output_filename(path, ".steel"); 

	fOut = fopen(output_filename, "w");

	if(!fOut) {
		fprintf(stderr, "Failed to open output file\n");
		fclose(fIn);
		free(IV);
		free(output_filename);
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);
		
		return false;
	}

	fwrite(IV, 1, IV_SIZE, fOut);
	fwrite(key.salt, 1, SALT_SIZE, fOut);

	while(fread(&block, 1, 1, fIn) == 1) {
		mcrypt_generic(td, &block, 1);
		fwrite(&block, 1, 1, fOut);
	}
	
	mcrypt_generic_deinit(td);
	mcrypt_module_close(td);

	free(IV);
	free(output_filename);

	fclose(fIn);
	fclose(fOut);

	remove(path);
	
	return true;
}

bool decrypt_file(const char *path, const char *passphrase)
{
	MCRYPT td;
	Key_t key;
	char block;
	char *IV = NULL;
	char *salt = NULL;
	int ret;
	FILE *fIn = NULL;
	FILE *fOut = NULL;
	char *output_filename = NULL;
	bool success;
	bool decryption_failed = false;

	IV = malloc(IV_SIZE);

	if(IV == NULL) {
		fprintf(stderr, "Malloc failed\n");
		return false;
	}

	salt = malloc(SALT_SIZE);

	if(salt == NULL) {
		fprintf(stderr, "Malloc failed\n");
		return false;
	}
	
	fIn = fopen(path, "r");

	if(!fIn) {
		fprintf(stderr, "Failed to open file\n");
		free(IV);
		free(salt);
		return false;
	}

	//Read iv and salt from the beginning of the file
	fread(IV, IV_SIZE, 1, fIn);
	fread(salt, SALT_SIZE, 1, fIn);
	
	key = generate_key_salt(passphrase, salt, &success);

	if(!success) {
		fprintf(stderr, "Failed to get new key\n");
		free(IV);
		free(salt);
		fclose(fIn);
		return false;
	}
	
	td = mcrypt_module_open("rijndael-256", NULL, "cfb", NULL);

	if(td == MCRYPT_FAILED) {
		fprintf(stderr, "Opening mcrypt module failed\n");
		free(IV);
		free(salt);
		fclose(fIn);
		
		return false;
	}

	ret = mcrypt_generic_init(td, key.data, KEY_SIZE, IV);

	if(ret < 0) {
		mcrypt_perror(ret);
		free(IV);
		free(salt);
		fclose(fIn);
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);
		
		return false;
	}

	output_filename = get_output_filename(path, ".plain");

	fOut = fopen(output_filename, "w");

	if(!fOut) {
		fprintf(stderr, "Failed to open output file\n");
		fclose(fIn);
		free(IV);
		free(salt);
		free(output_filename);
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);
		
		return false;
	}

	while(fread(&block, 1, 1, fIn) == 1) {

		if(mdecrypt_generic(td, &block, 1) != 0) {
			//If decryption fails, abort and remove output file
			fprintf(stderr, "Decryption failed\n");
			remove(output_filename);
			decryption_failed = true;
			break;
		}
		
		fwrite(&block, 1, 1, fOut);
	}
	
	mcrypt_generic_deinit(td);
	mcrypt_module_close(td);

	free(IV);
	free(output_filename);
	free(salt);
	
	fclose(fIn);
	fclose(fOut);

	//Only remove original file if decryption was successful
	if(!decryption_failed)
		remove(path);
	
	return true;
}
