/* ElemStrmFragBuf.cc -  bit-level output of MPEG-1/2 elementary video stream */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */
/* Modifications and enhancements (C) 2000/2001 Andrew Stevens */

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


#include "elemstrmwriter.hh"
#include "mpeg2encoder.hh"
#include <cassert>
#include <string.h>

ElemStrmWriter::ElemStrmWriter() 
{
    flushed = BITCOUNT_OFFSET/8;
}

ElemStrmWriter::~ElemStrmWriter()
{
}

/* *********************************************************************** */


OutputFragBuf::OutputFragBuf()
{
    pendingbits = 0;
    unflushed = 0;
    outcnt = 8;
}

OutputFragBuf::~OutputFragBuf()
{
}


/* *********************************************************************** */


ElemStrmFragBuf::ElemStrmFragBuf(ElemStrmWriter &_writer ) :
	OutputFragBuf(),
    writer(_writer)
{
    buffer = NULL;
    ResetBuffer();
}

ElemStrmFragBuf::~ElemStrmFragBuf()
{
	free( buffer );
}

void ElemStrmFragBuf::AdjustBuffer()
{
	buffer_size *= 2;
	buffer = static_cast<uint8_t *>(realloc( buffer, sizeof(uint8_t) * buffer_size));
	if( !buffer )
		mjpeg_error_exit1( "output buffer memory allocation: out of memory" );
}


void ElemStrmFragBuf::ResetBuffer()
{
    outcnt = 8;
    buffer_size = 1024*16;
    unflushed = 0;
    AdjustBuffer();
}

void ElemStrmFragBuf::FlushBuffer( )
{
	assert( outcnt == 8 );
	writer.WriteOutBufferUpto( buffer, unflushed );
    ResetBuffer();
}



/**************
 *
 * Write least significant n (0<=n<=32) bits of val to output buffer
 *
 *************/

void ElemStrmFragBuf::PutBits(uint32_t val, int n)
{
	val = (n == 32) ? val : (val & (~(0xffffffffU << n)));
	while( n >= outcnt )
	{
		pendingbits = (pendingbits << outcnt ) | (val >> (n-outcnt));
		if( unflushed == buffer_size )
			AdjustBuffer();
		buffer[unflushed] = pendingbits;
		++unflushed;
		n -= outcnt;
		outcnt = 8;
	}
	if( n != 0 )
	{
		pendingbits = (pendingbits<<n) | val;
		outcnt -= n;
	}
}


/* *********************************************************************** */


CountOnlyFragBuf::CountOnlyFragBuf() :
	OutputFragBuf()
{
}

CountOnlyFragBuf::~CountOnlyFragBuf()
{

}

void CountOnlyFragBuf::ResetBuffer()
{
    outcnt = 8;
    unflushed = 0;
}

void CountOnlyFragBuf::FlushBuffer( )
{
    unflushed = 0;
}

/**************
 *
 * Write least significant n (0<=n<=32) bits of val to output buffer
 *
 *************/

void CountOnlyFragBuf::PutBits(uint32_t val, int n)
{
	int bits = (8-outcnt)+n;
	int bytes = bits / 8;
	int remainder = bits % 8;
	unflushed += bytes;
	outcnt = 8-remainder;
}




/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
