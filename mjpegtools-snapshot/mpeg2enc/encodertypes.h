#ifndef _ENCODERTYPES_H
#define _ENCODERTYPES_H 

/* Representation classes for many various kinds of geometric entities
   floating around in an MPEG2 encoder...
*/

/*  (C) 2000-2004 Andrew Stevens */

/* These modifications are free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */


#include <stdlib.h>

class Parity
{
public:
    inline static int Invert( int x ) { return 1-x;}
    static const int top = 0;
    static const int bot = 1;
    static const int dim = 2;
    typedef int type;
};

class FieldOrder				/* Same as topfirst bit of picture */
{
public:
    static const int botfirst = 0;
    static const int topfirst = 1;
    static const int dim = 2;
    typedef int type;
};

class Dim
{
public:
    static const int X = 0;
    static const int Y = 1;
    static const int dim = 2;
    typedef int type;
};

class Coord;
class MotionVector
{
public:
	inline MotionVector() {}
	inline MotionVector( int x, int y ) 
	{
		val[Dim::X] = x; val[Dim::Y] = y;
	}
    inline static MotionVector
		Frame( const Coord &ref, const Coord &pred );
	inline static MotionVector 
		Field( const Coord &ref, const Coord &pred );
    inline int &operator [] (int i) { return val[i]; }
    inline const int &operator [] (int i) const { return val[i]; }
	inline int CodingPenaltyForSAD() const
	{
		return (abs(val[Dim::X]) + abs(val[Dim::Y]))<<3;
	}

	inline void Zero() { val[Dim::Y] = val[Dim::X] = 0; }

private:
		int val[Dim::dim];
};


class Coord 
{
public:
	inline Coord() {}
	
    inline Coord( int _x, int _y ) : x(_x), y(_y) {}
		
	inline Coord( const Coord &pred, const MotionVector &mv ) : 
		x( pred.x + mv[Dim::X] ), 
		y( pred.y + mv[Dim::Y] )
	{
	}

		inline void ToField() { y >>= 1; }
		inline void ToFrame() { y <<= 1; }
		inline void ToHalfPel() { x <<= 1; y <<= 1; }
		inline void ToFullPel() { x >>= 1; y >>= 1; }

	static inline Coord Field(const Coord &base) 
	{
		return Coord( base.x, base.y >> 1 );
	}

	static inline Coord Frame(const Coord &base)
	{
		return Coord( base.x, base.y << 1 );
	}
	
	static inline Coord HalfPel(const Coord &base)
	{
		return Coord( base.x << 1, base.y << 1 );
	}

	static inline Coord FullPel(const Coord &base)
	{
		return Coord( base.x >> 1, base.y >> 1 );
	}
	
	int x;
	int y;
};



inline MotionVector
MotionVector::Frame( const Coord &ref, const Coord &pred )
{
	return MotionVector( ref.x - pred.x, ref.y - pred.y );
}

inline MotionVector 
MotionVector::Field( const Coord &ref, const Coord &pred )
{
	return MotionVector(ref.x - pred.x,(ref.y<<1) - pred.y );
}

struct Rectangle
{
    Coord toplft;
    Coord botrgt;
};



 

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
