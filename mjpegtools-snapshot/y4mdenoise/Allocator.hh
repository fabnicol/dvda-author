#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include "mjpeg_types.h"
#include "Limits.hh"


// An allocator for small classes.  It gets large chunks from the
// standard memory allocator & divides it up.  It's able to handle
// several different object sizes at once.
template <size_t SIZES>
class Allocator
{
public:
	Allocator (size_t a_nChunkSize);
		// Constructor.  Specify the number of bytes to allocate at a
		// time from the standard memory allocator.

	~Allocator();
		// Destructor.

	void *Allocate (size_t a_nSize, size_t a_nBytes);
		// Allocate memory for another object.
		// Use the given size-bucket, which must be for the given number
		// of bytes.
		// Returns NULL if memory is exhausted.

	void Deallocate (size_t a_nSize, size_t a_nBytes, void *a_pMemory);
		// Deallocate previously-allocated memory.

	uint32_t GetNumAllocated (void) const { return m_ulAllocated; }
		// Get the number of allocated blocks.

private:
	// One chunk of memory.
	class Chunk
	{
	public:
		Chunk *m_pNext;
			// The next allocated chunk.
		char *m_aSpace;
			// The memory to divide up.
	};

	size_t m_nChunkSize;
		// The size of allocated chunks.  Set by the constructor.

	Chunk *m_pChunks;
		// A linked-list of all the allocated chunks.

	char *m_pFreeChunk;
		// The next piece of unallocated memory in the
		// most-recently-allocated chunk.

	void *m_apFree[SIZES];
		// Linked lists of freed pieces of memory, for all the sizes we
		// manage.

	uint32_t m_ulAllocated;
		// The number of live allocations, i.e. those that haven't been
		// deleted yet.

	#ifndef NDEBUG
	size_t m_aiSizes[SIZES];
	#endif // NDEBUG
		// The size of the pieces of memory in each bucket.
		// Used to make sure they always ask for the same memory size
		// in each bucket.

	void Purge (void);
		// Free up all chunks.
		// Only safe if there are no live allocations.
};



// Constructor.  Specify the number of bytes to allocate at a
// time from the standard memory allocator.
template <size_t SIZES>
Allocator<SIZES>::Allocator (size_t a_nChunkSize)
	: m_pChunks (NULL), m_pFreeChunk (NULL)
{
	// Round our chunk size up to the nearest pointer size.
	m_nChunkSize = ((a_nChunkSize + sizeof (Chunk *) - 1)
		/ sizeof (Chunk *)) * sizeof (Chunk *);

	// All our buckets are empty.
	for (size_t i = 0; i < SIZES; ++i)
	{
		m_apFree[i] = NULL;

		// (We don't know the memory-size of each bucket yet.)
		#ifndef NDEBUG
		m_aiSizes[i] = Limits<size_t>::Max;
		#endif // NDEBUG
	}

	// No allocations yet.
	m_ulAllocated = 0UL;
}



// Destructor.
template <size_t SIZES>
Allocator<SIZES>::~Allocator()
{
	// If all allocated objects were deallocated, go ahead and free
	// up our memory.  (If there are any allocated objects left, then
	// generally, that means this is a global allocator, and since C++
	// doesn't guarantee order of destruction for global objects, we
	// have no guarantee our clients have been destroyed, and so it
	// isn't safe to delete our memory.)
	if (m_ulAllocated == 0UL)
		Purge();
}



// Allocate memory for another object.
// Use the given size-bucket, which must be for the given number
// of bytes.
// Returns NULL if memory is exhausted.
template <size_t SIZES>
void *
Allocator<SIZES>::Allocate (size_t a_nSize, size_t a_nBytes)
{
	void *pAlloc;
		// The memory we allocate.

	// Make sure they gave us a valid size.
	assert (a_nSize >= 0 && a_nSize < SIZES);

	// Make sure they gave us a valid number of bytes.
	assert (a_nBytes >= 0);

	// Round the number of bytes up to the nearest pointer size.
	a_nBytes = ((a_nBytes + sizeof (Chunk *) - 1)
		/ sizeof (Chunk *)) * sizeof (Chunk *);

	// Make sure the requested size fits within our chunk size.
	assert (a_nBytes <= m_nChunkSize - sizeof (Chunk *));

	// If we don't know the size of this bucket, we do now.
	#ifndef NDEBUG
	if (m_aiSizes[a_nSize] == Limits<size_t>::Max)
		m_aiSizes[a_nSize] = a_nBytes;
	#endif // NDEBUG

	// Make sure they ask for the same number of bytes each time.
	assert (m_aiSizes[a_nSize] == a_nBytes);

	// If there's a free piece of memory of this size, return it.
	if (m_apFree[a_nSize] != NULL)
	{
		// Remember the allocated memory.
		pAlloc = m_apFree[a_nSize];

		// Remove it from our list.
		m_apFree[a_nSize] = *(void **)m_apFree[a_nSize];

		// That's one more allocation.
		++m_ulAllocated;

		// Return the allocated memory.
		return pAlloc;
	}

	// If there's enough unallocated space in the current chunk,
	// use it.
	if (m_pFreeChunk != NULL
	&& size_t (m_pFreeChunk - ((char *)m_pChunks))
		<= m_nChunkSize - a_nBytes)
	{
		// Remember the allocated memory.
		pAlloc = (void *) m_pFreeChunk;

		// Move past this new allocated memory.
		m_pFreeChunk += a_nBytes;

		// That's one more allocation.
		++m_ulAllocated;

		// Return the allocated memory.
		return pAlloc;
	}

	// We'll have to create a new chunk in order to satisfy this
	// allocation request.

	// First, find a place for the rest of this chunk.
	//
	// (Not currently possible, unless we make m_aiSizes[] a non-debug
	// thing.)

	// Add a new chunk to our list.
	{
		// Allocate a new chunk.
		Chunk *pNewChunk = (Chunk *) malloc (m_nChunkSize);
		if (pNewChunk == NULL)
			return NULL;

		// Hook it into our list.
		pNewChunk->m_pNext = m_pChunks;
		m_pChunks = pNewChunk;
	}

	// The unallocated portion of the new chunk is here.
	m_pFreeChunk = m_pChunks->m_aSpace + a_nBytes;

	// That's one more allocation.
	++m_ulAllocated;

	// Return the allocated memory.
	return (void *) (&(m_pChunks->m_aSpace));
}



// Deallocate previously-allocated memory.
template <size_t SIZES>
void
Allocator<SIZES>::Deallocate (size_t a_nSize, size_t /* a_nBytes */,
	void *a_pMemory)
{
	// Make sure they gave us a valid size.
	assert (a_nSize >= 0 && a_nSize < SIZES);

	// Put this memory into the given bucket.
	*(void **)a_pMemory = m_apFree[a_nSize];
	m_apFree[a_nSize] = a_pMemory;

	// That's one less allocation.
	--m_ulAllocated;

	// If all memory is unallocated, free up our chunks & start over.
	if (m_ulAllocated == 0UL)
		Purge();
}



// Free up all chunks.
template <size_t SIZES>
void
Allocator<SIZES>::Purge (void)
{
	// Make sure there are no live allocations
	assert (m_ulAllocated == 0UL);

	// Empty the free-space list.
	for (size_t i = 0; i < SIZES; ++i)
		m_apFree[i] = NULL;
	m_pFreeChunk = NULL;

	// Free all allocated chunks.
	while (m_pChunks != NULL)
	{
		// Remember the next chunk.
		Chunk *pNextChunk = m_pChunks->m_pNext;

		// Free this chunk.
		free (m_pChunks);

		// Move to the next chunk.
		m_pChunks = pNextChunk;
	}
}



#endif // __ALLOCATOR_H__
