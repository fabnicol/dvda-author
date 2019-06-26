/* mpeg2syntaxcodes.h - Identifying bit-patterns and codes  for MPEG2 syntax */

#ifndef _MPEG2SYNTAXCODES_H
#define _MPEG2SYNTAXCODES_H
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

#define PICTURE_START_CODE 0x100L
#define SLICE_MIN_START    0x101L
#define SLICE_MAX_START    0x1AFL
#define USER_START_CODE    0x1B2L
#define SEQ_START_CODE     0x1B3L
#define EXT_START_CODE     0x1B5L
#define SEQ_END_CODE       0x1B7L
#define GOP_START_CODE     0x1B8L
#define ISO_END_CODE       0x1B9L
#define PACK_START_CODE    0x1BAL
#define SYSTEM_START_CODE  0x1BBL
#define FFFF_VBV_DELAY     0xFFFF

/* picture coding type */
enum PICTURE_CODING
{
    I_TYPE=1,
    P_TYPE=2,
    B_TYPE=3,
    NUM_PICT_TYPES
};

#define FIRST_PICT_TYPE I_TYPE
#define LAST_PICT_TYPE B_TYPE


/* picture structure */
enum PICTURE_STRUCT
{
    TOP_FIELD=1,
    BOTTOM_FIELD=2,
    FRAME_PICTURE=3
};

/* macroblock type */
enum MACROBLOCK_CODING_BITS
{
    MB_INTRA=1,
    MB_PATTERN=2,
    MB_BACKWARD=4,
    MB_FORWARD=8,
    MB_QUANT=16
};

/* motion_type */

#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8  2
#define MC_DMV   3

/* mv_format */
#define MV_FIELD 0
#define MV_FRAME 1

/* chroma_format */
#define CHROMA420 1
//#define CHROMA422 2
//#define CHROMA444 3

/* extension start code IDs */

#define SEQ_ID       1
#define DISP_ID      2
#define QUANT_ID     3
#define SEQSCAL_ID   5
#define PANSCAN_ID   7
#define CODING_ID    8
#define SPATSCAL_ID  9
#define TEMPSCAL_ID 10



#endif
