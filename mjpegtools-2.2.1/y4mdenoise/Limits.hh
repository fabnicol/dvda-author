#ifndef __LIMITS_H__
#define __LIMITS_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include "mjpeg_types.h"

// Numerical limits.



// A class that allows numerical limits to be looked up, for each
// numerical type.
template <class NUM>
class Limits
{
public:
	static NUM Min;
		// This type's minimum value.
	static NUM Max;
		// This type's maximum value.
	static int Bits;
		// The number of bits in this type.
	static int Log2Bits;
		// The base-2-logarithm of the number of bits in this type.
};



// Some limits.
template <> int8_t Limits<int8_t>::Min = int8_t (0x80);
template <> int8_t Limits<int8_t>::Max = int8_t (0x7f);
template <> int Limits<int8_t>::Bits = 8;
template <> int Limits<int8_t>::Log2Bits = 3;
template <> uint8_t Limits<uint8_t>::Min = uint8_t (0x00);
template <> uint8_t Limits<uint8_t>::Max = uint8_t (0xff);
template <> int Limits<uint8_t>::Bits = 8;
template <> int Limits<uint8_t>::Log2Bits = 3;
template <> int16_t Limits<int16_t>::Min = int16_t (0x8000);
template <> int16_t Limits<int16_t>::Max = int16_t (0x7fff);
template <> int Limits<int16_t>::Bits = 16;
template <> int Limits<int16_t>::Log2Bits = 4;
template <> uint16_t Limits<uint16_t>::Min = uint16_t (0x0000);
template <> uint16_t Limits<uint16_t>::Max = uint16_t (0xffff);
template <> int Limits<uint16_t>::Bits = 16;
template <> int Limits<uint16_t>::Log2Bits = 4;
template <> int32_t Limits<int32_t>::Min = int32_t (0x80000000);
template <> int32_t Limits<int32_t>::Max = int32_t (0x7fffffff);
template <> int Limits<int32_t>::Bits = 32;
template <> int Limits<int32_t>::Log2Bits = 5;
template <> uint32_t Limits<uint32_t>::Min = uint32_t (0x00000000);
template <> uint32_t Limits<uint32_t>::Max = uint32_t (0xffffffff);
template <> int Limits<uint32_t>::Bits = 32;
template <> int Limits<uint32_t>::Log2Bits = 5;
template <> int64_t Limits<int64_t>::Min
	= int64_t (0x8000000000000000LL);
template <> int64_t Limits<int64_t>::Max
	= int64_t (0x7fffffffffffffffLL);
template <> int Limits<int64_t>::Bits = 64;
template <> int Limits<int64_t>::Log2Bits = 6;
template <> uint64_t Limits<uint64_t>::Min
	= uint64_t (0x0000000000000000ULL);
template <> uint64_t Limits<uint64_t>::Max
	= uint64_t (0xffffffffffffffffULL);
template <> int Limits<uint64_t>::Bits = 64;
template <> int Limits<uint64_t>::Log2Bits = 6;



#endif // __LIMITS_H__
