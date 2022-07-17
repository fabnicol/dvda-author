#ifndef __PLACEMENTALLOCATOR_H__
#define __PLACEMENTALLOCATOR_H__

// This file (C) 2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include "mjpeg_types.h"
#include "Limits.hh"


// An allocator class, compatible with Allocator<>, that allows
// placement allocation.
// One possible use is to allow Set<> to implement an in-place free-space
// list for a variable-size memory allocator.
class PlacementAllocator
{
public:
	PlacementAllocator (size_t a_nChunkSize);
		// Constructor.
		// The value of a_nChunkSize is ignored -- it's present only to
		// remain compatible with Allocator<>'s interface.
	
	~PlacementAllocator();
		// Destructor.
	
	void SetAllocatedMemory (void *a_pMemory, size_t a_nBytes);
		// Set the next block of memory that will be "allocated".
		// (The next call to Allocate() will return this block.)
	
	void *GetDeallocatedMemory (size_t &a_rnBytes);
		// Get the previous block of memory that was deallocated.
		// (The previous call to Deallocate() set these values.)

	void *Allocate (size_t a_nSize, size_t a_nBytes);
		// Allocate a block of memory of the given number of bytes.
		// Returns NULL if memory is exhausted.
		// The value of a_nSize is ignored.
	
	void Deallocate (size_t a_nSize, size_t a_nBytes, void *a_pMemory);
		// Deallocate previously-allocated memory.
		// The value of a_nSize is ignored.

private:
	void *m_pMemory;
	size_t m_nBytes;
		// The current block of memory, and its size.
		// Set by either SetAllocatedMemory() or Deallocate().
		// Retrieved by either Allocate() or GetDeallocatedMemory().
};



// Default constructor.
PlacementAllocator::PlacementAllocator (size_t /* a_nChunkSize */)
	: m_pMemory (NULL), m_nBytes (0)
{
	// Nothing else to do.
}



// Destructor.
PlacementAllocator::~PlacementAllocator()
{
	// Make sure no block of memory is still being tracked.
	assert (m_pMemory == NULL);
}



// Set the next block of memory that will be "allocated".
// (The next call to Allocate() will return this block.)
void
PlacementAllocator::SetAllocatedMemory (void *a_pMemory, size_t a_nBytes)
{
	// Make sure there's no existing block of memory being tracked.
	assert (m_pMemory == NULL);

	// Keep track of this block of allocated memory.
	m_pMemory = a_pMemory;
	m_nBytes = a_nBytes;
}



// Get the previous block of memory that was deallocated.
// (The previous call to Deallocate() set these values.)
void *
PlacementAllocator::GetDeallocatedMemory (size_t &a_rnBytes)
{
	// Make sure there's a block of deallocated memory.
	assert (m_pMemory != NULL);

	// Remember the block of deallocated memory.
	void *pMemory = m_pMemory;
	a_rnBytes = m_nBytes;

	// Remember that this block of deallocated memory has been retrieved.
	m_pMemory = NULL;
	m_nBytes = 0;

	// Return the block of deallocated memory.
	return pMemory;
}



// Allocate memory for another object of type TYPE.
// Use the given size-bucket, which must be for the given number
// of bytes.
// Returns NULL if memory is exhausted.
void *
PlacementAllocator::Allocate (size_t a_nSize, size_t a_nBytes)
{
	// Make sure there's a block of memory to allocate.
	assert (m_pMemory != NULL);

	// Make sure it's large enough for this allocation.
	assert (m_nBytes >= a_nBytes);

	// Remember the block of memory to allocate.
	void *pMemory = m_pMemory;

	// Remember that this block of memory has been allocated.
	m_pMemory = NULL;
	m_nBytes = 0;

	// Return the block of allocated memory.
	return pMemory;
}



// Deallocate previously-allocated memory.
void
PlacementAllocator::Deallocate (size_t a_nSize, size_t a_nBytes,
	void *a_pMemory)
{
	// Make sure there's a block of memory to deallocate.
	assert (a_pMemory != NULL);

	// Allow the tracking of an existing block of memory, i.e. don't assert
	// that m_pMemory is NULL.
	// This makes it possible to purge several items at once
	// without having to "clean up" between them.

	// Keep track of this block of deallocated memory.
	m_pMemory = a_pMemory;
	m_nBytes = a_nBytes;
}



#endif // __PLACEMENTALLOCATOR_H__
