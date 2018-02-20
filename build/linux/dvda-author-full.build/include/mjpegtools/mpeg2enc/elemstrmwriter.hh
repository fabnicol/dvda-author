#ifndef _ELEMSTREAMWRITER_HH
#define _ELEMSTREAMWRITER_HH

/*  (C) 2003 Andrew Stevens */

/*  This Software is free software; you can redistribute it
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

#include "mjpeg_types.h"

class EncoderParams;

class ElemStrmWriter 
{
public:
    ElemStrmWriter( );
    virtual ~ElemStrmWriter() = 0;
    virtual void WriteOutBufferUpto( const uint8_t *buffer, const uint32_t flush_upto ) = 0;
    inline uint64_t Flushed() const { return flushed; }
    
    virtual uint64_t BitCount() = 0;
protected:
    uint64_t flushed;
};



/******************************
 *
 * Elementry stream buffer used to accumulate (byte-aligned) fragments ofencoded video.  Currently
 * each frame has its own buffer.
 *
 *****************************/

class OutputFragBuf
{
public:
	OutputFragBuf();
    virtual ~OutputFragBuf();

    /**************
     *
     * Flush out buffer
     * N.b. attempts to flush in non byte-aligned states are illegal
     * and will abort
     *
     *************/
    virtual void FlushBuffer() = 0;

    /**************
     *
     * Reset buffer - empty buffer discarding current contents.
     *
     * ***********/

    virtual void ResetBuffer() = 0;

    /**************
     *
     * Write rightmost (least significant) n (0<=n<=32) bits of val to current buffer
     *
     *************/
    virtual void PutBits( uint32_t val, int n) = 0;

    inline void AlignBits()
    {
    	if (outcnt!=8)
    		PutBits(0,outcnt);
    }
    inline bool Aligned() const { return outcnt == 8; }
    inline int ByteCount() const { return unflushed; }

protected:
    int unflushed;
    int outcnt;                 // Bits unwritten in current output byte
    uint32_t pendingbits;
};


/******************************
 *
 * Elementry stream buffer used to accumulate (byte-aligned) fragments ofencoded video.  Currently
 * each frame has its own buffer.
 *
 *****************************/

class ElemStrmFragBuf : public OutputFragBuf
{
public:
	ElemStrmFragBuf( ElemStrmWriter &outstrm);
    ~ElemStrmFragBuf(); 

    /**************
     *
     * Flush out buffer
     * N.b. attempts to flush in non byte-aligned states are illegal
     * and will abort
     *
     *************/
    virtual void FlushBuffer();

    /**************
     * 
     * Reset buffer - empty buffer discarding current contents.
     * 
     * ***********/
     
    virtual void ResetBuffer();
    
    /**************
     *
     * Write rightmost (least significant) n (0<=n<=32) bits of val to current buffer 
     *
     *************/
    virtual void PutBits( uint32_t val, int n);
    
private:
    void AdjustBuffer();

protected:
    ElemStrmWriter &writer;
    uint8_t *buffer;            // Output buffer - used to hold byte
                                // aligned output before flushing or
                                // backing up and re-encoding
    int buffer_size;
};


/******************************
 *
 * Elementry stream buffer used to accumulate (byte-aligned) fragments ofencoded video.  Currently
 * each frame has its own buffer.
 *
 *****************************/

class CountOnlyFragBuf : public OutputFragBuf
{
public:
	CountOnlyFragBuf();
    ~CountOnlyFragBuf();

    /**************
     *
     * Flush out buffer
     * N.b. attempts to flush in non byte-aligned states are illegal
     * and will abort
     *
     *************/
    virtual void FlushBuffer();

    /**************
     *
     * Reset buffer - empty buffer discarding current contents.
     *
     * ***********/

    virtual void ResetBuffer();

    /**************
     *
     * Write rightmost (least significant) n (0<=n<=32) bits of val to current buffer
     *
     *************/
    virtual void PutBits( uint32_t val, int n);

};

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
