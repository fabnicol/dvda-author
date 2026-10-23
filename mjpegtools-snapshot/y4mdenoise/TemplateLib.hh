#ifndef __TEMPLATELIB_H__
#define __TEMPLATELIB_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// Basic template definitions.



enum { g_knBitsPerByte = 8 };
	// The number of bits per byte.  (I doubt this'll ever change,
	// but just in case, we make a constant for it.)



// Calcluate the size of an array of items.
#define ARRAYSIZE(CLASS,COUNT) ((size_t)(&(((CLASS*)0)[COUNT])))



// A template function class that declares two classes are the same.
template <class FIRST, class SECOND>
class Ident
{
public:
	const SECOND &operator() (const FIRST &a_rOther) const
  		{ return a_rOther; }
};



// A generic pair.
template <class FIRST, class SECOND>
class Pair
{
public:
	Pair()
		: m_oFirst (FIRST()), m_oSecond (SECOND()) {}
	Pair (const FIRST &a_oFirst, const SECOND &a_oSecond)
		: m_oFirst (a_oFirst), m_oSecond (a_oSecond) {}
	template <class U, class V> Pair (const Pair<U,V> &a_rOther)
		: m_oFirst (a_rOther.m_oFirst), m_oSecond (a_rOther.m_oSecond)
			{}
	
	FIRST m_oFirst;
	SECOND m_oSecond;
};



// A template function that returns the first item (i.e. in a Pair).
template <class PAIR, class FIRST>
class Select1st
{
public:
	const FIRST &operator() (const PAIR &a_rOther) const
		{ return a_rOther.m_oFirst; }
};



// A standard less-than comparison object.
template <class TYPE>
class Less
{
public:
	bool operator() (const TYPE &a_rLeft, const TYPE &a_rRight) const
		{ return a_rLeft < a_rRight; }
};



// A general absolute-value function.
template <class TYPE>
TYPE
AbsoluteValue (const TYPE &a_rValue)
{
	return (a_rValue < TYPE (0)) ? (-a_rValue) : a_rValue;
}



// A general minimum-value function.
template <class TYPE>
TYPE
Min (const TYPE &a_rLeft, const TYPE &a_rRight)
{
	return (a_rLeft < a_rRight) ? a_rLeft : a_rRight;
}



// A general maximum-value function.
template <class TYPE>
TYPE
Max (const TYPE &a_rLeft, const TYPE &a_rRight)
{
	return (a_rLeft > a_rRight) ? a_rLeft : a_rRight;
}



#endif // __TEMPLATELIB_H__
