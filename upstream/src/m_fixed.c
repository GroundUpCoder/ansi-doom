//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Fixed point implementation.
//



#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#include "m_fixed.h"




// Fixme. __USE_C_FIXED__ or something.

// (a * b) >> 16 without 64-bit integers: split b into its integer and
// fractional halves and add the two partial products.
fixed_t
FixedMul
( fixed_t	a,
  fixed_t	b )
{
    unsigned int ua = a < 0 ? -a : a;
    unsigned int ub = b < 0 ? -b : b;
    unsigned int ahi = ua >> 16, alo = ua & 0xffff;
    unsigned int bhi = ub >> 16, blo = ub & 0xffff;
    unsigned int result;

    result = ahi * bhi * 65536
           + ahi * blo
           + alo * bhi
           + ((alo * blo) >> 16);

    if ((a ^ b) < 0)
    {
        // The original rounds toward minus infinity (arithmetic shift).
        if ((alo * blo) & 0xffff)
            return -(int) result - 1;
        return -(int) result;
    }

    return (int) result;
}



//
// FixedDiv, C version.
//

fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((abs(a) >> 14) >= abs(b))
    {
	return (a^b) < 0 ? INT_MIN : INT_MAX;
    }
    else
    {
        // (a << 16) / b without 64-bit integers: shift-subtract long
        // division on the 48-bit numerator (a in the high 32 bits).
	unsigned int ua = a < 0 ? -a : a;
	unsigned int ub = b < 0 ? -b : b;
	unsigned int rem = ua >> 16;   // high part of the numerator
	unsigned int num = ua << 16;   // low part of the numerator
	unsigned int quot = 0;
	int i;

	for (i = 0; i < 32; i++)
	{
	    unsigned int carry = rem >> 31;
	    rem = (rem << 1) | (num >> 31);
	    num <<= 1;
	    quot <<= 1;
	    if (carry || rem >= ub)
	    {
		rem -= ub;
		quot |= 1;
	    }
	}

	return ((a ^ b) < 0) ? -(int) quot : (int) quot;
    }
}

