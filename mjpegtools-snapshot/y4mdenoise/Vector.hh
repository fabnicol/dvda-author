#ifndef __VECTOR_H__
#define __VECTOR_H__

// This file (C) 2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

/* A vector class that can be used to implement Set<>. */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include <new>
#include "mjpeg_types.h"
#include "Status_t.h"
#include "VariableSizeAllocator.hh"



// Define this to compile in code to double-check and debug the vector.
#ifndef NDEBUG
	#define DEBUG_VECTOR
#endif // NDEBUG



template <class KEY, class VALUE, class KEYFN, class PRED>
class Vector
{
private:
	// The type of a node in the vector
	struct Node
	{
		VALUE m_oValue;
			// The data held by this node.

		Node() : m_oValue() {}
			// Default constructor.

		explicit Node (const VALUE &a_rValue) : m_oValue (a_rValue) {}
			// Initializing constructor.
	};

	Vector (const Vector<KEY,VALUE,KEYFN,PRED> &a_rOther);
	const Vector<KEY,VALUE,KEYFN,PRED> &operator =
			(const Vector<KEY,VALUE,KEYFN,PRED> &a_rOther);
		// Disallow copying and assignment.

public:
	typedef VariableSizeAllocator Allocator_t;
		// The type of node allocator to use.

	static Allocator_t sm_oNodeAllocator;
		// The default node allocator.

	// A structure that contains all of our initialization parameters.
	struct InitParams
	{
	public:
		uint32_t m_nInitialItems, m_nAdditionalItems;
			// The number of initial/additional items to allocate.

		InitParams() : m_nInitialItems (128u), m_nAdditionalItems (128u) {}
			// Default constructor.  Pull some values out of thin air.

		InitParams (uint32_t a_nInitialItems,
				uint32_t a_nAdditionalItems)
			: m_nInitialItems (a_nInitialItems),
				m_nAdditionalItems (a_nAdditionalItems) {}
			// Initializing constructor.
	};

	Vector (const PRED &a_rPred = PRED(),
			Allocator_t &a_rAlloc = sm_oNodeAllocator);
		// Default constructor.  Must be followed by Init().

	Vector (Status_t &a_reStatus, bool a_bAllowDuplicates,
			const InitParams &a_rInitParams, const PRED &a_rPred = PRED(),
			Allocator_t &a_rAlloc = sm_oNodeAllocator);
		// Constructor.  Specify whether or not duplicates are allowed, and
		// the initial/additional number of items to allocate.

	void Init (Status_t &a_reStatus, bool a_bAllowDuplicates,
			const InitParams &a_rInitParams);
		// Construction method.  Specify whether or not duplicates are
		// allowed, and the initial/additional number of items to allocate.

	virtual ~Vector (void);
		// Destructor.

#ifdef DEBUG_VECTOR

	void Invariant (void) const;
		// Thoroughly analyze the vector for structural integrity.

	void SetDebug (bool a_bDebug);
		// Set whether to run the vector invariant before and after
		// methods.

#endif // DEBUG_VECTOR

	//
	// Iterator classes.
	//

	class Iterator;
	class ConstIterator;
	friend class ConstIterator;
	class ConstIterator
	{
		protected:
			friend class Vector<KEY,VALUE,KEYFN,PRED>;
				// Let Vector access m_pNode and m_nNode.
			Node *m_pNode;
				// The node we represent.
				// Used to make accessing the value simple.
			uint32_t m_nNode;
				// The index of the node we represent.
				// Used to avoid integer-division (which is slow on pretty
				// much every CPU in existence) to convert the node pointer
				// into an index.
			const Vector<KEY,VALUE,KEYFN,PRED> *m_pVector;
				// The vector that contains this node.
		public:
			ConstIterator() { m_pNode = NULL; m_pVector = NULL; }
			ConstIterator (Node *a_pNode, uint32_t a_nNode,
					const Vector<KEY,VALUE,KEYFN,PRED> *a_pVector)
				: m_pNode (a_pNode), m_nNode (a_nNode),
					m_pVector (a_pVector) {}
			ConstIterator (const Iterator &a_rOther)
				: m_pNode (a_rOther.m_pNode), m_nNode (a_rOther.m_nNode),
					m_pVector (a_rOther.m_pVector) {}
			const VALUE &operator*() const
			{
				// Make sure this iterator is associated with a vector.
				assert (m_pVector != NULL);

				// Make sure this iterator points within this vector.
				// (This helps to detect stale iterators.)
				assert (m_pNode >= m_pVector->Begin().m_pNode);
				assert (m_pNode <= m_pVector->Last().m_pNode);

				// Easy enough.
				return m_pNode->m_oValue;
			}
			ConstIterator& operator++()
			{
				// Make sure this iterator is associated with a vector.
				assert (m_pVector != NULL);

				// If they go forward from End(), wrap to the beginning.
				// If there are no items, End() is the only possible value
				// for an iterator.
				if (m_pNode == NULL || m_pVector->Size() == 0u)
				{
					m_pNode = m_pVector->Begin().m_pNode;
					m_nNode = 0u;
				}

				// If they go forward past the last item, then they're
				// at the end of the array.
				else if (m_pNode == m_pVector->Last().m_pNode)
				{
					m_pNode = NULL;
					m_nNode = 0u;
				}

				// Otherwise, move to the next item.
				else
				{
					++m_pNode;
					++m_nNode;
				}

				return *this;
			}
			ConstIterator operator++(int) { ConstIterator oTmp = *this;
				++*this; return oTmp; }
			ConstIterator& operator--()
			{
				// Make sure this iterator is associated with a vector.
				assert (m_pVector != NULL);

				// If they go backward from the first item, then they're
				// at the end of the array.
				// If there are no items, End() is the only possible value
				// for an iterator.
				if (m_pNode == m_pVector->Begin().m_pNode
					|| m_pVector->Size() == 0u)
				{
					m_pNode = NULL;
					m_nNode = 0u;
				}

				// If they go backward from End(), wrap to the last item.
				else if (m_pNode == NULL && m_pVector->Size() > 0u)
				{
					m_pNode = m_pVector->Last().m_pNode;
					if (m_pNode != NULL)
						m_nNode = m_pVector->Size() - 1u;
					else
						m_nNode = 0u;
				}

				// Otherwise, move to the previous item.
				else
				{
					--m_pNode;
					--m_nNode;
				}

				return *this;
			}
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
			Iterator (Node *a_pNode, uint32_t a_nNode,
					Vector *a_pVector)
				: BaseClass (a_pNode, a_nNode, a_pVector) {}
			VALUE &operator*()
			{
				// Make sure this iterator is associated with a vector.
				assert (BaseClass::m_pVector != NULL);

				// Make sure this iterator points within this vector.
				// (This helps to detect stale iterators.)
				assert (BaseClass::m_pNode
					>= BaseClass::m_pVector->Begin().m_pNode);
				assert (BaseClass::m_pNode
					<= BaseClass::m_pVector->Last().m_pNode);

				// Easy enough.
				return BaseClass::m_pNode->m_oValue;
			}
			Iterator& operator++() { ++((BaseClass &)*this);
				return *this; }
			Iterator operator++(int) { Iterator oTmp = *this; ++*this;
				return oTmp; }
			Iterator& operator--() { --((BaseClass &)*this);
				return *this; }
			Iterator operator--(int) { Iterator oTmp = *this; --*this;
				return oTmp; }
			bool operator== (const Iterator &a_rOther) const
				{ return (BaseClass::m_pNode == a_rOther.m_pNode)
					? true : false; }
			bool operator!= (const Iterator &a_rOther) const
				{ return (BaseClass::m_pNode != a_rOther.m_pNode)
					? true : false; }
	};

	//
	// Vector methods.
	//

	Iterator Begin (void)
			{ return Iterator ((m_nItems == 0u) ? NULL : m_pItems,
				0u, this); }
		// Return an iterator to the beginning of the list.
		// If there are no items, that's also the end of the list.

	ConstIterator Begin (void) const
			{ return ConstIterator ((m_nItems == 0u) ? NULL : m_pItems,
				0u, this); }
		// Return an iterator to the beginning of the list.
		// If there are no items, that's also the end of the list.

	Iterator Last (void)
			{ assert (m_nItems > 0u);
				return Iterator (m_pItems + m_nItems - 1u,
					m_nItems - 1u, this); }
		// Return an iterator to the last item in the list.
		// The list must have at least one item in it.

	ConstIterator Last (void) const
			{ assert (m_nItems > 0u);
				return ConstIterator (m_pItems + m_nItems - 1u,
					m_nItems - 1u, this); }
		// Return an iterator to the last item in the list.
		// The list must have at least one item in it.

	Iterator End (void) { return Iterator (NULL, 0u, this); }
		// Return an iterator to the end of the list.

	ConstIterator End (void) const
			{ return ConstIterator (NULL, 0u, this); }
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

	Iterator Insert (Status_t &a_reStatus, Iterator a_itPosition,
			const VALUE &a_rValue);
		// Insert an item into the list, at this exact location, if it's
		// safe.  Returns where it was really inserted.

	void Insert (Status_t &a_reStatus, ConstIterator a_itFirst,
			ConstIterator a_itLast);
		// Insert a range of items from another vector.

	Iterator Erase (Iterator a_itHere);
		// Erase the item here.  Return the item following the one
		// removed.

	Iterator Erase (Iterator a_itFirst, Iterator a_itLast);
		// Erase a range of items in this list.  Return the item
		// following the last one removed.

	void Clear (void);
		// Empty the list.
	
	void Purge (void);
		// Purge all internally-allocated memory.

	void Move (Vector<KEY,VALUE,KEYFN,PRED> &a_rOther);
		// Move all items from the other vector to ourself.
		// The current vector must be empty.
		// This operation cannot fail, i.e. it doesn't allocate memory.

	bool CanMove (const Vector<KEY,VALUE,KEYFN,PRED> &a_rOther) const;
		// Returns true if the two vectors move items between
		// each other.

	void Assign (Status_t &a_reStatus,
			const Vector<KEY,VALUE,KEYFN,PRED> &a_rOther);
		// Assign the contents of the other vector to ourselves.

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

	size_t GetSizeOfLargestNode (void) const { return sizeof (Node); }
		// Return the size of the largest possible node.
		// (Not used -- it's only to preserve interface compatibility
		// with SkipList<>.)

private:

	Allocator_t &m_rNodeAllocator;
		// Where we get memory to allocate nodes.

	bool m_bAllowDuplicates;
		// true if we allow duplicate elements.

	Node *m_pItems;
		// The array of items we contain.

	uint32_t m_nItems;
		// The number of valid items in this vector.
	
	uint32_t m_nSpace;
		// The amount of space for items in this vector.
		// (Will always be greater than or equal to the number of
		// valid items.)
	
	uint32_t m_nInitialItems, m_nAdditionalItems;
		// The number of initial/additional items to allocate.

	KEYFN m_oKeyFn;
		// How we extract a key from a value.

	PRED m_oPred;
		// How we compare keys to each other.
	
	void MakeSpace (Status_t &a_reStatus, uint32_t a_nItems);
		// Make space in the array for the given number of items.

	void SearchLower (const KEY &a_rKey, uint32_t &a_rnTraverse) const;
		// Search for an item greater than or equal to a_rKey.

	void SearchUpper (const KEY &a_rKey, uint32_t &a_rnTraverse) const;
		// Search for an item greater than a_rKey.

	#ifdef DEBUG_VECTOR
	bool m_bDebug;
	#endif // DEBUG_VECTOR
		// true if the invariant should be checked.
};



// The default node allocator.  Allocates 64K at a time.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Allocator_t
	Vector<KEY,VALUE,KEYFN,PRED>::sm_oNodeAllocator (65536);



// Default constructor.  Must be followed by Init().
template <class KEY, class VALUE, class KEYFN, class PRED>
Vector<KEY,VALUE,KEYFN,PRED>::Vector (const PRED &a_rPred,
		Allocator_t &a_rAlloc)
	: m_rNodeAllocator (a_rAlloc), m_oPred (a_rPred)
{
	// Set up some defaults.
	m_bAllowDuplicates = false;
	m_pItems = NULL;
	m_nItems = m_nSpace = m_nInitialItems = m_nAdditionalItems = 0;
#ifdef DEBUG_VECTOR
	m_bDebug = false;

	// Make sure we're intact.
	Invariant();
#endif // DEBUG_VECTOR
}



// Constructor.  Specify whether or not duplicates are allowed, and
// the initial/additional number of items to allocate.
template <class KEY, class VALUE, class KEYFN, class PRED>
Vector<KEY,VALUE,KEYFN,PRED>::Vector (Status_t &a_reStatus,
		bool a_bAllowDuplicates, const InitParams &a_rInitParams,
		const PRED &a_rPred, Allocator_t &a_rAlloc)
	: m_rNodeAllocator (a_rAlloc), m_oPred (a_rPred)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Set up some defaults.
	m_bAllowDuplicates = false;
	m_pItems = NULL;
	m_nItems = m_nSpace = m_nInitialItems = m_nAdditionalItems = 0;
	#ifdef DEBUG_VECTOR
	m_bDebug = false;
	#endif // DEBUG_VECTOR

	// Init() does all the work.
	Init (a_reStatus, a_rInitParams, a_bAllowDuplicates);
}



// Construction method.  Specify whether or not duplicates are allowed, and
// the initial/additional number of items to allocate.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Init (Status_t &a_reStatus,
	bool a_bAllowDuplicates, const InitParams &a_rInitParams)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the number of initial/additional items is sane.
	assert (a_rInitParams.m_nInitialItems > 0);
	assert (a_rInitParams.m_nAdditionalItems > 0);

	// Make sure we haven't been initialized already.
	assert (m_nInitialItems == 0);

	// Fill in the blanks.
	m_bAllowDuplicates = a_bAllowDuplicates;
	m_nItems = m_nSpace = 0;
	m_nInitialItems = a_rInitParams.m_nInitialItems;
	m_nAdditionalItems = a_rInitParams.m_nAdditionalItems;

	// No items yet.
	m_pItems = NULL;

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR
}



// Destructor.
template <class KEY, class VALUE, class KEYFN, class PRED>
Vector<KEY,VALUE,KEYFN,PRED>::~Vector (void)
{
	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// If we have anything to delete, delete it.
	if (m_pItems != NULL)
	{
		for (uint32_t i = 0; i < m_nSpace; ++i)
			m_pItems[i].~Node();
		m_rNodeAllocator.Deallocate (0, m_nSpace * sizeof (Node),
			(void *) m_pItems);
	}
}



#ifdef DEBUG_VECTOR

// Set whether to run the vector invariant before and after methods.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::SetDebug (bool a_bDebug)
{
	// Easy enough.
	m_bDebug = a_bDebug;
}

#endif // DEBUG_VECTOR



// Insert an item into the list.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::InsertResult
Vector<KEY,VALUE,KEYFN,PRED>::Insert (Status_t &a_reStatus,
	const VALUE &a_rValue)
{
	uint32_t nIndex;
		// The index where the new node gets inserted.
	InsertResult oInsertResult;
		// The result of the insertion.

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0);

	// Make space for this new item, if needed.
	MakeSpace (a_reStatus, m_nItems + 1u);
	if (a_reStatus != g_kNoError)
	{
		// The item cannot be inserted.
		oInsertResult.m_itPosition = Iterator (NULL, 0u, this);
		oInsertResult.m_bInserted = false;
		return oInsertResult;
	}

	// Find where to put this key.  (Put equal keys at the upper bound.)
	SearchUpper (m_oKeyFn (a_rValue), nIndex);

	// Insert this node if there's no duplicate already in here.
	//
	// If duplicates are allowed, always insert.
	// If the new item is to be put at index 0, then there's no item of
	// the same value in the array, so it's OK to insert.
	// If the item before the insertion location is less than the item
	// being inserted, then there's no item of the same value in the array,
	// so it's OK to insert.
	if (m_bAllowDuplicates
	|| nIndex == 0
	|| m_oPred (m_oKeyFn (m_pItems[nIndex - 1u].m_oValue),
		m_oKeyFn (a_rValue)))
	{
		// Move items out of the way to make space for the new item.
		// Use assignment in a loop instead of memmove(), to preserve
		// the value's assignment semantics (i.e. to use the value class'
		// assignment operator).
		if (nIndex != m_nItems)
			for (uint32_t i = m_nItems; i > nIndex; --i)
				m_pItems[i] = m_pItems[i - 1u];

		// Install the value.
		m_pItems[nIndex].m_oValue = a_rValue;
		++m_nItems;

		// Let them know where we put it, and that we did insert it.
		oInsertResult.m_itPosition = Iterator (m_pItems + nIndex,
			nIndex, this);
		oInsertResult.m_bInserted = true;
	}
	else
	{
		// We didn't insert it.  Show them the equal item.
		oInsertResult.m_itPosition = Iterator (m_pItems + nIndex - 1u,
			nIndex - 1u, this);
		oInsertResult.m_bInserted = false;
	}

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Let them know what happened.
	return oInsertResult;
}



// Insert an item into the list, at this exact location, if it's safe.
// Returns where it was really inserted.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Iterator
Vector<KEY,VALUE,KEYFN,PRED>::Insert (Status_t &a_reStatus,
	Iterator a_itPosition, const VALUE &a_rValue)
{
	const KEY &rKey = m_oKeyFn (a_rValue);
		// The key of what they're inserting.
	uint32_t nIndex;
		// The index where the new node gets inserted.
	InsertResult oInsertResult;
		// The result of the insertion.

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure this iterator is sane, i.e. that it's associated with
	// this vector and that the node-pointer and node-index agree.
	assert (a_itPosition.m_pVector == this);
	assert (a_itPosition.m_pNode == NULL
		|| (a_itPosition.m_nNode < m_nItems
			&& a_itPosition.m_pNode == m_pItems + a_itPosition.m_nNode));
	assert (a_itPosition.m_pNode != NULL || a_itPosition.m_nNode == 0u);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Make space for this new item, if needed.
	MakeSpace (m_nItems + 1u);

	// If the given iterator is at the end of the list, fix it to
	// point one item past the last item in the list.  (Such an iterator
	// isn't generally considered valid, but it's only used internally.)
	//
	// At the same time, get the item-index corresponding to this iterator.
	if (a_itPosition.m_pNode == NULL)
	{
		nIndex = m_nItems;
		a_itPosition.m_pNode = m_pItems + m_nItems;
	}
	else
	{
		nIndex = a_itPosition.m_nNode;

		// Since space could have been reallocated by the above call to
		// MakeSpace(), the given iterator needs to be fixed.
		a_itPosition.m_pNode = m_pItems + a_itPosition.m_nNode;
	}

	// The new item has to fit where this iterator points.
	Iterator oBefore = a_itPosition; --oBefore;
	if ((a_itPosition.m_pNode != m_pItems + m_nItems
		&& m_oPred (m_oKeyFn (a_itPosition.m_pNode->m_oValue), rKey))
	|| (oBefore.m_pNode != NULL
		&& m_oPred (rKey, m_oKeyFn (oBefore.m_pNode->m_oValue))))
	{
		// Don't use their iterator.
		oInsertResult = Insert (a_reStatus, a_rValue);
	}

	// Insert this node if there's no duplicate already in here.
	//
	// If duplicates are allowed, always insert.
	// If the new item is to be put at index 0, then there's no item of
	// the same value in the array, so it's OK to insert.
	// If the item before the insertion location is less than the item
	// being inserted, then there's no item of the same value in the array,
	// so it's OK to insert.
	else if (m_bAllowDuplicates
	|| nIndex == 0
	|| m_oPred (m_oKeyFn (m_pItems[nIndex - 1u].m_oValue),
		m_oKeyFn (a_rValue)))
	{
		// Move items out of the way to make space for the new item.
		if (nIndex != m_nItems)
		{
			uint32_t i = m_nItems;
			for (;;)
			{
				m_pItems[i] = m_pItems[i - 1u];
				if (i == nIndex)
					break;
				--i;
			}
		}

		// Install the value.
		m_pItems[nIndex].m_oValue = a_rValue;
		++m_nItems;

		// Let them know where we put it, and that we did insert it.
		oInsertResult.m_itPosition = Iterator (m_pItems + nIndex,
			nIndex, this);
		oInsertResult.m_bInserted = true;
	}
	else
	{
		// We didn't insert it.  Show them the equal item.
		oInsertResult.m_itPosition = Iterator (m_pItems + nIndex,
			nIndex, this);
		oInsertResult.m_bInserted = false;
	}

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Let them know what happened.
	return oInsertResult.m_itPosition;
}



// Insert a range of items from another vector.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Insert (Status_t &a_reStatus,
	ConstIterator a_itFirst, ConstIterator a_itLast)
{
	ConstIterator itHere;
		// The next item to insert.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Try to insert every item they gave us.
	for (itHere = a_itFirst; itHere != a_itLast; itHere++)
	{
		Insert (a_reStatus, *itHere);
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
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Iterator
Vector<KEY,VALUE,KEYFN,PRED>::Erase (Iterator a_itHere)
{
	// Make sure this iterator is sane, i.e. that it's associated with
	// this vector and that the node-pointer and node-index agree.
	assert (a_itHere.m_pVector == this);
	assert (a_itHere.m_pNode == NULL
		|| (a_itHere.m_nNode < m_nItems
			&& a_itHere.m_pNode == m_pItems + a_itHere.m_nNode));
	assert (a_itHere.m_pNode != NULL || a_itHere.m_nNode == 0u);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Don't let them erase End().
	if (a_itHere.m_pNode == NULL)
		return Begin();

	// Erase it.
	for (uint32_t i = a_itHere.m_nNode; i < m_nItems - 1u; ++i)
		m_pItems[i] = m_pItems[i + 1u];
	--m_nItems;

	// The last item in the array was moved down one index.
	// Clean up the previous copy of the last item by assigning
	// a default instance to it.  (This is probably overkill in
	// practice, but it keeps our C++ semantics clean.)
	m_pItems[m_nItems] = Node();

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Return an iterator that points past the deleted node.
	if (a_itHere.m_nNode == m_nItems)
	{
		// If the deleted node was the last one in the array, then
		// one past the deleted node is End().
		a_itHere.m_pNode = NULL;
		a_itHere.m_nNode = 0u;
	}
	return a_itHere;
}



// Erase a range of items in this list.  Return the item following the
// last one removed.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Iterator
Vector<KEY,VALUE,KEYFN,PRED>::Erase (Iterator a_itFirst, Iterator a_itLast)
{
	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure these iterators are sane, i.e. that they're associated
	// with this vector and that the node-pointer and node-index agree.
	assert (a_itFirst.m_pVector == this);
	assert (a_itFirst.m_pNode == NULL
		|| (a_itFirst.m_nNode < m_nItems
			&& a_itFirst.m_pNode == m_pItems + a_itFirst.m_nNode));
	assert (a_itFirst.m_pNode != NULL || a_itFirst.m_nNode == 0u);
	assert (a_itLast.m_pVector == this);
	assert (a_itLast.m_pNode == NULL
		|| (a_itLast.m_nNode < m_nItems
			&& a_itLast.m_pNode == m_pItems + a_itLast.m_nNode));
	assert (a_itLast.m_pNode != NULL || a_itLast.m_nNode == 0u);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Make sure these iterators are in the right order.
	assert (a_itFirst == Begin()
		|| a_itLast == End()
		|| !m_oPred (m_oKeyFn (a_itLast.m_pNode->m_oValue),
			m_oKeyFn (a_itFirst.m_pNode->m_oValue)));
	
	// If they gave us an empty range, we're done.
	if (a_itFirst == a_itLast)
		return a_itLast;

	// If the last-iterator is at the end of the list, fix it to
	// point one item past the last item in the list.  (Such an iterator
	// isn't generally considered valid, but it's only used internally.)
	if (a_itLast.m_pNode == NULL)
	{
		a_itLast.m_pNode = m_pItems + m_nItems;
		a_itLast.m_nNode = m_nItems;
	}

	// Sanity check: make sure the node-indices are in the right order.
	assert (a_itFirst.m_nNode <= a_itLast.m_nNode);

	// Copy valid items down over the range of deleted items.
	uint32_t nOffset = a_itLast.m_nNode - a_itFirst.m_nNode;
	uint32_t nEnd = m_nItems - nOffset;
	for (uint32_t i = a_itFirst.m_nNode; i < nEnd; ++i)
		m_pItems[i] = m_pItems[i + nOffset];

	// Clean up the previous instances of the moved items by assigning
	// default instances to them.  (This is probably overkill in practice,
	// but it keeps our C++ semantics clean.)
	// Never mind...if VALUE's default constructor doesn't initialize
	// anything, this generates a compiler warning about that.
	/* Node oDefaultInstance;
	for (uint32_t i = nEnd; i < m_nItems; ++i)
		m_pItems[i] = oDefaultInstance; */

	// Now we contain less items.
	m_nItems -= nOffset;

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Return the end of the removed range.
	a_itLast.m_pNode -= nOffset;
	a_itLast.m_nNode -= nOffset;
	return a_itLast;
}



// Empty the list.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Clear (void)
{
	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Clean up the previous instances of the items by assigning default
	// instances to them.  (This is probably overkill in practice, but it
	// keeps our C++ semantics clean.)
	// Never mind...if VALUE's default constructor doesn't initialize
	// anything, this generates a compiler warning about that.
	/* Node oDefaultInstance;
	for (uint32_t i = 0u; i < m_nItems; ++i)
		m_pItems[i] = oDefaultInstance; */

	// Now there are no items.
	m_nItems = 0u;
}



// Purge all internally-allocated memory.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Purge (void)
{
	// Make sure there are no contained items.
	assert (m_nItems == 0u);

	// If we have anything to delete, delete it.
	if (m_pItems != NULL)
	{
		// Destroy the contained items.
		for (uint32_t i = 0; i < m_nSpace; ++i)
			m_pItems[i].~Node();

		// Deallocate the contained memory.
		m_rNodeAllocator.Deallocate (0, m_nSpace * sizeof (Node),
			(void *) m_pItems);

		// Now there is no contained memory.
		m_pItems = NULL;
		m_nSpace = 0u;
	}
}



// Move all items from the other vector to ourself.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Move (Vector<KEY,VALUE,KEYFN,PRED> &a_rOther)
{
	Node *p;
	uint32_t nSpace;
		// Used to swap structures.

	// Make sure the vectors can move items between themselves.
	assert (CanMove (a_rOther));

	// Make sure we're all intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	a_rOther.Invariant();
	#endif // DEBUG_VECTOR

	// Make sure we're empty.
	assert (m_nItems == 0);

	// Swap the contained arrays.
	p = m_pItems;
	m_pItems = a_rOther.m_pItems;
	a_rOther.m_pItems = p;

	// Now we have all their items.
	m_nItems = a_rOther.m_nItems;
	a_rOther.m_nItems = 0;

	// Swap the amount of allocated space.
	nSpace = m_nSpace;
	m_nSpace = a_rOther.m_nSpace;
	a_rOther.m_nSpace = nSpace;

	// Make sure we're all intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	a_rOther.Invariant();
	#endif // DEBUG_VECTOR
}



// Returns true if the two vectors can move their items between each other.
template <class KEY, class VALUE, class KEYFN, class PRED>
bool
Vector<KEY,VALUE,KEYFN,PRED>::CanMove
	(const Vector<KEY,VALUE,KEYFN,PRED> &a_rOther) const
{
	// They can if they have the same allocator.
	return (&m_rNodeAllocator == &a_rOther.m_rNodeAllocator);
}



// Assign the contents of the other vector to ourselves.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Assign (Status_t &a_reStatus,
	const Vector<KEY,VALUE,KEYFN,PRED> &a_rOther)
{
	Node *pBegin, *pEnd;
		// Where we're looking in the other list.

	// Make sure we're all intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	a_rOther.Invariant();
	#endif // DEBUG_VECTOR

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Get the range of items from the other list.
	pBegin = a_rOther.Begin().m_pNode;
	pEnd = a_rOther.End().m_pNode;

	// Make space for all the items in the other vector, if needed.
	MakeSpace (a_reStatus, a_rOther.m_nItems);
	if (a_reStatus != g_kNoError)
		return;

	// Copy all the items.
	for (uint32_t i = 0u; i < a_rOther.m_nItems; ++i)
		m_pItems[i] = a_rOther.m_pItems[i];
	m_nItems = a_rOther.m_nItems;

	// Make sure we're all intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR
}



// Find the given item in the list.  Returns End() if not found.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Iterator
Vector<KEY,VALUE,KEYFN,PRED>::Find (const KEY &a_rKey)
{
	Iterator itHere;
		// What we found.

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Look for the item.
	itHere = LowerBound (a_rKey);

	// LowerBound() returns the first item >= the key.  So if this item
	// is greater than what they were asking for, that means we didn't
	// find it.
	if (itHere == End()
	|| m_oPred (a_rKey, m_oKeyFn (itHere.m_pNode->m_oValue)))
		return End();
	else
		return itHere;
}



// Find the given item in the list.  Returns End() if not found.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::ConstIterator
Vector<KEY,VALUE,KEYFN,PRED>::Find (const KEY &a_rKey) const
{
	ConstIterator itHere;
		// What we found.

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Look for the item.
	itHere = LowerBound (a_rKey);

	// LowerBound() returns the first item >= the key.  So if this item
	// is greater than what they were asking for, that means we didn't
	// find it.
	if (itHere == End()
	|| m_oPred (a_rKey, m_oKeyFn (itHere.m_pNode->m_oValue)))
		return End();
	else
		return itHere;
}



// Return the position of the first item that's >= the key.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Iterator
Vector<KEY,VALUE,KEYFN,PRED>::LowerBound (const KEY &a_rKey)
{
	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Search for the first item >= the key.
	uint32_t nIndex;
	SearchLower (a_rKey, nIndex);

	// Return what we found.
	if (nIndex < m_nItems)
		return Iterator (m_pItems + nIndex, nIndex, this);
	else
		return End();
}



// Return the position of the first item that's >= the key.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::ConstIterator
Vector<KEY,VALUE,KEYFN,PRED>::LowerBound (const KEY &a_rKey) const
{
	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Search for the first item >= the key.
	uint32_t nIndex;
	SearchLower (a_rKey, nIndex);

	// Return what we found.
	if (nIndex < m_nItems)
		return ConstIterator (m_pItems + nIndex, nIndex, this);
	else
		return End();
}



// Return the position of the first item that's > the key.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::Iterator
Vector<KEY,VALUE,KEYFN,PRED>::UpperBound (const KEY &a_rKey)
{
	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Search for the first item >= the key.
	uint32_t nIndex;
	SearchUpper (a_rKey, nIndex);

	// Return what we found.
	if (nIndex < m_nItems)
		return Iterator (m_pItems + nIndex, nIndex, this);
	else
		return End();
}



// Return the position of the first item that's > the key.
template <class KEY, class VALUE, class KEYFN, class PRED>
typename Vector<KEY,VALUE,KEYFN,PRED>::ConstIterator
Vector<KEY,VALUE,KEYFN,PRED>::UpperBound (const KEY &a_rKey) const
{
	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Search for the first item >= the key.
	uint32_t nIndex;
	SearchUpper (a_rKey, nIndex);

	// Return what we found.
	if (nIndex < m_nItems)
		return ConstIterator (m_pItems + nIndex, nIndex, this);
	else
		return End();
}



// Make space in the array for the given number of items.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::MakeSpace (Status_t &a_reStatus,
	uint32_t a_nItems)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// If there isn't space for this many items, allocate it,
	if (m_nSpace < a_nItems)
	{
		uint32_t nSpace;
			// The amount of space the vector will have now.
		Node *pSpace;
			// The newly allocated space.

		// If no items have been allocated yet, do so.
		// If items have been allocated, allocate some additional items.
		// Allocate at least as many as they asked for.
		nSpace = Max (((m_nSpace == 0u) ? m_nInitialItems
			: m_nSpace + m_nAdditionalItems), a_nItems);

		// Sanity check: make sure we're trying to allocate more space.
		assert (nSpace > m_nSpace);

		// Allocate this much space.
		pSpace = (Node *) m_rNodeAllocator.Allocate (0,
			nSpace * sizeof (Node));
		if (pSpace == NULL)
		{
			a_reStatus = g_kOutOfMemory;
			return;
		}

		// Copy the valid items to their new location.
		for (uint32_t i = 0; i < m_nItems; ++i)
			new ((void *)(pSpace + i)) Node (m_pItems[i]);
		// (Create all of the unused nodes.)
		for (uint32_t i = m_nItems; i < nSpace; ++i)
			new ((void *)(pSpace + i)) Node;

		// Now we have more space for items.
		if (m_pItems != NULL)
		{
			for (uint32_t i = 0; i < m_nSpace; ++i)
				m_pItems[i].~Node();
			m_rNodeAllocator.Deallocate (0, m_nSpace * sizeof (Node),
				(void *) m_pItems);
		}
		m_pItems = pSpace;
		m_nSpace = nSpace;
	}
}



// Search for an item greater than or equal to a_rKey.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::SearchLower (const KEY &a_rKey,
	uint32_t &a_rnTraverse) const
{
	uint32_t nIndex;
		// Where we're searching.
	uint32_t nLow, nHigh;
		// The range of items we're searching.

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Do a binary search through the array, looking for the first
	// item that's >= a_rKey.
	nLow = 0;
	nHigh = m_nItems;
	while (nLow < nHigh)
	{
		// Get the current item.
		nIndex = (nLow + nHigh) >> 1;
		const KEY &rKey = m_oKeyFn (m_pItems[nIndex].m_oValue);

		// If it's less than the item we're looking for, we don't need to
		// search below this item, and this item is definitely not the one
		// we're looking for.
		if (m_oPred (rKey, a_rKey))
			nLow = nIndex + 1u;

		// If it's greater than or equal to the item we're looking for, we
		// don't need to search above this item, but this item might be the
		// one we're looking for.
		else
			nHigh = nIndex;

		// Sanity check: make sure we don't enter an infinite loop.
		assert (nLow == nHigh || ((nLow + nHigh) >> 1) != nIndex);
	}

	// Let our caller know what we found.
	assert (nLow == nHigh);
	a_rnTraverse = nLow;
}



// Search for an item greater than a_rKey.
template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::SearchUpper (const KEY &a_rKey,
	uint32_t &a_rnTraverse) const
{
	uint32_t nIndex;
		// Where we're searching.
	uint32_t nLow, nHigh;
		// The range of items we're searching.

	// Make sure we're intact.
	#ifdef DEBUG_VECTOR
	Invariant();
	#endif // DEBUG_VECTOR

	// Make sure we have been initialized properly.
	assert (m_nInitialItems != 0u);

	// Do a binary search through the array, looking for the first
	// item that's > a_rKey.
	nLow = 0;
	nHigh = m_nItems;
	while (nLow < nHigh)
	{
		// Get the current item.
		nIndex = (nLow + nHigh) >> 1;
		const KEY &rKey = m_oKeyFn (m_pItems[nIndex].m_oValue);

		// If it's greater than the item we're looking for, we don't need
		// to search above this item, but this item might be the one we're
		// looking for.
		if (m_oPred (a_rKey, rKey))
			nHigh = nIndex;

		// If it's less than or equal to the item we're looking for, we
		// don't need to search below this item, and this item is
		// definitely not the one we're looking for.
		else
			nLow = nIndex + 1u;

		// Sanity check: make sure we don't enter an infinite loop.
		assert (nLow == nHigh || ((nLow + nHigh) >> 1) != nIndex);
	}

	// Let our caller know what we found.
	assert (nLow == nHigh);
	a_rnTraverse = nLow;
}



#ifdef DEBUG_VECTOR

template <class KEY, class VALUE, class KEYFN, class PRED>
void
Vector<KEY,VALUE,KEYFN,PRED>::Invariant (void) const
{
	// Only check the invariant if they requested we do.
	if (!m_bDebug)
		return;

	// If the vector is not fully initialized, we have less to check.
	if (m_nInitialItems == 0u)
	{
		assert (m_pItems == NULL);
		assert (m_nItems == 0);
		assert (m_nSpace == 0);
		assert (m_nAdditionalItems == 0);
		return;
	}

	// If there are any items allocated, check them.
	if (m_nItems != 0u)
	{
		// Make sure we don't have more items than we have space for items.
		assert (m_nItems <= m_nSpace);

		// Run through the array, make sure the items are sorted properly.
		// (The first and last item don't need to be iterated through here,
		// since there's no item before the first and no item after the
		// last.)
		for (uint32_t i = 1u; i < m_nItems - 1u; ++i)
		{
			// Make sure that the nodes are in proper sorted order.
			if (m_bAllowDuplicates)
			{
				// The current item has to be >= the previous item.
				assert (!m_oPred (m_oKeyFn (m_pItems[i].m_oValue),
					m_oKeyFn (m_pItems[i - 1u].m_oValue)));
			}
			else
			{
				// The current item has to be > the previous item.
				assert (m_oPred (m_oKeyFn (m_pItems[i - 1u].m_oValue),
					m_oKeyFn (m_pItems[i].m_oValue)));
			}
		}
	}
}

#endif // DEBUG_VECTOR



#endif // __VECTOR_H__
