#ifndef __DOUBLY_LINKED_LIST_H__
#define __DOUBLY_LINKED_LIST_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// A cheap doubly-linked-list class.  Needs to be the base class for
// whatever type it's parameterized with, in order to work.
template <class TYPE>
class DoublyLinkedList
{
public:
	TYPE *m_pForward;
	TYPE *m_pBackward;
		// Forward/backward pointers.

	DoublyLinkedList();
		// Default constructor.

	~DoublyLinkedList();
		// Destructor.

	void InsertBefore (TYPE *a_pThis, TYPE *a_pCell);
		// Insert the given cell before us in the list.

	void InsertAfter (TYPE *a_pThis, TYPE *a_pCell);
		// Insert the given cell after us in the list.

	void Remove (void);
		// Remove ourselves from the list we're in.
};



// Default constructor.
template <class TYPE>
DoublyLinkedList<TYPE>::DoublyLinkedList()
{
	// Not part of any list.
	m_pForward = m_pBackward = NULL;
}



// Destructor.
template <class TYPE>
DoublyLinkedList<TYPE>::~DoublyLinkedList()
{
	// Make sure we're not part of a list any more -- otherwise, our
	// destruction will leave dangling references.
	assert (m_pForward == NULL);
	assert (m_pBackward == NULL);
}



// Insert the given cell before us in the list.
template <class TYPE>
void
DoublyLinkedList<TYPE>::InsertBefore (TYPE *a_pThis, TYPE *a_pCell)
{
	// Make sure they passed us a pointer to ourselves, different only
	// by type.
	assert (a_pThis == this);

	// Make sure they gave us a cell to insert.
	assert (a_pCell != NULL);

	// Make sure the cell doesn't think it's in a list already.
	assert (a_pCell->m_pForward == NULL);
	assert (a_pCell->m_pBackward == NULL);

	// Pretty straightforward.
	a_pCell->m_pBackward = m_pBackward;
	m_pBackward = a_pCell;
	a_pCell->m_pForward = a_pThis;
	assert (a_pCell->m_pBackward->m_pForward == a_pThis);
	a_pCell->m_pBackward->m_pForward = a_pCell;
}



// Insert the given cell after us in the list.
template <class TYPE>
void
DoublyLinkedList<TYPE>::InsertAfter (TYPE *a_pThis, TYPE *a_pCell)
{
	// Make sure they passed us a pointer to ourselves, different only
	// by type.
	assert (a_pThis == this);

	// Make sure they gave us a cell to insert.
	assert (a_pCell != NULL);

	// Make sure the cell doesn't think it's in a list already.
	assert (a_pCell->m_pForward == NULL);
	assert (a_pCell->m_pBackward == NULL);

	// Pretty straightforward.
	a_pCell->m_pForward = m_pForward;
	m_pForward = a_pCell;
	a_pCell->m_pBackward = a_pThis;
	assert (a_pCell->m_pForward->m_pBackward == a_pThis);
	a_pCell->m_pForward->m_pBackward = a_pCell;
}



// Remove ourselves from the list we're in.
template <class TYPE>
void
DoublyLinkedList<TYPE>::Remove (void)
{
	// Make sure we think we're in a list.
	assert (m_pForward != NULL);
	assert (m_pBackward != NULL);

	// Get the nodes that we'll be pointing to.  (We have to do this
	// as a separate step, in case we're the last item in a circular
	// list.)
	TYPE *pForward = m_pForward;
	TYPE *pBackward = m_pBackward;

	// Remove ourselves from the doubly-linked list we're in.
	m_pBackward->m_pForward = pForward;
	m_pForward->m_pBackward = pBackward;

	// Remove our stale links.
	m_pForward = m_pBackward = NULL;
}



#endif // __DOUBLY_LINKED_LIST_H__
