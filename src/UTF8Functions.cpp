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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <string>
#include "UTF8Functions.h"

// A helper function to read a character from utf8 string
// s: the string
// p [in,out]: the position
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
int utf8ReadForward(const char* s, int& p) {
	int ch = (unsigned char)s[p];
	if (ch < 0x80){
		if (ch) p++;
		return ch;
	} else if (ch < 0xC0){
		// skip invalid characters
		while (((unsigned char)s[p] & 0xC0) == 0x80) p++;
		return -1;
	} else if (ch < 0xE0){
		int c2 = (unsigned char)s[++p];
		if ((c2 & 0xC0) != 0x80) return -1;

		ch = ((ch & 0x1F) << 6) | (c2 & 0x3F);
		p++;
		return ch;
	} else if (ch < 0xF0){
		int c2 = (unsigned char)s[++p];
		if ((c2 & 0xC0) != 0x80) return -1;
		int c3 = (unsigned char)s[++p];
		if ((c3 & 0xC0) != 0x80) return -1;

		ch = ((ch & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
		p++;
		return ch;
	} else if (ch < 0xF8){
		int c2 = (unsigned char)s[++p];
		if ((c2 & 0xC0) != 0x80) return -1;
		int c3 = (unsigned char)s[++p];
		if ((c3 & 0xC0) != 0x80) return -1;
		int c4 = (unsigned char)s[++p];
		if ((c4 & 0xC0) != 0x80) return -1;

		ch = ((ch & 0x7) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
		if (ch >= 0x110000) ch = -1;
		p++;
		return ch;
	} else {
		p++;
		return -1;
	}
}

// A helper function to read a character backward from utf8 string (experimental)
// s: the string
// p [in,out]: the position
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
int utf8ReadBackward(const char* s, int& p) {
	if (p <= 0) return 0;

	do {
		p--;
	} while (p > 0 && ((unsigned char)s[p] & 0xC0) == 0x80);

	int tmp = p;
	return utf8ReadForward(s, tmp);
}

const char* utf8GoToNextCharacter(const char* s) {
	if (*s == 0) return s;
	do {
		s++;
	} while (((unsigned char)(*s) & 0xC0) == 0x80);
	return s;
}

const char* utf8GoToPrevCharacter(const char* s) {
	do {
		s--;
	} while (((unsigned char)(*s) & 0xC0) == 0x80);
	return s;
}

bool utf32IsSpace(int ch) {
	switch (ch) {
	case 0x9: case 0xA: case 0xC: case 0xD: case 0x20: case 0xA0: case 0x1680:
	case 0x2028: case 0x2029: case 0x202F: case 0x205F: case 0x3000:
		return true;
	default:
		return (ch >= 0x2000 && ch <= 0x200B);
	}
}

bool utf32IsBreakableSpace(int ch) {
	switch (ch) {
	case 0x9: case 0xA: case 0xC: case 0xD: case 0x20: /* case 0xA0: */ case 0x1680:
	case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004: case 0x2005: case 0x2006: /* case 0x2007: */
	case 0x2008: case 0x2009: case 0x200A: case 0x200B:
	case 0x2028: case 0x2029: /* case 0x202F: */ case 0x205F: case 0x3000:
		return true;
	default:
		return false;
	}
}

bool utf32IsAlpha(int ch) {
	//ripped from the output of glib-2.60.0 (only a subset)

	static const int ranges_65_247[] = {
		65, 26,
		97, 26,
		170, 1,
		181, 1,
		186, 1,
		192, 23,
		216, 31,
	};
	static const int ranges_248_751[] = {
		248, 458,
		710, 12,
		736, 5,
		748, 1,
		750, 1,
	};
	static const int ranges_880_1328[] = {
		880, 5,
		886, 2,
		890, 4,
		895, 1,
		902, 1,
		904, 3,
		908, 1,
		910, 20,
		931, 83,
		1015, 139,
		1162, 166,
	};

	// skip 0x0530 - 0x1CFF

	static const int ranges_7424_8189[] = {
		7424, 192,
		7680, 278,
		7960, 6,
		7968, 38,
		8008, 6,
		8016, 8,
		8025, 1,
		8027, 1,
		8029, 1,
		8031, 31,
		8064, 53,
		8118, 7,
		8126, 1,
		8130, 3,
		8134, 7,
		8144, 4,
		8150, 6,
		8160, 13,
		8178, 3,
		8182, 7,
	};

	// skip 0x2000 - 0x10FFFF

	const int *ranges = NULL;
	int rangeSize = 0;

#define RANGE(LPS,LPE) ranges_##LPS##_##LPE
#define CHECK_RANGE(LPS,LPE) \
	else if (ch < LPS) { \
	} else if (ch < LPE) { \
		ranges = RANGE(LPS,LPE); rangeSize = sizeof(RANGE(LPS,LPE)) / sizeof(RANGE(LPS,LPE)[0]); \
	}

	if (false) {}
	CHECK_RANGE(65, 247)
	CHECK_RANGE(248, 751)
	CHECK_RANGE(880, 1328)
	CHECK_RANGE(7424, 8189)

	for (int i = 0; i < rangeSize; i += 2) {
		const int lps = ranges[i];
		const int lpe = lps + ranges[i + 1];
		if (ch < lps) break;
		else if (ch < lpe) {
			return true;
		}
	}

	return false;
}

bool utf32IsCJK(int ch) {
	return (ch >= 0x002E80 && ch <= 0x009FFF) /* CJK scripts and symbols */
		|| (ch >= 0x00F900 && ch <= 0x00FAFF) /* CJK Compatibility Ideographs */
		|| (ch >= 0x00FE30 && ch <= 0x00FE4F) /* CJK Compatibility Forms */
		|| (ch >= 0x020000 && ch <= 0x03FFFF) /* Supplementary Ideographic Plane & Tertiary Ideographic Plane */
		;
}

bool utf32IsCJKEndingPunctuation(int ch) {
	//ripped from M$ Word
	switch (ch) {
	case 0x21: case 0x25: case 0x29: case 0x2C: case 0x2E: case 0x3A: case 0x3B: case 0x3E: case 0x3F: case 0x5D: case 0x7D:
	case 0xA2: case 0xA8: case 0xB0: case 0xB7:
	case 0x2C7: case 0x2C9:
	case 0x2015: case 0x2016: case 0x2019: case 0x201D: case 0x2026: case 0x2030: case 0x2032: case 0x2033: case 0x203A: case 0x2103: case 0x2236:
	case 0x3001: case 0x3002: case 0x3003: case 0x3009: case 0x300B: case 0x300D: case 0x300F: case 0x3011: case 0x3015: case 0x3017: case 0x301E:
	case 0x0FE36: case 0x0FE3A: case 0x0FE3E: case 0x0FE40: case 0x0FE44: case 0x0FE5A: case 0x0FE5C: case 0x0FE5E:
	case 0x0FF01: case 0x0FF02: case 0x0FF05: case 0x0FF07: case 0x0FF09: case 0x0FF0C: case 0x0FF0E: case 0x0FF1A: case 0x0FF1B: case 0x0FF1F:
	case 0x0FF3D: case 0x0FF40: case 0x0FF5C: case 0x0FF5D: case 0x0FF5E: case 0x0FFE0:
		return true;
	default:
		return false;
	}
}

bool utf32IsCJKStartingPunctuation(int ch) {
	//ripped from M$ Word
	switch (ch) {
	case 0x24: case 0x28: case 0x5B: case 0x7B:
	case 0xA3: case 0xA5: case 0xB7:
	case 0x2018: case 0x201C:
	case 0x3008: case 0x300A: case 0x300C: case 0x300E: case 0x3010: case 0x3014: case 0x3016: case 0x301D:
	case 0x0FE59: case 0x0FE5B: case 0x0FE5D:
	case 0x0FF04: case 0x0FF08: case 0x0FF0E:
	case 0x0FF3B: case 0x0FF5B: case 0x0FFE1: case 0x0FFE5:
		return true;
	default:
		return false;
	}
}

int utf32ToLower(int ch) {
	//ripped from the output of glib-2.60.0

	static const int ranges_65_223[] = {
		65, 26, 32,
		192, 23, 32,
		216, 7, 32,
	};
	static const int ranges_304_504[] = {
		304, 1, -199,
		376, 1, -121,
		385, 1, 210,
		390, 1, 206,
		393, 2, 205,
		398, 1, 79,
		399, 1, 202,
		400, 1, 203,
		403, 1, 205,
		404, 1, 207,
		406, 1, 211,
		407, 1, 209,
		412, 1, 211,
		413, 1, 213,
		415, 1, 214,
		422, 1, 218,
		425, 1, 218,
		430, 1, 218,
		433, 2, 217,
		439, 1, 219,
		452, 1, 2,
		455, 1, 2,
		458, 1, 2,
		497, 1, 2,
		502, 1, -97,
		503, 1, -56,
	};
	static const int ranges_544_582[] = {
		544, 1, -130,
		570, 1, 10795,
		573, 1, -163,
		574, 1, 10792,
		579, 1, -195,
		580, 1, 69,
		581, 1, 71,
	};
	static const int ranges_895_1018[] = {
		895, 1, 116,
		902, 1, 38,
		904, 3, 37,
		908, 1, 64,
		910, 2, 63,
		913, 17, 32,
		931, 9, 32,
		975, 1, 8,
		1012, 1, -60,
		1017, 1, -7,
	};
	static const int ranges_1021_1367[] = {
		1021, 3, -130,
		1024, 16, 80,
		1040, 32, 32,
		1216, 1, 15,
		1329, 38, 48,
	};
	static const int ranges_4256_5110[] = {
		4256, 38, 7264,
		4295, 1, 7264,
		4301, 1, 7264,
		5024, 80, 38864,
		5104, 6, 8,
	};
	static const int ranges_7312_8499[] = {
		7312, 43, -3008,
		7357, 3, -3008,
		7838, 1, -7615,
		7944, 8, -8,
		7960, 6, -8,
		7976, 8, -8,
		7992, 8, -8,
		8008, 6, -8,
		8025, 1, -8,
		8027, 1, -8,
		8029, 1, -8,
		8031, 1, -8,
		8040, 8, -8,
		8072, 8, -8,
		8088, 8, -8,
		8104, 8, -8,
		8120, 2, -8,
		8122, 2, -74,
		8124, 1, -9,
		8136, 4, -86,
		8140, 1, -9,
		8152, 2, -8,
		8154, 2, -100,
		8168, 2, -8,
		8170, 2, -112,
		8172, 1, -7,
		8184, 2, -128,
		8186, 2, -126,
		8188, 1, -9,
		8486, 1, -7517,
		8490, 1, -8383,
		8491, 1, -8262,
		8498, 1, 28,
	};
	static const int ranges_11264_11392[] = {
		11264, 47, 48,
		11362, 1, -10743,
		11363, 1, -3814,
		11364, 1, -10727,
		11373, 1, -10780,
		11374, 1, -10749,
		11375, 1, -10783,
		11376, 1, -10782,
		11390, 2, -10815,
	};
	static const int ranges_42877_42932[] = {
		42877, 1, -35332,
		42893, 1, -42280,
		42922, 1, -42308,
		42923, 1, -42319,
		42924, 1, -42315,
		42925, 1, -42305,
		42926, 1, -42308,
		42928, 1, -42258,
		42929, 1, -42282,
		42930, 1, -42261,
		42931, 1, 928,
	};
	static const int ranges_65313_125218[] = {
		65313, 26, 32,
		66560, 40, 40,
		66736, 36, 40,
		68736, 51, 64,
		71840, 32, 32,
		93760, 32, 32,
		125184, 34, 34,
	};

	static const int ranges2_256_440[] = {
		256, 302,
		306, 310,
		313, 327,
		330, 374,
		377, 381,
		386, 388,
		391, 391,
		395, 395,
		401, 401,
		408, 408,
		416, 420,
		423, 423,
		428, 428,
		431, 431,
		435, 437,
		440, 440,
	};
	static const int ranges2_444_590[] = {
		444, 444,
		453, 453,
		456, 456,
		459, 475,
		478, 494,
		498, 500,
		504, 542,
		546, 562,
		571, 571,
		577, 577,
		582, 590,
	};
	static const int ranges2_880_1326[] = {
		880, 882,
		886, 886,
		984, 1006,
		1015, 1015,
		1018, 1018,
		1120, 1152,
		1162, 1214,
		1217, 1229,
		1232, 1326,
	};
	static const int ranges2_7680_11506[] = {
		7680, 7828,
		7840, 7934,
		8579, 8579,
		11360, 11360,
		11367, 11371,
		11378, 11378,
		11381, 11381,
		11392, 11490,
		11499, 11501,
		11506, 11506,
	};
	static const int ranges2_42560_42936[] = {
		42560, 42604,
		42624, 42650,
		42786, 42798,
		42802, 42862,
		42873, 42875,
		42878, 42886,
		42891, 42891,
		42896, 42898,
		42902, 42920,
		42932, 42936,
	};

	const int *ranges = NULL, *ranges2 = NULL;
	int rangeSize = 0, range2Size = 0;

#define RANGE(LPS,LPE) ranges_##LPS##_##LPE
#define CHECK_RANGE(LPS,LPE) \
	else if (ch < LPS) { \
	} else if (ch < LPE) { \
		ranges = RANGE(LPS,LPE); rangeSize = sizeof(RANGE(LPS,LPE)) / sizeof(RANGE(LPS,LPE)[0]); \
	}

#define RANGE2(LPS,LPE) ranges2_##LPS##_##LPE
#define CHECK_RANGE2(LPS,LPE) \
	else if (ch < LPS) { \
	} else if (ch <= LPE) { \
		ranges2 = RANGE2(LPS,LPE); range2Size = sizeof(RANGE2(LPS,LPE)) / sizeof(RANGE2(LPS,LPE)[0]); \
	}

	if (false) {}
	CHECK_RANGE(65, 223)
	CHECK_RANGE(304, 504)
	CHECK_RANGE(544, 582)
	CHECK_RANGE(895, 1018)
	CHECK_RANGE(1021, 1367)
	CHECK_RANGE(4256, 5110)
	CHECK_RANGE(7312, 8499)
	CHECK_RANGE(11264, 11392)
	CHECK_RANGE(42877, 42932)
	CHECK_RANGE(65313, 125218)

	for (int i = 0; i < rangeSize; i += 3) {
		const int lps = ranges[i];
		const int lpe = lps + ranges[i + 1];
		if (ch < lps) break;
		else if (ch < lpe) {
			return ch + ranges[i + 2];
		}
	}

	if (false) {}
	CHECK_RANGE2(256, 440)
	CHECK_RANGE2(444, 590)
	CHECK_RANGE2(880, 1326)
	CHECK_RANGE2(7680, 11506)
	CHECK_RANGE2(42560, 42936)

	for (int i = 0; i < range2Size; i += 2) {
		const int lps = ranges2[i];
		const int lpe = ranges2[i + 1];
		if (ch < lps) break;
		else if (ch <= lpe) {
			if (((ch - lps) & 0x1) == 0) return ch + 1;
			else break;
		}
	}

#undef RANGE
#undef RANGE2
#undef CHECK_RANGE
#undef CHECK_RANGE2

	return ch;
}
