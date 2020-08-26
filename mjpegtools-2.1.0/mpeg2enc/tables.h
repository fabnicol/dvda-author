/* tables.h, Tables for MPEG syntax                        */

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


/* global syntax look-up tables */

#include "mjpeg_types.h"

#ifdef __cplusplus
#define CLINKAGE "C"
#else
#define CLINKAGE
#endif

#define EXTERNTBL extern CLINKAGE


EXTERNTBL const char version[];

EXTERNTBL const char author[];


EXTERNTBL const uint8_t zig_zag_scan[64]; /* zig-zag Macroblock scan */

EXTERNTBL const uint8_t alternate_scan[64]; /* alternate Macroblock scan */

/* default intra quantization matrix */
EXTERNTBL const uint16_t default_intra_quantizer_matrix[64];

EXTERNTBL const uint16_t hires_intra_quantizer_matrix[64];

EXTERNTBL const uint16_t default_nonintra_quantizer_matrix[64];

EXTERNTBL const uint16_t kvcd_intra_quantizer_matrix[64];

EXTERNTBL const uint16_t kvcd_nonintra_quantizer_matrix[64];

EXTERNTBL const uint16_t flat_intra_quantizer_matrix[64];

EXTERNTBL const uint16_t flat_nonintra_quantizer_matrix[64];

EXTERNTBL const uint16_t tmpgenc_intra_quantizer_matrix[64];

EXTERNTBL const uint16_t tmpgenc_nonintra_quantizer_matrix[64];

/* Hires non intra quantization matrix.  This *is* the MPEG
	default...  */
EXTERNTBL const uint16_t *hires_nonintra_quantizer_matrix;


EXTERNTBL const char pict_type_char[6];


/* Support for the picture layer(!) insertion of scan data fieldsas
   MPEG user-data section as part of I-frames.  */

EXTERNTBL const uint8_t dummy_svcd_scan_data[14];


EXTERNTBL const uint8_t map_non_linear_mquant[113];
EXTERNTBL const uint8_t non_linear_mquant_table[32];
