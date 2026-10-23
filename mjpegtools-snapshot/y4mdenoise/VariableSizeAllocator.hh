#ifndef __VARIABLESIZEALLOCATOR_H__
#define __VARIABLESIZEALLOCATOR_H__

// This file (C) 2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include "mjpeg_types.h"
#include <stdlib.h>
#include "Limits.hh"
#include "PlacementAllocator.hh"
#include "Set.hh"



// Define this to print progress to stderr.
//#define VSA_PRINT_PROGRESS

// Define this to be a value larger than zero in order to guard
// memory allocations from writing out of bounds.
#define VSA_GUARD_SIZE 0



// An allocator for variable-sized blocks of memory.  It gets large chunks
// from the standard memory allocator & divides it up.
//
// It maintains its own free-space set, using a SkipList<> with a
// PlacementAllocator.  This allows the free blocks themselves to be used
// by the skip-list nodes, allowing for an in-place free-space set.
class VariableSizeAllocator
{
private:
	// One block of memory.
	class Block
	{
	public:
		#ifndef NDEBUG
		void *m_pMemory;
		#endif // !NDEBUG
			// The address of the block.
			// Only needed for verification.

		size_t m_nBytes;
			// The size of the block, in bytes.

		// A comparison class, suitable for Set<>.
		class SortBySize
		{
		public:
			inline bool operator() (const Block &a_rLeft,
					const Block &a_rRight) const
				{ return a_rLeft.m_nBytes < a_rRight.m_nBytes; }
		};
	};

	typedef SkipList<Block,Block,Ident<Block,Block>,Block::SortBySize,
			10,PlacementAllocator> SetImpl_t;
		// How the free-space set is implemented internally.

public:
	typedef SetImpl_t::InitParams InitParams;
		// How that internal type is initialized.

	VariableSizeAllocator (size_t a_nChunkSize);
		// Constructor.  Specify the number of bytes to allocate at a
		// time from the standard memory allocator.

	void Init (Status_t &a_reStatus,
			const InitParams &a_rInitParams = InitParams());
		// Construction method.

	~VariableSizeAllocator();
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
		char m_aSpace[];
			// The memory to divide up.
	};

	size_t m_nChunkSize;
		// The size of all allocated chunks (unless a particular request is
		// over this size).  Set by the constructor.

	Chunk *m_pChunks;
		// A linked-list of all the allocated chunks.

	char *m_pFreeChunk;
	size_t m_nFreeChunk;
		// The next piece of unallocated memory in the
		// most-recently-allocated chunk.

	PlacementAllocator m_oPlacementAllocator;
		// Used to allow Set<> to implement our free-space list in-place.

	typedef Set<Block,Block::SortBySize,SetImpl_t> FreeSpaceSet_t;
	FreeSpaceSet_t m_setFreeSpace;
		// The free-space set.  Uses the free-blocks in-place as skip-list
		// nodes.

	size_t m_nSmallestBlockSize;
		// The size of the smallest block that can be allocated.
		//
		// Currently, this holds the size of the largest skip-list node
		// possible in m_setFreeSpace.
		// There may be other restrictions later.

	uint32_t m_ulAllocated;
		// The number of live allocations, i.e. those that haven't been
		// deleted yet.

	void Purge (void);
		// Free up all chunks.
		// Only safe if there are no live allocations.
};



// Constructor.  Specify the number of bytes to allocate at a
// time from the standard memory allocator.
VariableSizeAllocator::VariableSizeAllocator (size_t a_nChunkSize)
	: m_nChunkSize (a_nChunkSize), m_pChunks (NULL), m_pFreeChunk (NULL),
	m_nFreeChunk (0), m_oPlacementAllocator (a_nChunkSize),
	m_setFreeSpace (Block::SortBySize(), m_oPlacementAllocator),
	m_nSmallestBlockSize (m_setFreeSpace.GetSizeOfLargestNode()),
	m_ulAllocated (0UL)
{
	// Nothing else to do.
}


// Construction method.
void
VariableSizeAllocator::Init (Status_t &a_reStatus,
	const InitParams &a_rInitParams)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Finish setting up the free-space list.
	// Allow duplicates, i.e. free blocks of the same size.
	m_setFreeSpace.Init (a_reStatus, true, a_rInitParams);
	if (a_reStatus != g_kNoError)
		return;
}


// Destructor.
VariableSizeAllocator::~VariableSizeAllocator()
{
	// If all allocated objects were deallocated, go ahead and free
	// up our memory.  (If there are any allocated objects left, then
	// generally, that means this is a global allocator, and since C++
	// doesn't guarantee order of destruction for global objects, we
	// have no guarantee our clients have been destroyed, and so it
	// isn't safe to delete our memory.)
	if (m_ulAllocated == 0UL && m_pChunks != NULL)
		Purge();
}



// Allocate memory for another object.
// Use the given size-bucket, which must be for the given number
// of bytes.
// Returns NULL if memory is exhausted.
void *
VariableSizeAllocator::Allocate (size_t a_nSize, size_t a_nBytes)
{
	void *pAlloc;
		// The memory we allocate.

	// Make sure they gave us a valid size.
	// (This allocator has only one bucket.)
	assert (a_nSize == 0);

	// Make sure they gave us a valid number of bytes.
	assert (a_nBytes >= 0);

	// Adjust the size of the block so that it's large enough to be in the
	// free-space set.
	a_nBytes = Max (a_nBytes, m_nSmallestBlockSize);

	// Round the number of bytes up to the nearest pointer size.
	a_nBytes = ((a_nBytes + sizeof (Chunk *) - 1)
		/ sizeof (Chunk *)) * sizeof (Chunk *);

	// Print our progress.
	#ifdef VSA_PRINT_PROGRESS
	fprintf (stderr, "\tVariableSizeAllocator::Allocate(): request for "
		"%d bytes\n", (int)a_nBytes);
	fprintf (stderr, "\t\tFree-space set has %d entries\n",
		(int)m_setFreeSpace.Size());
	#endif // VSA_PRINT_PROGRESS

	// Look for a block of memory that's this size, or larger.
	//
	// Look for the upper bound, which will find the first block that's
	// greater than the requested size, and then get the previous item.
	// SkipList<> puts equal-valued keys (in this case, equal-size blocks)
	// at the upper-bound.  In order to maximize memory locality, free space
	// is allocated as a stack, not a queue.
	Block oSearch;
	oSearch.m_nBytes = a_nBytes;
	FreeSpaceSet_t::Iterator itHere = m_setFreeSpace.UpperBound (oSearch);
	--itHere;
	if (itHere != m_setFreeSpace.End() && (*itHere).m_nBytes == a_nBytes)
	{
		// Get the address of this block.
		#ifndef NDEBUG
		void *pMemoryDebug = (*itHere).m_pMemory;
		#endif // !NDEBUG

		// Remove this block from the free-space set.
		m_setFreeSpace.Erase (itHere);

		// Get the address of the freed block.
		size_t nBytesUnused = 0;
		pAlloc = m_oPlacementAllocator.GetDeallocatedMemory (nBytesUnused);

		// Print our progress.
		#ifdef VSA_PRINT_PROGRESS
		fprintf (stderr, "\t\t(Found in free-space set, address %08X)\n",
			(int)pAlloc);
		#endif // VSA_PRINT_PROGRESS

		// Double-check the address and size of the block.
		assert (pAlloc == pMemoryDebug);

		// Guard against writing out of bounds.
		#if VSA_GUARD_SIZE > 0
		{
			uint32_t *pGuard;
			size_t uGuard;

			// Guard the area before the block.
			pGuard = (uint32_t *)pAlloc;
			uGuard = VSA_GUARD_SIZE;
			while (uGuard > 0)
			{
				--pGuard;
				assert (*pGuard == 0xDEADBEEF);
				uGuard -= sizeof (uint32_t);
			}

			// Guard the area after the block.
			pGuard = (uint32_t *)((int)pAlloc + a_nBytes);
			uGuard = VSA_GUARD_SIZE;
			while (uGuard > 0)
			{
				assert (*pGuard == 0xC0EDBABE);
				++pGuard;
				uGuard -= sizeof (uint32_t);
			}
		}
		#endif // VSA_GUARD_SIZE > 0

		// Return the allocated memory.
		goto allocated;
	}

	// Adjust the size of the block for any guard regions.
	#if VSA_GUARD_SIZE > 0
	a_nBytes += 2 * VSA_GUARD_SIZE;
	#endif // VSA_GUARD_SIZE > 0

	// If there's enough unallocated space in the current chunk,
	// use it.
	if (m_pFreeChunk != NULL && m_nFreeChunk >= a_nBytes)
	{
		// Remember the allocated memory.
		pAlloc = (void *) m_pFreeChunk;

		// Move past this new allocated memory.
		m_pFreeChunk += a_nBytes;
		m_nFreeChunk -= a_nBytes;

		// Print our progress.
		#ifdef VSA_PRINT_PROGRESS
		fprintf (stderr, "\t\t(Found in current chunk, address %08X, "
			"%d bytes left)\n", (int)pAlloc, m_nFreeChunk);
		#endif // VSA_PRINT_PROGRESS

		// Return the allocated memory.
		goto allocated;
	}

	// We'll have to create a new chunk in order to satisfy this
	// allocation request.

	// First, find a place for the rest of this chunk, if it's big enough
	// to be a free-space block.  (Otherwise, it remains unused.)
	if (m_nFreeChunk >= m_nSmallestBlockSize)
	{
		// Set up this chunk to be a free-space block.
		Block oInsertBlock;
		#ifndef NDEBUG
		oInsertBlock.m_pMemory = m_pFreeChunk;
		#endif // !NDEBUG
		oInsertBlock.m_nBytes = m_nFreeChunk;

		// Put this block into the free-space set.
		// Since the placement allocator doesn't actually allocate memory,
		// the insertion can't fail.
		m_oPlacementAllocator.SetAllocatedMemory (m_pFreeChunk,
			m_nFreeChunk);
		Status_t eStatus = g_kNoError;
		#ifndef NDEBUG
		FreeSpaceSet_t::InsertResult oInsertResult =
		#endif // !NDEBUG
			m_setFreeSpace.Insert (eStatus, oInsertBlock);
		assert (eStatus == g_kNoError);
		assert (oInsertResult.m_bInserted);

		// Print our progress.
		#ifdef VSA_PRINT_PROGRESS
		fprintf (stderr, "\t\t(Put rest of chunk, address %08X, %d bytes, "
			"into free-space set)\n", (int)m_pFreeChunk, m_nFreeChunk);
		#endif // VSA_PRINT_PROGRESS

		// Now there's no more free chunk.
		m_pFreeChunk = NULL;
		m_nFreeChunk = 0;
	}

	// Create a new chunk and allocate a portion of it.
	{
		// Calculate the size of the new chunk.
		// Make sure it's big enough to handle this allocation, i.e. in case
		// it's bigger than the configured chunk size.
		size_t nChunkSize = ((a_nBytes + 2 * sizeof (Chunk *) - 1)
			/ sizeof (Chunk *)) * sizeof (Chunk *);
		nChunkSize = Max (nChunkSize, m_nChunkSize);

		// Add a new chunk to our list.
		{
			// Allocate a new chunk.
			Chunk *pNewChunk = (Chunk *) malloc (nChunkSize);
			if (pNewChunk == NULL)
				return NULL;

			// Hook it into our list.
			pNewChunk->m_pNext = m_pChunks;
			m_pChunks = pNewChunk;
		}

		// Allocate a portion of the chunk.
		pAlloc = (void *)(&(m_pChunks->m_aSpace));

		// Print our progress.
		#ifdef VSA_PRINT_PROGRESS
		fprintf (stderr, "\t\t(Allocated new chunk, address %08X, "
			"%d bytes)\n", (int)pAlloc, nChunkSize);
		#endif // VSA_PRINT_PROGRESS

		// The unallocated portion of the new chunk is here.
		m_pFreeChunk = m_pChunks->m_aSpace + a_nBytes;
		assert (((char *)m_pChunks + nChunkSize)	// (Sanity check)
			>= m_pChunks->m_aSpace + a_nBytes);
		m_nFreeChunk = ((char *)m_pChunks + nChunkSize - a_nBytes)
			- m_pChunks->m_aSpace;
	}

	// (All successful memory allocations end up here.
allocated:;

	// That's one more allocation.
	++m_ulAllocated;

	// Guard against writing out of bounds.
	#if VSA_GUARD_SIZE > 0
	{
		uint32_t *pGuard;
		size_t uGuard;

		// Guard the area before the block.
		pGuard = (uint32_t *)pAlloc;
		uGuard = VSA_GUARD_SIZE;
		while (uGuard > 0)
		{
			*pGuard = 0xDEADBEEF;
			++pGuard;
			uGuard -= sizeof (uint32_t);
		}

		// Guard the area after the block.
		pGuard = (uint32_t *)((int)pAlloc + a_nBytes);
		uGuard = VSA_GUARD_SIZE;
		while (uGuard > 0)
		{
			--pGuard;
			*pGuard = 0xC0EDBABE;
			uGuard -= sizeof (uint32_t);
		}

		// Give our caller the area between the guards.
		pAlloc = (void *)((int)pAlloc + VSA_GUARD_SIZE);
	}
	#endif // VSA_GUARD_SIZE > 0

	// Return the allocated memory.
	return pAlloc;
}



// Deallocate previously-allocated memory.
void
VariableSizeAllocator::Deallocate (size_t a_nSize, size_t a_nBytes,
	void *a_pMemory)
{
	// Make sure they gave us a valid size.
	// (This allocator has only one bucket.)
	assert (a_nSize == 0);

	// Print our progress.
	#ifdef VSA_PRINT_PROGRESS
	fprintf (stderr, "\tVariableSizeAllocator::Deallocate(): freed "
		"%d bytes at %08X\n", (int)a_nBytes, (int)a_pMemory);
	#endif // VSA_PRINT_PROGRESS

	// Guard against writing out of bounds.
	#if VSA_GUARD_SIZE > 0
	{
		uint32_t *pGuard;
		size_t uGuard;

		// Guard the area before the block.
		pGuard = (uint32_t *)a_pMemory;
		uGuard = VSA_GUARD_SIZE;
		while (uGuard > 0)
		{
			--pGuard;
			assert (*pGuard == 0xDEADBEEF);
			uGuard -= sizeof (uint32_t);
		}

		// Guard the area after the block.
		pGuard = (uint32_t *)((int)a_pMemory + a_nBytes);
		uGuard = VSA_GUARD_SIZE;
		while (uGuard > 0)
		{
			assert (*pGuard == 0xC0EDBABE);
			++pGuard;
			uGuard -= sizeof (uint32_t);
		}

		// Reclaim the given memory and the guard areas.
		a_pMemory = (void *)((int)a_pMemory - VSA_GUARD_SIZE);
		a_nBytes += 2 * VSA_GUARD_SIZE;
	}
	#endif // VSA_GUARD_SIZE > 0

	// Set up this chunk to be a free-space block.
	Block oInsertBlock;
	#ifndef NDEBUG
	oInsertBlock.m_pMemory = a_pMemory;
	#endif // !NDEBUG
	oInsertBlock.m_nBytes = a_nBytes;

	// Put this block into the free-space set.
	// Since the placement allocator doesn't actually allocate memory,
	// the insertion can't fail.
	m_oPlacementAllocator.SetAllocatedMemory (a_pMemory, a_nBytes);
	Status_t eStatus = g_kNoError;
	#ifndef NDEBUG
	FreeSpaceSet_t::InsertResult oInsertResult =
	#endif // !NDEBUG
		m_setFreeSpace.Insert (eStatus, oInsertBlock);
	assert (eStatus == g_kNoError);
	assert (oInsertResult.m_bInserted);

	// That's one less allocation.
	--m_ulAllocated;

	// If all memory is unallocated, free up our chunks & start over.
	if (m_ulAllocated == 0UL)
		Purge();
}



// Free up all chunks.
void
VariableSizeAllocator::Purge (void)
{
	// Make sure there are no live allocations
	assert (m_ulAllocated == 0UL);

	// Empty the free-space set.
	if (m_setFreeSpace.Size() > 0)
	{
		// Clear the free-space set.
		m_setFreeSpace.Clear();

		// Forget about any recently deallocated memory.
		size_t nBytesUnused = 0;
		(void) m_oPlacementAllocator.GetDeallocatedMemory (nBytesUnused);
	}

	// No more free chunk.
	m_pFreeChunk = NULL;
	m_nFreeChunk = 0;

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



#endif // __VARIABLESIZEALLOCATOR_H__
