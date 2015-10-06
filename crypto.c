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
#include "bcrypt/bcrypt.h"

//Our magic number that's written into the
//encrypted file. Used to determine if the file
//is encrypted.
static const int MAGIC_HEADER = 0x33497545;

static const int KEY_SIZE = 32; //256 bits
static const int IV_SIZE = 32; //256 bits
static const int SALT_SIZE = 8;

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

static char *generate_random_data(size_t size)
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

	fread(data, size, 1, frnd);
	fclose(frnd);
	
	return data;
}

static bool write_bcrypt_hash(FILE *fOut, const char *passphrase)
{
	char salt[BCRYPT_HASHSIZE];
	char hash[BCRYPT_HASHSIZE];
	int ret;

	ret = bcrypt_gensalt(12, salt);

	if(ret != 0) {
		fprintf(stderr, "Could not generate salt\n");
		return false;
	}

	ret = bcrypt_hashpw(passphrase, salt, hash);

	if(ret != 0) {
		fprintf(stderr, "Could not hash password\n");
		return false;
	}

	fwrite(hash, 1, BCRYPT_HASHSIZE, fOut);
	
	return true;
}

static Key_t generate_key(const char *passphrase, bool *success)
{
	int ret;
	char *keybytes = NULL;
	Key_t key;
	char *saltbytes = NULL;
	
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

	//memmove(key.data, keybytes, KEY_SIZE);
	//memmove(key.salt, saltbytes, SALT_SIZE);
	
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

bool is_file_encrypted(const char *path)
{
	FILE *fp = NULL;
	int data;
	
	fp = fopen(path, "r");

	if(fp == NULL) {
		fprintf(stderr, "Failed to open file\n");
		return false;
	}

	//Skip hash, we don't need it here
	fseek(fp, BCRYPT_HASHSIZE, SEEK_SET);
	
	fread((void *)&data, sizeof(MAGIC_HEADER), 1, fp);
	fclose(fp);

	if(data != MAGIC_HEADER)
		return false;
	
	return true;
}

bool verify_passphrase(const char *passphrase, const char *hash)
{
	int ret;
	
	ret = bcrypt_checkpw(passphrase ,hash);

	if(ret != 0)
		return false;
	
	return true;
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

	if(!write_bcrypt_hash(fOut, passphrase)) {
		fprintf(stderr, "Bcrypt failed\n");
		fclose(fIn);
		fclose(fOut);
		free(IV);
		free(output_filename);
		mcrypt_generic_deinit(td);
		mcrypt_module_close(td);
		
		return false;
	}

	fwrite((void *)&MAGIC_HEADER, sizeof(MAGIC_HEADER), 1, fOut);
	fwrite(IV, 1, IV_SIZE, fOut);
	fwrite(key.salt, 1, SALT_SIZE, fOut);

	//Encrypt rest of the file content (the actual data,
	//that needs to be protected)
	while(fread(&block, 1, 1, fIn) == 1) {
		mcrypt_generic(td, &block, 1);
		fwrite(&block, 1, 1, fOut);
	}
	
	mcrypt_generic_deinit(td);
	mcrypt_module_close(td);

	free(IV);
	
	fclose(fIn);
	fclose(fOut);

	remove(path);
	rename(output_filename, path);

	free(output_filename);
	
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
	char hash[BCRYPT_HASHSIZE];

	if(!is_file_encrypted(path)) {
		fprintf(stderr, "File is not encrypted with Steel\n");
		return false;
	}
	
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

	//Read bcrypt hash, iv and salt from the beginning of the file
	fread(hash, BCRYPT_HASHSIZE, 1, fIn);
	//Skip the magic header, file's already checked
	fseek(fIn, sizeof(int), SEEK_CUR);

	fread(IV, IV_SIZE, 1, fIn);
	fread(salt, SALT_SIZE, 1, fIn);

	//Verify passphrase
	if(!verify_passphrase(passphrase, hash)) {
		fprintf(stderr, "Invalid passphrase\n");
		free(IV);
		free(salt);
		fclose(fIn);

		return false;
	}
	
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
	
	free(salt);
	
	fclose(fIn);
	fclose(fOut);

	//Only remove original file if decryption was successful
	if(!decryption_failed) {
		remove(path);
		rename(output_filename, path);
	}

	free(output_filename);
	
	return true;
}
