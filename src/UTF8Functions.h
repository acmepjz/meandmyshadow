/*
 * Copyright (C) 2019 Me and My Shadow
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

#ifndef UTF8FUNCTIONS_H
#define UTF8FUNCTIONS_H

// A helper function to read a character from utf8 string and advance the pointer
// s: the string
// p [in,out]: the position
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
int utf8ReadForward(const char* s, int& p);

// A helper function to read a character backward from utf8 string and advance the pointer (experimental)
// s: the string
// p [in,out]: the position
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
int utf8ReadBackward(const char* s, int& p);

// A helper function to read the first character from utf8 string
// s: the string
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
// NOTE: Consider utf8ReadForward() instead if you want to read multiple characters
inline int utf8GetCharacter(const char* s) {
	int tmp = 0;
	return utf8ReadForward(s, tmp);
}

// A helper function to advance the pointer in a utf8 string to next character
// s: the pointer
// return value: the new pointer
// WARNING: there is no sanity check!
const char* utf8GoToNextCharacter(const char* s);

// A helper function to advance the pointer in a utf8 string to previous character
// s: the pointer
// return value: the new pointer
// WARNING: there is no sanity check!
const char* utf8GoToPrevCharacter(const char* s);

bool utf32IsSpace(int ch);
bool utf32IsAlpha(int ch);
bool utf32IsCJK(int ch);
bool utf32IsCJKEndingPunctuation(int ch); // check if the character should't be at start of line in CJK mode
bool utf32IsCJKStartingPunctuation(int ch); // check if the character should't be at end of line in CJK mode
int utf32ToLower(int ch);

#define U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(STR,I,M,CH,INVALID_CH) \
	for(size_t I=0;I<M;I++){ \
		int CH=(unsigned char)STR[I]; \
		if(CH<0x80){ \
		}else if(CH<0xC0){ \
			CH=INVALID_CH; \
		}else if(CH<0xE0){ \
			if(I+1>=M) CH=INVALID_CH; \
			else{ \
				int c2=(unsigned char)STR[I+1]; \
				if((c2&0xC0)!=0x80) CH=INVALID_CH; \
				else{ \
					CH=((CH & 0x1F)<<6) | (c2 & 0x3F); \
					I++; \
				} \
			} \
		}else if(CH<0xF0){ \
			if(I+2>=M) CH=INVALID_CH; \
			else{ \
				int c2=(unsigned char)STR[I+1]; \
				int c3=(unsigned char)STR[I+2]; \
				if((c2&0xC0)!=0x80 || (c3&0xC0)!=0x80) CH=INVALID_CH; \
				else{ \
					CH=((CH & 0xF)<<12) | ((c2 & 0x3F)<<6) | (c3 & 0x3F); \
					I+=2; \
				} \
			} \
		}else if(CH<0xF8){ \
			if(I+3>=M) CH=INVALID_CH; \
			else{ \
				int c2=(unsigned char)STR[I+1]; \
				int c3=(unsigned char)STR[I+2]; \
				int c4=(unsigned char)STR[I+3]; \
				if((c2&0xC0)!=0x80 || (c3&0xC0)!=0x80 || (c4&0xC0)!=0x80) CH=INVALID_CH; \
				else{ \
					CH=((CH & 0x7)<<18) | ((c2 & 0x3F)<<12) | ((c3 & 0x3F)<<6) | (c4 & 0x3F); \
					if(CH>=0x110000) CH=INVALID_CH; \
					else I+=3; \
				} \
			} \
		}else{ \
			CH=INVALID_CH; \
		}

#define U8STRING_FOR_EACH_CHARACTER_DO_END() }

#define U8_ENCODE(CH,OPERATION) \
	if(CH<0x80){ \
		OPERATION(CH); \
	}else if(CH<0x800){ \
		OPERATION(0xC0 | (CH>>6)); \
		OPERATION(0x80 | (CH & 0x3F)); \
	}else if(CH<0x10000){ \
		OPERATION(0xE0 | (CH>>12)); \
		OPERATION(0x80 | ((CH>>6) & 0x3F)); \
		OPERATION(0x80 | (CH & 0x3F)); \
	}else{ \
		OPERATION(0xF0 | (CH>>18)); \
		OPERATION(0x80 | ((CH>>12) & 0x3F)); \
		OPERATION(0x80 | ((CH>>6) & 0x3F)); \
		OPERATION(0x80 | (CH & 0x3F)); \
	}

#define U16STRING_FOR_EACH_CHARACTER_DO_BEGIN(STR,I,M,CH,INVALID_CH) \
	for(size_t I=0;I<M;I++){ \
		int CH=(unsigned short)(STR[I]); \
		if(CH<0xD800){ \
		}else if(CH<0xDC00){ \
			/* lead surrogate */ \
			I++; \
			if(I>=M) CH=INVALID_CH; \
			else{ \
				int c2=(unsigned short)STR[I]; \
				if(CH>=0xDC00 && CH<0xE000){ \
					/* trail surrogate */ \
					CH=0x10000 + (((CH & 0x3FF)<<10) | (c2 & 0x3FF)); \
				}else{ \
					/* invalid */ \
					CH=INVALID_CH; \
					I--; \
				} \
			} \
		}else if(CH<0xE000){ \
			/* invalid trail surrogate */ \
			CH=INVALID_CH; \
		}

#define U16STRING_FOR_EACH_CHARACTER_DO_END() }

#define U16_ENCODE(CH,OPERATION) \
	if(CH<0x10000){ \
		OPERATION(CH); \
	}else{ \
		OPERATION(0xD800 | ((CH-0x10000)>>10)); \
		OPERATION(0xDC00 | (CH & 0x3FF)); \
	}

const int REPLACEMENT_CHARACTER = 0x00FFFD;

#endif
