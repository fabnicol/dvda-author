#ifndef __SET_H__
#define __SET_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// Set is an STL-like set class.
// Handles the STL equivalent of MultiSet, too.

#include "SkipList.hh"
#include "TemplateLib.hh"



template <class TYPE, class PRED = Less<TYPE>,
	class IMP = SkipList<TYPE,TYPE,Ident<TYPE,TYPE>,PRED> >
class Set
{
public:
	typedef IMP Imp;
		// The type that implements our internal operations.

	typedef typename Imp::InitParams InitParams;
		// How that internal type is initialized.

private:
	Imp m_oImp;
		// How we implement ourselves.
	
public:
	typedef typename Imp::Allocator_t Allocator;
		// The type of allocator to use to allocate items in the set.

	Set (const PRED &a_rPred = PRED(),
				Allocator &a_rAlloc = Imp::sm_oNodeAllocator)
			: m_oImp (a_rPred, a_rAlloc) {}
		// Default constructor.  Must be followed by Init().
	
	Set (Status_t &a_reStatus, bool a_bAllowDuplicates = false,
			const InitParams &a_rInitParams = InitParams(),
			const PRED &a_rPred = PRED(),
			Allocator &a_rAlloc = Imp::sm_oNodeAllocator)
		: m_oImp (a_reStatus, a_bAllowDuplicates, a_rInitParams, a_rPred,
			a_rAlloc) {}
		// Initializing constructor.
	
	void Init (Status_t &a_reStatus, bool a_bAllowDuplicates = false,
				const InitParams &a_rInitParams = InitParams())
			{ m_oImp.Init (a_reStatus, a_bAllowDuplicates, a_rInitParams); }
		// Construction method.
	
	virtual ~Set (void) {}
		// Destructor.
	
#ifdef DEBUG_SKIPLIST
	
	void SetDebug (bool a_bDebug) { m_oImp.SetDebug (a_bDebug); }
		// Set whether to run the set invariant before and after
		// methods.
	
	void Invariant (void) const { m_oImp.Invariant(); }
		// Thoroughly analyze the set for structural integrity.
	
#endif // DEBUG_SKIPLIST
	
	typedef typename Imp::Iterator Iterator;
	typedef typename Imp::ConstIterator ConstIterator;
	typedef typename Imp::InsertResult InsertResult;
		// Iterator classes & other helpers.
	
	//
	// Set methods.
	//
	
	Iterator Begin (void) { return m_oImp.Begin(); }
		// Return an iterator to the beginning of the list.
	
	ConstIterator Begin (void) const { return m_oImp.Begin(); }
		// Return an iterator to the beginning of the list.
	
	Iterator End (void) { return m_oImp.End(); }
		// Return an iterator to the end of the list.
	
	ConstIterator End (void) const { return m_oImp.End(); }
		// Return an iterator to the end of the list.
	
	int32_t Size (void) const { return m_oImp.Size(); }
		// Return the number of items in the list.
		// (May be called on a default-constructed object, making it
		// possible for default-constructed subclasses/owners to destroy
		// themselves safely.)
	
	bool Empty (void) const { return m_oImp.Empty(); }
		// Return whether the list is empty.
	
	InsertResult Insert (Status_t &a_reStatus, const TYPE &a_rValue)
			{ return m_oImp.Insert (a_reStatus, a_rValue); }
		// Insert an item into the list.
	
	Iterator Insert (Status_t &a_reStatus, Iterator a_itPosition,
				const TYPE &a_rValue)
		{ return m_oImp.Insert (a_reStatus, a_itPosition, a_rValue); }
		// Insert an item into the list, at this exact location, if it's
		// safe.  Returns where it was really inserted.
	
	void Insert (Status_t &a_reStatus, ConstIterator a_itFirst,
				ConstIterator a_itLast)
			{ m_oImp.Insert (a_reStatus, a_itFirst, a_itLast); }
		// Insert a range of items from another set.
	
	Iterator Erase (Iterator a_itHere)
			{ return m_oImp.Erase (a_itHere); }
		// Erase the item here.  Return the item following the one
		// removed.
	
	Iterator Erase (Iterator a_itFirst, Iterator a_itLast)
			{ return m_oImp.Erase (a_itFirst, a_itLast); }
		// Erase a range of items in this list.  Return the item
		// following the  last one removed.
	
	void Clear (void) { m_oImp.Clear(); }
		// Empty the list.
	
	void Purge (void) { m_oImp.Purge(); }
		// Purge all internally-allocated memory.
	
	InsertResult Move (Set<TYPE,PRED,IMP> &a_rOther, Iterator a_itHere)
			{ return m_oImp.Move (a_rOther.m_oImp, a_itHere); }
		// Remove an item from another set and insert it into ourselves.
		// Just like an Erase() followed by an Insert(), except that
		// there's no possibility of the operation failing.
	
	void Move (Set<TYPE,PRED,IMP> &a_rOther, Iterator a_itFirst,
				Iterator a_itLast)
			{ m_oImp.Move (a_rOther.m_oImp, a_itFirst, a_itLast); }
		// Remove a range of items from another set and insert them
		// into ourselves.
		// Just like an Erase() followed by an Insert(), except that
		// there's no possibility of the operation failing.
	
	void Move (Set<TYPE,PRED,IMP> &a_rOther)
			{ m_oImp.Move (a_rOther.m_oImp); }
		// Move all items from the other set to ourself.
		// The current set must be empty.

	bool CanMove (const Set<TYPE,PRED,IMP> &a_rOther) const
			{ return m_oImp.CanMove (a_rOther.m_oImp); }
		// Returns true if the two sets can move items between
		// each other.

	void Assign (Status_t &a_reStatus, const Set<TYPE,PRED,IMP> &a_rOther)
			{ m_oImp.Assign (a_reStatus, a_rOther.m_oImp); }
		// Assign the contents of the other set to ourselves.
	
	Iterator Find (const TYPE &a_rKey) { return m_oImp.Find (a_rKey); }
		// Find the given item in the list.  Returns End() if not found.
	
	ConstIterator Find (const TYPE &a_rKey) const
			{ return m_oImp.Find (a_rKey); }
		// Find the given item in the list.  Returns End() if not found.
	
	Iterator LowerBound (const TYPE &a_rKey)
			{ return m_oImp.LowerBound (a_rKey); }
		// Return the position of the first item that's >= the key.
	
	ConstIterator LowerBound (const TYPE &a_rKey) const
			{ return m_oImp.LowerBound (a_rKey); }
		// Return the position of the first item that's >= the key.
	
	Iterator UpperBound (const TYPE &a_rKey)
			{ return m_oImp.UpperBound (a_rKey); }
		// Return the position of the first item that's > the key.
	
	ConstIterator UpperBound (const TYPE &a_rKey) const
			{ return m_oImp.UpperBound (a_rKey); }
		// Return the position of the first item that's > the key.

	size_t GetSizeOfLargestNode (void) const
			{ return m_oImp.GetSizeOfLargestNode(); }
		// Return the size of the largest possible skip-list node.
		//
		// (Not generally useful...needed by VariableSizeAllocator,
		// to make sure all free-space blocks can become part of an
		// in-place skip-list.)
};



#endif // __SET_H__
