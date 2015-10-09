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

#ifndef __CRYPTO_H
#define __CRYPTO_H

#define KEY_SIZE (32) //256 bits
#define IV_SIZE (32) //256 bits
#define SALT_SIZE (8) //64 bits
#define HMAC_SIZE (32) //256 bits

typedef struct Key
{
	//C99 does not support variable size
	//arrays in the file scope
	
	char data[32]; //KEY_SIZE
	char salt[8];  //SALT_SIZE

} Key_t;

unsigned char *get_data_hmac(const char *data, long datalen, Key_t key);
bool hmac_file_content(const char *path, Key_t key);
bool verify_hmac(const unsigned char *old, const unsigned char *new);
bool encrypt_file(const char *path, const char *passphrase);
bool decrypt_file(const char *path, const char *passphrase);
bool verify_passphrase(const char *passphrase, const char *hash);
bool is_file_encrypted(const char *path);

#endif
