/*
 * Copyright (C) 2012 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MD5_H
#define MD5_H

#define MD5_CTX_SIZE 128

class Md5{
public:
	//compute the MD5 message digest of the n bytes at d and place it in md
	//(which must have space for 16 bytes of output). If md is NULL,
	//the digest is placed in a static array. 
	static unsigned char *calc(const void *d, unsigned long n, unsigned char *md);

	//convert a 16-byte digest to string representation.
	//the return value is in a static array.
	static char *toString(const unsigned char *md);

	//convert a string representation of MD5 to a 16-byte digest.
	//return value: if the conversion is successful.
	static bool fromString(const char* s, unsigned char *md);

	//Following are lower-level functions.

	//initializes the class for calculating MD5.
	void init();
	//add chunks of the message to be hashed (len bytes at data). 
	void update(const void *data, unsigned long len);
	//finished the caluclation, places the message digest in md,
	//which must have space for 16 bytes of output (or NULL).
	unsigned char *final(unsigned char *md);
private:
	//byte array, should be large enough to holding MD5_CTX.
	char md5_ctx[MD5_CTX_SIZE];
};

#endif
