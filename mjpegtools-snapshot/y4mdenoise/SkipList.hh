#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

/* A skip list is a sorted data structure with probabilistic balancing
   that performs the same functions as balanced-binary-tree sorts of
   data structures, but skip lists have lots of unusual avenues for
   optimizations not available to binary trees. */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include <new>
#include "mjpeg_types.h"
#include "Status_t.h"
#include "Allocator.hh"



// Define this to compile in code to double-check and debug the skip
// list.
#ifndef NDEBUG
//	#define DEBUG_SKIPLIST
#endif // NDEBUG



// The skip list.  Parameterized by the class of the sorting key, the
// class of the contained value, the method for extracting the key from
// the value, the method for comparing keys, the number of forward pointers
// in the header, and the type of allocator to use for skip-list nodes.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC = 10, class ALLOC = Allocator<HC> >
class SkipList
{
private:
	// The type of a node in the skip list.
	struct Node
	{
		VALUE m_oValue;
			// The data held by this node.
		#ifdef DEBUG_SKIPLIST
		int32_t m_nLevel;
		#endif // DEBUG_SKIPLIST
			// The number of forward pointers in this node.
		Node *m_pBackward;
			// The node before this one.
		Node *m_apForward[1];
			// The nodes after this one, at all the levels we exist.
	};

	SkipList (const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther);
	const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &operator =
			(const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther);
		// Disallow copying and assignment.

 	enum { HEADERCHUNK = HC };
 		// The number of forward pointers in the header.
 		// Will give good sorting for up to e^HC items.

public:
	typedef ALLOC Allocator_t;
		// The type of node allocator to use.

	static Allocator_t sm_oNodeAllocator;
		// The default node allocator.

	// A structure that contains all of our initialization parameters.
	struct InitParams
	{
	public:
		uint32_t m_nRandSeed;
			// The skip list's random number seed.

		bool m_bAllocateInternalNodesFromAllocator;
			// True if the skip list's internal nodes (i.e. the header
			// and search-fingers) should be allocated from the allocator.
			// False if they should be allocated from system memory.
			//
			// Since these nodes live for the lifetime of the skip-list,
			// allocating them from the allocator prevents the allocator
			// from being able to purge itself.

		InitParams() : m_nRandSeed (rand()),
				m_bAllocateInternalNodesFromAllocator (false) {}
			// Default constructor.
			// Gets the random seed from the system.

		InitParams (uint32_t a_nRandSeed,
				bool a_bAllocateInternalNodesFromAllocator)
			: m_nRandSeed (a_nRandSeed),
				m_bAllocateInternalNodesFromAllocator
					(a_bAllocateInternalNodesFromAllocator) {}
			// Initializing constructor.
	};

	SkipList (const PRED &a_rPred = PRED(),
			Allocator_t &a_rAlloc = sm_oNodeAllocator);
		// Default constructor.  Must be followed by Init().

	SkipList (Status_t &a_reStatus, bool a_bAllowDuplicates,
			const InitParams &a_rInitParams, const PRED &a_rPred = PRED(),
			Allocator_t &a_rAlloc = sm_oNodeAllocator);
		// Constructor.  Specify whether or not duplicates are allowed,
		// and provide a random number seed.

	void Init (Status_t &a_reStatus, bool a_bAllowDuplicates,
			const InitParams &a_rInitParams);
		// Construction method.  Specify whether or not duplicates are
		// allowed, and provide a random number seed.

	virtual ~SkipList (void);
		// Destructor.

#ifdef DEBUG_SKIPLIST

	void Invariant (void) const;
		// Thoroughly analyze the skip list for structural integrity.

	void SetDebug (bool a_bDebug);
		// Set whether to run the skip-list invariant before and after
		// methods.

#endif // DEBUG_SKIPLIST

	//
	// Iterator classes.
	//

	class Iterator;
	class ConstIterator;
	friend class ConstIterator;
	class ConstIterator
	{
		protected:
			friend class SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>;
				// Let SkipList access m_pNode.
			Node *m_pNode;
				// The node we represent.
		public:
			ConstIterator() { m_pNode = NULL; }
			ConstIterator (Node *a_pNode) : m_pNode (a_pNode) {}
			ConstIterator (const Iterator &a_rOther)
				: m_pNode (a_rOther.m_pNode) {}
			const VALUE &operator*() const {return m_pNode->m_oValue; }
			ConstIterator& operator++()
				{ m_pNode = m_pNode->m_apForward[0];
					return *this; }
			ConstIterator operator++(int) { ConstIterator oTmp = *this;
				++*this; return oTmp; }
			ConstIterator& operator--()
				{ m_pNode = m_pNode->m_pBackward;
					return *this; }
			ConstIterator operator--(int) { ConstIterator oTmp = *this;
				--*this; return oTmp; }
			bool operator== (const ConstIterator &a_rOther) const
				{ return (m_pNode == a_rOther.m_pNode) ? true : false; }
			bool operator!= (const ConstIterator &a_rOther) const
				{ return (m_pNode != a_rOther.m_pNode) ? true : false; }
	};
	friend class Iterator;
	class Iterator : public ConstIterator
	{
		private:
			typedef ConstIterator BaseClass;
				// Keep track of who our base class is.

		public:
			Iterator() : ConstIterator() {}
			Iterator (Node *a_pNode) : ConstIterator (a_pNode) {}
			VALUE &operator*() {return BaseClass::m_pNode->m_oValue; }
			Iterator& operator++() { BaseClass::m_pNode
				= BaseClass::m_pNode->m_apForward[0]; return *this; }
			Iterator operator++(int) { Iterator oTmp = *this; ++*this;
				return oTmp; }
			Iterator& operator--() { BaseClass::m_pNode
				= BaseClass::m_pNode->m_pBackward; return *this; }
			Iterator operator--(int) { Iterator oTmp = *this; --*this;
				return oTmp; }
			bool operator== (const ConstIterator &a_rOther) const
				{ return (((BaseClass&)(*this)) == a_rOther); }
			bool operator!= (const ConstIterator &a_rOther) const
				{ return (((BaseClass&)(*this)) != a_rOther); }
	};

	//
	// Skip list methods.
	//

	Iterator Begin (void)
			{ return Iterator (m_pHeader->m_apForward[0]); }
		// Return an iterator to the beginning of the list.

	ConstIterator Begin (void) const
			{ return ConstIterator (m_pHeader->m_apForward[0]); }
		// Return an iterator to the beginning of the list.

	Iterator End (void) { return Iterator (m_pHeader); }
		// Return an iterator to the end of the list.

	ConstIterator End (void) const { return ConstIterator (m_pHeader); }
		// Return an iterator to the end of the list.

	uint32_t Size (void) const { return m_nItems; }
		// Return the number of items in the list.
		// (May be called on a default-constructed object, making it
		// possible for default-constructed subclasses/owners to destroy
		// themselves safely.)

	bool Empty (void) const { return (m_nItems == 0) ? true : false; }
		// Return whether the list is empty.

	// A structure used to return the result of an insertion.
	struct InsertResult
	{
		Iterator m_itPosition;
			// Where the item was inserted, or where the duplicate was
			// found.

		bool m_bInserted;
			// true if the item was inserted into the list.
	};

	InsertResult Insert (Status_t &a_reStatus, const VALUE &a_rValue);
		// Insert an item into the list.

	Iterator Insert (Status_t &a_reStatus, Iterator a_oPosition,
			const VALUE &a_rValue);
		// Insert an item into the list, at this exact location, if it's
		// safe.  Returns where it was really inserted.

	void Insert (Status_t &a_reStatus, ConstIterator a_oFirst,
			ConstIterator a_oLast);
		// Insert a range of items from another skip-list.

	Iterator Erase (Iterator a_oHere);
		// Erase the item here.  Return the item following the one
		// removed.

	Iterator Erase (Iterator a_oFirst, Iterator a_oLast);
		// Erase a range of items in this list.  Return the item
		// following the last one removed.

	void Clear (void);
		// Empty the list.
	
	void Purge (void) { }
		// Purge all internally-allocated memory.
		// Not implemented for SkipList -- this is only here to preserve
		// interface compatibility with Vector<>.

	InsertResult Move (SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther,
			Iterator a_oHere);
		// Remove an item from another skip list and insert it into
		// ourselves.
		// Just like an Erase() followed by an Insert(), except that
		// there's no possibility of the operation failing.

	void Move (SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther,
			Iterator a_oFirst, Iterator a_oLast);
		// Remove a range of items from another skip-list and insert
		// them into ourselves.
		// Just like an Erase() followed by an Insert(), except that
		// there's no possibility of the operation failing.

	void Move (SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther);
		// Move all items from the other skip-list to ourself.
		// The current skip-list must be empty.

	bool CanMove (const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther)
			const;
		// Returns true if the two skip lists can move items between
		// each other.

	void Assign (Status_t &a_reStatus,
			const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther);
		// Assign the contents of the other skip list to ourselves.

	Iterator Find (const KEY &a_rKey);
		// Find the given item in the list.  Returns End() if not found.

	ConstIterator Find (const KEY &a_rKey) const;
		// Find the given item in the list.  Returns End() if not found.

	Iterator LowerBound (const KEY &a_rKey);
		// Return the position of the first item that's >= the key.

	ConstIterator LowerBound (const KEY &a_rKey) const;
		// Return the position of the first item that's >= the key.

	Iterator UpperBound (const KEY &a_rKey);
		// Return the position of the first item that's > the key.

	ConstIterator UpperBound (const KEY &a_rKey) const;
		// Return the position of the first item that's > the key.

	size_t GetSizeOfLargestNode (void) const;
		// Return the size of the largest possible skip-list node.
		//
		// (Not generally useful...needed by VariableSizeAllocator,
		// to make sure all free-space blocks can become part of an
		// in-place skip-list.)

private:

	Allocator_t &m_rNodeAllocator;
		// Where we get memory to allocate nodes.

	bool m_bAllowDuplicates;
		// true if we allow duplicate elements.

	int16_t m_nHeaderLevel;
		// How many valid pointers m_pHeader[] is holding.

	Node *m_pHeader;
		// The beginning of the list.

	Node *m_pSearchFinger, *m_pSearchFinger2;
		// Used to speed up searches, by caching the results of
		// previous searches.

	uint32_t m_nItems;
		// The number of items in this list.

	uint32_t m_nRandSeed;
		// The random-number seed.

	bool m_bAllocateInternalNodesFromAllocator;
		// True if the skip list's internal nodes (i.e. the header
		// and search-fingers) should be allocated from the allocator.
		// False if they should be allocated from system memory.
		//
		// Since these nodes live for the lifetime of the skip-list,
		// allocating them from the allocator prevents the allocator
		// from being able to purge itself.

	KEYFN m_oKeyFn;
		// How we extract a key from a value.

	PRED m_oPred;
		// How we compare keys to each other.

	void SearchLower (const KEY &a_rKey, Node *a_pTraverse) const;
		// Search for an item greater than or equal to a_rKey.

	bool SearchExact (Node *a_pKey, Node *a_pTraverse) const;
		// Search for this exact item.  Returns true if it's found.

	void SearchUpper (const KEY &a_rKey, Node *a_pTraverse) const;
		// Search for an item greater than a_rKey.

	void SearchEnd (Node *a_pTraverse) const;
		// Point to the end of the list.

	void InsertNode (Node *a_pNewNode, int16_t a_nNewLevel,
			Node *a_pTraverse);
		// Insert a new node into the skip list.  Assume a_pTraverse is
		// set up.

	Node *RemoveNode (Node *a_pToRemove, Node *a_pTraverse,
			int16_t *a_pLevel = NULL);
		// Remove the given node from the skip list.  Assume a_pTraverse
		// is set up.
		// Returns the node that got removed, and optionally backpatches
		// its level.

	Node *GetNewNode (int16_t a_nForwardPointers);
		// Allocate a new node with the given number of forward
		// pointers.  Returns NULL if something goes wrong.

	Node *GetNewNodeOfRandomLevel (int16_t &a_rnLevel);
		// Allocate a new node with a random number of forward pointers.
		// Returns NULL if something goes wrong; otherwise, a_rnLevel
		// gets backpatched with the number of levels.

	void DeleteNode (int16_t a_nForwardPointers, Node *a_pToDelete);
		// Delete a node.

	void DeallocateNode (int16_t a_nForwardPointers, Node *a_pToDelete);
		// Deallocate a node's storage.

	int16_t Rand (void);
		// Get another random number.

	#ifdef DEBUG_SKIPLIST
	bool m_bDebug;
	#endif // DEBUG_SKIPLIST
		// true if the invariant should be checked.
};



// The default node allocator.  Allocates 64K at a time.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Allocator_t
	SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::sm_oNodeAllocator (65536);



// Default constructor.  Must be followed by Init().
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SkipList (const PRED &a_rPred,
		Allocator_t &a_rAlloc)
	: m_rNodeAllocator (a_rAlloc), m_oPred (a_rPred)
{
	// Set up some defaults.
	m_bAllowDuplicates = false;
	m_nHeaderLevel = 0;
	m_pHeader = NULL;
	m_pSearchFinger = NULL;
	m_pSearchFinger2 = NULL;
	m_nItems = 0;
	m_nRandSeed = 0;
	m_bAllocateInternalNodesFromAllocator = false;
#ifdef DEBUG_SKIPLIST
	m_bDebug = false;

	// Make sure we're intact.
	Invariant();
#endif // DEBUG_SKIPLIST
}



// Constructor.  Specify whether or not duplicates are allowed, and
// provide a random number seed.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SkipList (Status_t &a_reStatus,
		bool a_bAllowDuplicates, const InitParams &a_rInitParams,
		const PRED &a_rPred, Allocator_t &a_rAlloc)
	: m_rNodeAllocator (a_rAlloc), m_oPred (a_rPred)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Set up some defaults.
	m_bAllowDuplicates = false;
	m_nHeaderLevel = 0;
	m_pHeader = NULL;
	m_pSearchFinger = NULL;
	m_pSearchFinger2 = NULL;
	m_nItems = 0;
	m_nRandSeed = 0;
	m_bAllocateInternalNodesFromAllocator = false;
	#ifdef DEBUG_SKIPLIST
	m_bDebug = false;
	#endif // DEBUG_SKIPLIST

	// Init() does all the work.
	Init (a_reStatus, a_bAllowDuplicates, a_rInitParams);
}



// Construction method.  Specify whether or not duplicates are allowed,
// and provide a random number seed.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Init (Status_t &a_reStatus,
	bool a_bAllowDuplicates, const InitParams &a_rInitParams)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we haven't been initialized already.
	assert (m_pHeader == NULL);

	// Fill in the blanks.
	m_bAllowDuplicates = a_bAllowDuplicates;
	m_nHeaderLevel = 0;
	m_nItems = 0;
	m_nRandSeed = a_rInitParams.m_nRandSeed;
	m_bAllocateInternalNodesFromAllocator
		= a_rInitParams.m_bAllocateInternalNodesFromAllocator;

	// Allocate our necessary nodes.
	if (m_bAllocateInternalNodesFromAllocator)
	{
		m_pHeader = GetNewNode (HEADERCHUNK);
		m_pSearchFinger = GetNewNode (HEADERCHUNK);
		m_pSearchFinger2 = GetNewNode (HEADERCHUNK);
	}
	else
	{
		// Do this from system memory, instead of the supplied allocator.
		// Otherwise, these "permanent" allocations won't allow the
		// allocator to purge itself during the lifetime of this skip-list.
		size_t nHeaderBytes = (size_t)(&(((Node *)0x80000000)
			->m_apForward[HEADERCHUNK])) - 0x80000000;
		m_pHeader = (Node *) malloc (nHeaderBytes);
		m_pSearchFinger = (Node *) malloc (nHeaderBytes);
		m_pSearchFinger2 = (Node *) malloc (nHeaderBytes);
	}
	if (m_pHeader == NULL || m_pSearchFinger == NULL
		|| m_pSearchFinger2 == NULL)
	{
		if (m_bAllocateInternalNodesFromAllocator)
		{
			if (m_pSearchFinger2 != NULL)
				DeallocateNode (HEADERCHUNK, m_pSearchFinger2);
			if (m_pSearchFinger != NULL)
				DeallocateNode (HEADERCHUNK, m_pSearchFinger);
			if (m_pHeader != NULL)
				DeallocateNode (HEADERCHUNK, m_pHeader);
		}
		else
		{
			free (m_pSearchFinger2);
			free (m_pSearchFinger);
			free (m_pHeader);
		}
		m_pHeader = NULL;
		m_pSearchFinger = NULL;
		m_pSearchFinger2 = NULL;
		a_reStatus = g_kOutOfMemory;
		return;
	}

	// Set up our nodes.
	//
	// The way we work, incrementing End() gets you Begin(), and
	// decrementing Begin() gets you End().  We implement that by using
	// our header node as a NULL-like sentinel node.
	#ifdef DEBUG_SKIPLIST
	m_pHeader->m_nLevel = HEADERCHUNK;
	m_pSearchFinger->m_nLevel = HEADERCHUNK;
	m_pSearchFinger2->m_nLevel = HEADERCHUNK;
	#endif // DEBUG_SKIPLIST
	new ((void *)(&(m_pHeader->m_oValue))) VALUE;
	new ((void *)(&(m_pSearchFinger->m_oValue))) VALUE;
	new ((void *)(&(m_pSearchFinger2->m_oValue))) VALUE;
	m_pHeader->m_pBackward = m_pHeader;
	m_pSearchFinger->m_pBackward = m_pHeader;
	m_pSearchFinger2->m_pBackward = m_pHeader;
	for (int16_t nI = 0; nI < HEADERCHUNK; nI++)
	{
		m_pHeader->m_apForward[nI] = m_pHeader;
		m_pSearchFinger->m_apForward[nI] = m_pHeader;
		m_pSearchFinger2->m_apForward[nI] = m_pHeader;
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST
}



// Destructor.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::~SkipList (void)
{
	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// If we have anything to delete, delete it.
	if (m_pHeader != NULL)
	{
		// Delete everything in the list.
		Erase (Begin(), End());

		// Free up our extra nodes.
		m_pSearchFinger2->m_oValue.~VALUE();
		m_pSearchFinger->m_oValue.~VALUE();
		m_pHeader->m_oValue.~VALUE();
		if (m_bAllocateInternalNodesFromAllocator)
		{
			DeallocateNode (HEADERCHUNK, m_pSearchFinger2);
			DeallocateNode (HEADERCHUNK, m_pSearchFinger);
			DeallocateNode (HEADERCHUNK, m_pHeader);
		}
		else
		{
			free (m_pSearchFinger2);
			free (m_pSearchFinger);
			free (m_pHeader);
		}
	}
}



#ifdef DEBUG_SKIPLIST

// Set whether to run the skip-list invariant before and after methods.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SetDebug (bool a_bDebug)
{
	// Easy enough.
	m_bDebug = a_bDebug;
}

#endif // DEBUG_SKIPLIST



// Insert an item into the list.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::InsertResult
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Insert (Status_t &a_reStatus,
	const VALUE &a_rValue)
{
	Node *pNewNode;
		// The new node being inserted.
	int16_t nNewLevel;
		// The level of the new node.
	InsertResult oInsertResult;
		// The result of the insertion.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Find where to put this key.  (Put equal keys at the upper bound.)
	SearchUpper (m_oKeyFn (a_rValue), m_pSearchFinger);

	// Insert this node if there's no duplicate already in here.
	if (m_bAllowDuplicates
	|| (pNewNode = m_pSearchFinger->m_apForward[0],
		pNewNode == m_pHeader
		|| m_oPred (m_oKeyFn (pNewNode->m_oValue),
			m_oKeyFn (a_rValue))))
	{
		// Generate a node of random level.
		pNewNode = GetNewNodeOfRandomLevel (nNewLevel);
		if (pNewNode == NULL)
		{
			// We couldn't allocate space.
			a_reStatus = g_kOutOfMemory;
			oInsertResult.m_itPosition = End();
			oInsertResult.m_bInserted = false;
			return oInsertResult;
		}

		// Install the value.
		new ((void *)(&(pNewNode->m_oValue))) VALUE (a_rValue);

		// Put it into the skip list here.
		InsertNode (pNewNode, nNewLevel, m_pSearchFinger);

		// Let them know where we put it, and that we did insert it.
		oInsertResult.m_itPosition = Iterator (pNewNode);
		oInsertResult.m_bInserted = true;
	}
	else
	{
		// We didn't insert it.  Show them the equal item.
		oInsertResult.m_itPosition = Iterator (pNewNode);
		oInsertResult.m_bInserted = false;
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Let them know what happened.
	return oInsertResult;
}



// Insert an item into the list, at this exact location, if it's safe.
// Returns where it was really inserted.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Iterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Insert (Status_t &a_reStatus,
	Iterator a_oPosition, const VALUE &a_rValue)
{
	const KEY &rKey = m_oKeyFn (a_rValue);
		// The key of what they're inserting.
	Node *pNewNode;
		// The node we're inserting.
	int16_t nNewLevel;
		// The level it's at.
	Iterator oHere;
		// Where it gets inserted.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// According to STL semantics, the insertion point must immediately
	// follow a_oPosition.  To fit more naturally with skip-list
	// semantics, the insertion point needs to precede a_oPosition.
	if (a_oPosition != End())
		++a_oPosition;

	// The new item has to fit where this iterator points, and we must
	// be able to find this iterator's node in the list.
	Iterator oBefore = a_oPosition; --oBefore;
	if ((a_oPosition.m_pNode != m_pHeader
		&& m_oPred (m_oKeyFn (a_oPosition.m_pNode->m_oValue), rKey))
	|| (oBefore.m_pNode != m_pHeader
		&& m_oPred (rKey, m_oKeyFn (oBefore.m_pNode->m_oValue)))
	|| !SearchExact (a_oPosition.m_pNode, m_pSearchFinger))
	{
		// Don't use their iterator.
		oHere = Insert (a_reStatus, a_rValue).m_itPosition;
	}

	// Insert this node if we always insert, or if there's no duplicate
	// already.
	else if (m_bAllowDuplicates
	|| (pNewNode = m_pSearchFinger->m_apForward[0]->m_apForward[0],
		pNewNode == m_pHeader
		|| m_oPred (rKey, m_oKeyFn (pNewNode->m_oValue))))
	{
		// Generate a node of random level.
		pNewNode = GetNewNodeOfRandomLevel (nNewLevel);
		if (pNewNode == NULL)
		{
			// We couldn't allocate space.
			a_reStatus = g_kOutOfMemory;
			oHere = End();
		}
		else
		{
			// Install the value.
			new ((void *)(&(pNewNode->m_oValue))) VALUE (a_rValue);

			// Put it into the skip list here.
			InsertNode (pNewNode, nNewLevel, m_pSearchFinger);

			// Let them know where we put it.
			oHere = Iterator (pNewNode);
		}
	}

	// We didn't insert it.  Show them the equal item.
	else
		oHere = Iterator (pNewNode);

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Let our caller know what happened.
	return oHere;
}



// Insert a range of items from another skip-list.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Insert (Status_t &a_reStatus,
	ConstIterator a_oFirst, ConstIterator a_oLast)
{
	ConstIterator oHere;
		// The next item to insert.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Try to insert every item they gave us.
	for (oHere = a_oFirst; oHere != a_oLast; oHere++)
	{
		Insert (a_reStatus, *oHere);
		if (a_reStatus != g_kNoError)
		{
			// BUG: this is messy.  If we can't insert the entire range,
			// we shouldn't insert any of it.  Fix this by preallocating
			// all necessary nodes.
			return;
		}
	}
}



// Erase the item here.  Return the item following the one removed.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Iterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Erase (Iterator a_oHere)
{
	Node *pToRemove;
	int16_t nLevel;
		// The node being deleted, and its level.

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Don't let them erase End().
	if (a_oHere.m_pNode == m_pHeader)
		return Begin();

	// If this iterator does not point to a current node, leave.
	// (This check also sets up the search finger for RemoveNode().)
	if (!SearchExact (a_oHere.m_pNode, m_pSearchFinger))
		return Begin();

	// Erase it.
	pToRemove = RemoveNode (m_pSearchFinger->m_apForward[0]
		->m_apForward[0], m_pSearchFinger, &nLevel);
	DeleteNode (nLevel, pToRemove);

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Return an iterator that points past the deleted node.
	return Iterator (m_pSearchFinger->m_apForward[0]->m_apForward[0]);
}



// Erase a range of items in this list.  Return the item following the
// last one removed.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Iterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Erase (Iterator a_oFirst,
	Iterator a_oLast)
{
	int16_t nI;
		// Used to loop through things.
	Node *pNode, *pToRemove;
		// Nodes we're examining and deleting.
	int16_t nLevel;
		// The level of each removed node.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Make sure these iterators are in the right order.
	assert (a_oFirst == Begin()
		|| a_oLast == End()
		|| !m_oPred (m_oKeyFn (a_oLast.m_pNode->m_oValue),
			m_oKeyFn (a_oFirst.m_pNode->m_oValue)));

	// If the first item to be removed is the end of the list, then
	// nothing needs to be done.
	if (a_oFirst == End())
		return End();

	// A skip-list's range-delete doesn't involve a rebalancing for
	// every node deleted, like a tree structure's range-delete would.
	// Here, we just generate search fingers for each end of the
	// range, and fix the forward/backward pointers in one shot.
	// We still have to loop through the deleted range in order to
	// delete the nodes, but that's nowhere near the cost of a
	// rebalance per node.  Spiffy, huh?

	// First, search for the beginning of the range to delete.
	// Abort if we can't find it.
	if (!SearchExact (a_oFirst.m_pNode, m_pSearchFinger))
		return End();

	// Use that as a base for the second search.
	for (nI = 0; nI < m_nHeaderLevel; nI++)
		m_pSearchFinger2->m_apForward[nI]
			= m_pSearchFinger->m_apForward[nI];

	// Now search for the end of the range to delete.
	// Abort if we can't find it.
	if (!SearchExact (a_oLast.m_pNode, m_pSearchFinger2))
		return End();

	// Loop through the removed nodes, destroy the values inside,
	// and deallocate the nodes.
	pNode = a_oFirst.m_pNode;
	while (pNode != a_oLast.m_pNode)
	{
		// Get the node to destroy.
		pToRemove = pNode;
		pNode = pNode->m_apForward[0];

		// Destroy it.
		pToRemove = RemoveNode (pToRemove, m_pSearchFinger, &nLevel);
		DeleteNode (nLevel, pToRemove);
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Return the end of the removed range.
	return a_oLast;
}



// Empty the list.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Clear (void)
{
	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Easy enough.
	Erase (Begin(), End());

	// Since there are no nodes left, it's OK to "fix the dice" at
	// zero again.
	m_nHeaderLevel = 0;
}



// Remove an item from another skip list and insert it into ourselves.
// Just like an Erase() followed by an Insert(), except that there's no
// possibility of the operation failing.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::InsertResult
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Move
	(SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther, Iterator a_oHere)
{
	Node *pNode;
		// The node we're moving.
	int16_t nLevel;
		// The level of the node we're moving.
	InsertResult oInsertResult;
		// The result of the insertion.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Make sure the skip-lists can move items between themselves.
	assert (CanMove (a_rOther));

	// Don't let them move End().
	assert (a_oHere.m_pNode != a_rOther.m_pHeader);

	// Get a reference to the key of the value being moved.
	const KEY &rKey = m_oKeyFn (a_oHere.m_pNode->m_oValue);

	// Find where to put this key.  (Put equal keys at the upper bound.)
	SearchUpper (rKey, m_pSearchFinger);

	// Move this node if there's no duplicate already in here.
	if (m_bAllowDuplicates
	|| (pNode = m_pSearchFinger->m_apForward[0],
		pNode == m_pHeader
			|| m_oPred (m_oKeyFn (pNode->m_oValue), rKey)))
	{
		bool bFound;
		// true if this node was found in the other skip list.

		// Make sure the other list is intact.
		#ifdef DEBUG_SKIPLIST
		a_rOther.Invariant();
		#endif // DEBUG_SKIPLIST

		// Find this node in the other skip list.
		bFound = a_rOther.SearchExact (a_oHere.m_pNode,
			a_rOther.m_pSearchFinger);

		// Make sure this iterator points to a current node.
		assert (bFound);

		// Remove it from that skip list.
		pNode = a_rOther.RemoveNode (a_oHere.m_pNode,
			a_rOther.m_pSearchFinger, &nLevel);

		// If the node we're about to add is a higher level than any
		// node we've seen so far, keep track of the new maximum.
		if (m_nHeaderLevel < nLevel)
			m_nHeaderLevel = nLevel;

		// Put it into the skip list here.
		InsertNode (pNode, nLevel, m_pSearchFinger);

		// Let them know where we put it, and that we did insert it.
		oInsertResult.m_itPosition = Iterator (pNode);
		oInsertResult.m_bInserted = true;
	}
	else
	{
		// We didn't insert it.  Show them the equal item.
		oInsertResult.m_itPosition = Iterator (pNode);
		oInsertResult.m_bInserted = false;
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Let them know what happened.
	return oInsertResult;
}



// Remove a range of items from another skip-list and insert them
// into ourselves.
// Just like an Erase() followed by an Insert(), except that there's no
// possibility of the operation failing.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Move
	(SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther, Iterator a_oFirst,
	Iterator a_oLast)
{
	Iterator oHere, oNext;
		// The item we're moving.

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Loop through all the items, move each one.
	oHere = a_oFirst;
	while (oHere != a_oLast)
	{
		// Keep track of the next item to move.
		oNext = oHere;
		oNext++;

		// Move this item.
		Move (a_rOther, oHere);

		// Loop back and move the next one.
		oHere = oNext;
	}
}



// Move all items from the other skip-list to ourself.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Move
	(SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther)
{
	Node *p;
		// Used to swap structures.

	// Make sure we're all intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	a_rOther.Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure the skip-lists can move items between themselves.
	assert (CanMove (a_rOther));

	// Make sure we're empty.
	assert (m_nItems == 0);

	// Swap the header & search fingers.
	p = m_pHeader;
	m_pHeader = a_rOther.m_pHeader;
	a_rOther.m_pHeader = p;
	p = m_pSearchFinger;
	m_pSearchFinger = a_rOther.m_pSearchFinger;
	a_rOther.m_pSearchFinger = p;
	p = m_pSearchFinger2;
	m_pSearchFinger2 = a_rOther.m_pSearchFinger2;
	a_rOther.m_pSearchFinger2 = p;

	// Use their header level, if it's bigger than ours.
	if (m_nHeaderLevel < a_rOther.m_nHeaderLevel)
		m_nHeaderLevel = a_rOther.m_nHeaderLevel;
	a_rOther.m_nHeaderLevel = 0;

	// Now we have all their items.
	m_nItems = a_rOther.m_nItems;
	a_rOther.m_nItems = 0;

	// Make sure we're all intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	a_rOther.Invariant();
	#endif // DEBUG_SKIPLIST
}



// Returns true if the two skip lists can move items between
// each other.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
bool
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::CanMove
	(const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther) const
{
	// They can if they have the same allocator.
	return (&m_rNodeAllocator == &a_rOther.m_rNodeAllocator);
}



// Assign the contents of the other skip list to ourselves.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Assign (Status_t &a_reStatus,
	const SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC> &a_rOther)
{
	Node *pBegin, *pEnd;
		// Where we're looking in the other list.
	Node *pHere;
		// Where we're looking in our list.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Get the range of items from the other list.
	pBegin = a_rOther.Begin().m_pNode;
	pEnd = a_rOther.End().m_pNode;

	// First, if we have more nodes than they do, erase enough of ours
	// so that we have the same amount.
	if (Size() > a_rOther.Size())
	{
		// Calculate the number of nodes to delete.
		int32_t nToDelete = Size() - a_rOther.Size();

		// Move forward that many items.
		Iterator oToDelete = Begin();
		while (--nToDelete >= 0)
			oToDelete++;

		// Delete those items.
		Erase (Begin(), oToDelete);
	}

	// Start modifying our items here.
	pHere = Begin().m_pNode;

	// Next, insert as many items as we can, using the nodes already
	// allocated in the list.
	while (pHere != m_pHeader && pBegin != pEnd)
	{
		// Store the new item over the old one.
		pHere->m_oValue = pBegin->m_oValue;

		// Move forward.
		pHere = pHere->m_apForward[0];
		pBegin = pBegin->m_apForward[0];
	}

	// If we ran out of nodes, insert the rest in the normal way.
	if (pBegin != pEnd)
		Insert (a_reStatus, ConstIterator (pBegin),
			ConstIterator (pEnd));

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST
}



// Find the given item in the list.  Returns End() if not found.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Iterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Find (const KEY &a_rKey)
{
	Iterator oHere;
		// What we found.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Look for the item.
	oHere = LowerBound (a_rKey);

	// LowerBound() returns the first item >= the key.  So if this item
	// is greater than what they were asking for, that means we didn't
	// find it.
	if (oHere == End()
	|| m_oPred (a_rKey, m_oKeyFn (oHere.m_pNode->m_oValue)))
		return End();
	else
		return oHere;
}



// Find the given item in the list.  Returns End() if not found.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::ConstIterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Find (const KEY &a_rKey) const
{
	ConstIterator oHere;
		// What we found.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Look for the item.
	oHere = LowerBound (a_rKey);

	// LowerBound() returns the first item >= the key.  So if this item
	// is greater than what they were asking for, that means we didn't
	// find it.
	if (oHere == End()
	|| m_oPred (a_rKey, m_oKeyFn (oHere.m_pNode->m_oValue)))
		return End();
	else
		return oHere;
}



// Return the position of the first item that's >= the key.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Iterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::LowerBound (const KEY &a_rKey)
{
	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Search for the first item >= the key.
	SearchLower (a_rKey, m_pSearchFinger);

	// Return what we found.
	return Iterator (m_pSearchFinger->m_apForward[0]->m_apForward[0]);
}



// Return the position of the first item that's >= the key.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::ConstIterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::LowerBound (const KEY &a_rKey)
	const
{
	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Search for the first item >= the key.
	SearchLower (a_rKey, m_pSearchFinger);

	// Return what we found.
	return ConstIterator (m_pSearchFinger->m_apForward[0]
		->m_apForward[0]);
}



// Return the position of the first item that's > the key.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Iterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::UpperBound (const KEY &a_rKey)
{
	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Search for the first item > the key.
	SearchUpper (a_rKey, m_pSearchFinger);

	// Return what we found.
	return Iterator (m_pSearchFinger->m_apForward[0]->m_apForward[0]);
}



// Return the position of the first item that's > the key.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::ConstIterator
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::UpperBound (const KEY &a_rKey)
	const
{
	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Search for the first item > the key.
	SearchUpper (a_rKey, m_pSearchFinger);

	// Return what we found.
	return ConstIterator (m_pSearchFinger->m_apForward[0]
		->m_apForward[0]);
}



// Return the size of the largest possible skip-list node.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
size_t
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::GetSizeOfLargestNode (void) const
{
	// Easy enough.
	return (size_t)(&(((Node *)0x80000000)->m_apForward[HEADERCHUNK]))
		- 0x80000000;
}



// Search for an item greater than or equal to a_rKey.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SearchLower (const KEY &a_rKey,
	Node *a_pTraverse) const
{
	Node *pCurrentNode;
		// Where we're searching.
	int16_t nCurrentLevel;
		// The level we've searched so far.
	Node *pLastFinger;
		// A node that we know is < the key.
	Node *pAlreadyChecked;
		// A node that we know is >= the key.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// See how much of the previous search we can reuse.
	pCurrentNode = m_pHeader;
	pLastFinger = m_pHeader;
	pAlreadyChecked = m_pHeader;
	nCurrentLevel = m_nHeaderLevel;
	while (--nCurrentLevel >= 0)
	{
		Node *pCurrentFinger;
			// What's at this level.

		// See what's at this level.
		pCurrentFinger = a_pTraverse->m_apForward[nCurrentLevel];

		// If this is what we found before, OR if there's nothing here,
		// OR we haven't gone past what we're trying to find, then
		// there's a chance we can reuse the search at this level.
		if (pCurrentFinger == pLastFinger
		|| pCurrentFinger == m_pHeader
		|| m_oPred (m_oKeyFn (pCurrentFinger->m_oValue), a_rKey))
		{
			// Look at the item past the current search finger.
			Node *pNextFinger
				= pCurrentFinger->m_apForward[nCurrentLevel];

			// If there's nothing here, OR what's here is >= what we're
			// looking for, then we can reuse all of this level's
			// search.
			if (pNextFinger == m_pHeader
			|| pNextFinger == pAlreadyChecked
			|| !m_oPred (m_oKeyFn (pNextFinger->m_oValue), a_rKey))
			{
				// We can reuse all of this level's search.
				pCurrentNode = pCurrentFinger;
				pLastFinger = pCurrentFinger;
				pAlreadyChecked = pNextFinger;
			}
			else
			{
				// We can reuse this level's search, but we have to
				// start looking here.
				pCurrentNode = pCurrentFinger;
				nCurrentLevel++;
				break;
			}
		}
		else
		{
			// We can't reuse this level.  Redo it.
			nCurrentLevel++;
			break;
		}
	}

	// Now search for the item in the list.
	while (--nCurrentLevel >= 0)
	{
		Node *pNextNode;
		while (pNextNode = pCurrentNode->m_apForward[nCurrentLevel],
			pNextNode != pAlreadyChecked && pNextNode != m_pHeader
			&& m_oPred (m_oKeyFn (pNextNode->m_oValue), a_rKey))
		{
			pCurrentNode = pNextNode;
		}
		pAlreadyChecked = pCurrentNode->m_apForward[nCurrentLevel];
		a_pTraverse->m_apForward[nCurrentLevel] = pCurrentNode;
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST
}



// Search for this exact item.  Returns true if it's found.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
bool
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SearchExact (Node *a_pKey,
	Node *a_pTraverse) const
{
	Node *pFound;
		// What we found.
	int16_t nI;
		// Used to iterate through levels.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// If they passed us End(), just search to the end and succeed.
	if (a_pKey == m_pHeader)
	{
		SearchEnd (a_pTraverse);
		return true;
	}

	// Find the lower bound.
	SearchLower (m_oKeyFn (a_pKey->m_oValue), a_pTraverse);

	// Run through the equal range, look for this exact item.
	for (pFound = a_pTraverse->m_apForward[0]->m_apForward[0];
		 pFound != m_pHeader && pFound != a_pKey;
		 pFound = pFound->m_apForward[0])
	{
		// Advance the search finger.
		for (nI = 0;
			 nI < m_nHeaderLevel
			 	&& a_pTraverse->m_apForward[nI]->m_apForward[nI]
					== pFound;
			 nI++)
		{
			a_pTraverse->m_apForward[nI] = pFound;
		}
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Let them know if we succeeded.
	return (pFound == a_pKey);
}



// Search for an item greater than a_rKey.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SearchUpper (const KEY &a_rKey,
	Node *a_pTraverse) const
{
	Node *pCurrentNode;
		// Where we're searching.
	int16_t nCurrentLevel;
		// The level we've searched so far.
	Node *pLastFinger;
		// A node that we know is <= the key.
	Node *pAlreadyChecked;
		// A node that we know is > the key.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// See how much of the previous search we can reuse.
	pCurrentNode = m_pHeader;
	pLastFinger = m_pHeader;
	pAlreadyChecked = m_pHeader;
	nCurrentLevel = m_nHeaderLevel;
	while (--nCurrentLevel >= 0)
	{
		Node *pCurrentFinger;
			// What's at this level.

		// See what's at this level.
		pCurrentFinger = a_pTraverse->m_apForward[nCurrentLevel];

		// If this is what we found before, OR if there's nothing here,
		// OR we haven't gone past what we're trying to find, then
		// there's a chance we can reuse the search at this level.
		if (pCurrentFinger == pLastFinger
		|| pCurrentFinger == m_pHeader
		|| !m_oPred (a_rKey, m_oKeyFn (pCurrentFinger->m_oValue)))
		{
			// Look at the item past the current search finger.
			Node *pNextFinger
				= pCurrentFinger->m_apForward[nCurrentLevel];

			// If there's nothing here, OR what's here is > what we're
			// looking for, then we can reuse all of this level's
			// search.
			if (pNextFinger == m_pHeader
			|| pNextFinger == pAlreadyChecked
			|| m_oPred (a_rKey, m_oKeyFn (pNextFinger->m_oValue)))
			{
				// We can reuse all of this level's search.
				pCurrentNode = pCurrentFinger;
				pLastFinger = pCurrentFinger;
				pAlreadyChecked = pNextFinger;
			}
			else
			{
				// We can reuse this level's search, but we have to
				// start looking here.
				pCurrentNode = pCurrentFinger;
				nCurrentLevel++;
				break;
			}
		}
		else
		{
			// We can't reuse this level.  Redo it.
			nCurrentLevel++;
			break;
		}
	}

	// Now search for the item in the list.
	while (--nCurrentLevel >= 0)
	{
		Node *pNextNode;
		while (pNextNode = pCurrentNode->m_apForward[nCurrentLevel],
			pNextNode != pAlreadyChecked && pNextNode != m_pHeader
			&& !m_oPred (a_rKey, m_oKeyFn (pNextNode->m_oValue)))
		{
			pCurrentNode = pNextNode;
		}
		pAlreadyChecked = pCurrentNode->m_apForward[nCurrentLevel];
		a_pTraverse->m_apForward[nCurrentLevel] = pCurrentNode;
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST
}



// Point to the end of the list.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::SearchEnd (Node *a_pTraverse)
	const
{
	// Make sure we have been initialized properly.
	assert (m_pHeader != NULL);

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Point every level of the traverse array to the last node before
	// the end.
	Node *pLastNode = a_pTraverse->m_apForward[m_nHeaderLevel - 1];
	int16_t nCurrentLevel = m_nHeaderLevel;
	while (--nCurrentLevel >= 0)
	{
		Node *pNextNode;
		while (pNextNode = pLastNode->m_apForward[nCurrentLevel],
				pNextNode != m_pHeader)
		{
			pLastNode = pNextNode;
		}
		a_pTraverse->m_apForward[nCurrentLevel] = pLastNode;
	}

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST
}



// Insert a new node into the skip list.  Assume a_pTraverse is
// set up.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::InsertNode (Node *a_pNewNode,
	int16_t a_nNewLevel, Node *a_pTraverse)
{
	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Set up the backward link of the new node.
	a_pNewNode->m_pBackward = a_pTraverse->m_apForward[0];

	// Modify all levels of pointers.
	while (--a_nNewLevel >= 0)
	{
		// Insert ourselves where the traverse array points.
		a_pNewNode->m_apForward[a_nNewLevel]
			= a_pTraverse->m_apForward[a_nNewLevel]
				->m_apForward[a_nNewLevel];
		a_pTraverse->m_apForward[a_nNewLevel]->m_apForward[a_nNewLevel]
			= a_pNewNode;

		// Point the traverse array at the new node.
		a_pTraverse->m_apForward[a_nNewLevel] = a_pNewNode;
	}

	// Set up the backward link of the node after the new one.
	a_pNewNode->m_apForward[0]->m_pBackward = a_pNewNode;

	// That's one more node in the list.
	m_nItems++;

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST
}



// Remove the given node from the skip list.  Assume a_pTraverse is
// set up.
// Returns the node that got removed, and optionally backpatches its
// level.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Node *
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::RemoveNode (Node *a_pToRemove,
	Node *a_pTraverse, int16_t *a_pLevel)
{
	int16_t nI;
		// Used to loop through skip-list levels.
	Node *pCurrentNode;
		// A node that's pointing to the node we're removing.

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Make sure this traversal array is set up to remove this node.
	assert (a_pTraverse->m_apForward[0]->m_apForward[0] == a_pToRemove);

	// Remove this node from all levels where it appears.
	for (nI = 0;
		 nI < m_nHeaderLevel
		 && (pCurrentNode = a_pTraverse->m_apForward[nI],
			pCurrentNode->m_apForward[nI] == a_pToRemove);
		 nI++)
	{
		pCurrentNode->m_apForward[nI] = a_pToRemove->m_apForward[nI];
	}

	// Make sure the node had as many levels as we expected.
	#ifdef DEBUG_SKIPLIST
	assert (nI == a_pToRemove->m_nLevel);
	#endif // DEBUG_SKIPLIST

	// Remove this node from the back-links.
	a_pTraverse->m_apForward[0]->m_apForward[0]->m_pBackward
		= a_pTraverse->m_apForward[0];

	// That's one less item in the list.
	m_nItems--;

	// Give them the level of this node, if they asked for it.
	if (a_pLevel != NULL)
		*a_pLevel = nI;

	// Make sure we're intact.
	#ifdef DEBUG_SKIPLIST
	Invariant();
	#endif // DEBUG_SKIPLIST

	// Return the node we removed.
	return a_pToRemove;
}



// Allocate a new node with the given number of forward pointers.
// Returns NULL if something goes wrong.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Node *
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::GetNewNode
	(int16_t a_nForwardPointers)
{
	size_t nBytes;
		// The number of bytes this node needs.
	Node *pNode;
		// The node we allocate.

	// Calculate the number of bytes it takes to represent a node with
	// this many forward pointers.
	nBytes = (size_t)(&(((Node *)0x80000000)
		->m_apForward[a_nForwardPointers])) - 0x80000000;

	// Allocate space.
	pNode = (Node *) m_rNodeAllocator.Allocate (a_nForwardPointers - 1,
		nBytes);
	if (pNode == NULL)
		return NULL;

	// Set the number of forward pointers.
	#ifdef DEBUG_SKIPLIST
	pNode->m_nLevel = a_nForwardPointers;
	#endif // DEBUG_SKIPLIST

	// Return what we allocated.
	return pNode;
}



// Allocate a new node with a random number of forward pointers.
// Returns NULL if something goes wrong; otherwise, a_rnLevel gets
// backpatched with the number of levels.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
typename SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Node *
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::GetNewNodeOfRandomLevel
	(int16_t &a_rnLevel)
{
	// Fix the dice (as described in the skip-list paper).
	int16_t nHighestLevel = (m_nHeaderLevel == HEADERCHUNK)
		? HEADERCHUNK : (m_nHeaderLevel + 1);

	// Now generate a random level for the node.  (12055 / 32768 equals
	// our skip list's p, which is 1/e.)
	int16_t nNewLevel = 1;
	while (nNewLevel < nHighestLevel && Rand() < 12055)
		nNewLevel++;

	// Keep track of the new maximum.
	if (nNewLevel == nHighestLevel)
		m_nHeaderLevel = nHighestLevel;

	// Allocate a node of this size.
	a_rnLevel = nNewLevel;
	return GetNewNode (nNewLevel);
}



// Delete a node.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::DeleteNode
	(int16_t a_nForwardPointers, Node *a_pToDelete)
{
	// Make sure they gave us a node to delete.
	assert (a_pToDelete != NULL);

	// Delete the value we contain.
	a_pToDelete->m_oValue.~VALUE();

	// Deallocate the node's storage.
	DeallocateNode (a_nForwardPointers, a_pToDelete);
}



// Deallocate a node's storage.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::DeallocateNode
	(int16_t a_nForwardPointers, Node *a_pToDelete)
{
	// Make sure they gave us a node to deallocate.
	assert (a_pToDelete != NULL);

	// Calculate the number of bytes it takes to represent a node with
	// this many forward pointers.
	size_t nBytes = (size_t)(&(((Node *)0x80000000)
		->m_apForward[a_nForwardPointers])) - 0x80000000;

	// Return the memory to our node allocator.
	m_rNodeAllocator.Deallocate (a_nForwardPointers - 1,
		nBytes, (void *)a_pToDelete);
}



// Get another random number.
template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
int16_t
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Rand (void)
{
	// I'd use the normal rand() but it's WAAAY too slow.
	// Feel free to adjust this routine all you want.
	m_nRandSeed = m_nRandSeed * 42989 + 17891;
	return int16_t (m_nRandSeed & 32767);
}



#ifdef DEBUG_SKIPLIST

template <class KEY, class VALUE, class KEYFN, class PRED,
	int HC, class ALLOC>
void
SkipList<KEY,VALUE,KEYFN,PRED,HC,ALLOC>::Invariant (void) const
{
	int32_t nI;
		// Used to loop through things.
	Node *pSearchFinger;
		// Used to verify the integrity of the forward pointers.
	Node *pCurrentNode;
		// The node whose integrity we are presently verifying.
	Node *pPreviousNode;
		// The node before the current one.
	uint32_t nItems;
		// The number of items we found.

	// Only check the invariant if they requested we do.
	if (!m_bDebug)
		return;

	// If the skip list is not fully initialized, we have less to check.
	if (m_pHeader == NULL)
	{
		assert (m_nHeaderLevel == 0);
		assert (m_pSearchFinger == NULL);
		assert (m_pSearchFinger2 == NULL);
		assert (m_nItems == 0);
		return;
	}

	// Make sure the skip list level isn't bigger than the compiled-in
	// maximum.
	assert (m_nHeaderLevel <= HEADERCHUNK);

	// Calculate the number of bytes it takes to represent a node with
	// this many forward pointers.
	size_t nBytes = (size_t)(&(((Node *)0x80000000)
		->m_apForward[HEADERCHUNK])) - 0x80000000;

	// We're going to run through the skip list and make sure all the
	// forward pointers are correct.  For that, we need a temporary
	// search finger to keep track of the traversal.
	pSearchFinger = (Node *) alloca (nBytes);
	pSearchFinger->m_nLevel = 0;
	pSearchFinger->m_pBackward = NULL;
	for (nI = 0; nI < HEADERCHUNK; nI++)
		pSearchFinger->m_apForward[nI] = m_pHeader;

	// The backward link for the header node points to the last item in
	// the list.  So find the last item in the list.
	pPreviousNode = m_pHeader;
	while (pPreviousNode->m_apForward[0] != m_pHeader)
		pPreviousNode = pPreviousNode->m_apForward[0];

	// Run through the skip list, make sure the forward pointers and
	// backward pointers are intact.
	pCurrentNode = m_pHeader;
	nItems = 0;
	for (;;)
	{
		int32_t nLevel;
			// The level of the current node.

		// At least one forward pointer of the temporary search finger
		// points to the current node.  The number of forward pointers
		// that point to the current node, must be equal to its assigned
		// level.  So start by calculating what level this node must be.
		nLevel = 0;
		while (nLevel < HEADERCHUNK
				&& pSearchFinger->m_apForward[nLevel] == pCurrentNode)
			nLevel++;

		// Make sure the node has that many forward pointers.
		assert (pCurrentNode->m_nLevel == nLevel);

		// Make sure the backward pointer is intact.
		assert (pCurrentNode->m_pBackward == pPreviousNode);

		// Make sure that the nodes are in proper sorted order.
		if (pPreviousNode != m_pHeader && pCurrentNode != m_pHeader)
		{
			if (m_bAllowDuplicates)
			{
				// The current item has to be >= the previous item.
				assert (!m_oPred (m_oKeyFn (pCurrentNode->m_oValue),
					m_oKeyFn (pPreviousNode->m_oValue)));
			}
			else
			{
				// The current item has to be > the previous item.
				assert (m_oPred (m_oKeyFn (pPreviousNode->m_oValue),
					m_oKeyFn (pCurrentNode->m_oValue)));
			}
		}

		// If we're at the end of the list, stop.
		if (pCurrentNode->m_apForward[0] == m_pHeader)
			break;

		// That's one more item we found.  (We put this here in order to
		// avoid counting the header node as an item.)
		nItems++;

		// Move forward.
		pPreviousNode = pCurrentNode;
		pCurrentNode = pCurrentNode->m_apForward[0];
		for (nI = 0; nI < nLevel; nI++)
			pSearchFinger->m_apForward[nI]
				= pSearchFinger->m_apForward[nI]->m_apForward[nI];
	}

	// Make sure the list has the expected number of items.
	assert (m_nItems == nItems);
}

#endif // DEBUG_SKIPLIST



#endif // __SKIPLIST_H__
