/*
 * Copyright (c) 1991 MPEG/audio software simulation group, All Rights Reserved
 *
 * common.c
 *
 * MPEG/audio coding/decoding software, work in progress
 *   NOT for public distribution until verified and approved by the
 *   MPEG/audio committee.  For further information, please contact
 *   Davis Pan, 508-493-2241, e-mail: pan@3d.enet.dec.com
 *
 * VERSION 4.0
 *   changes made since last update:
 *   date   programmers         comment
 * 2/25/91  Doulas Wong,        start of version 1.0 records
 *          Davis Pa
 * 5/10/91  W. Joseph Carter    Created this file for all common
 *                              functions and global variables.
 *                              Ported to Macintosh and Unix.
 *                              Added Jean-Georges Fritsch's
 *                              "bitstream.c" package.
 *                              Added routines to handle AIFF PCM
 *                              sound files.
 *                              Added "mem_alloc()" and "mem_free()"
 *                              routines for memory allocation
 *                              portability.
 *                              Added routines to convert between
 *                              Apple SANE extended floating point
 *                              format and IEEE double precision
 *                              floating point format.  For AIFF.
 * 02jul91 dpwe (Aware Inc)     Moved allocation table input here;
 *                              Tables read from subdir TABLES_PATH.
 *                              Added some debug printout fns (Write*)
 * 7/10/91 Earle Jennings       replacement of the one float by FLOAT
 *                              port to MsDos from MacIntosh version
 * 8/ 5/91 Jean-Georges Fritsch fixed bug in open_bit_stream_r()
 *10/ 1/91 S.I. Sudharsanan,    Ported to IBM AIX platform.
 *         Don H. Lee,
 *         Peter W. Farrett
 *10/3/91  Don H. Lee           implemented CRC-16 error protection
 *                              newly introduced functions are
 *                              I_CRC_calc, II_CRC_calc and
 *                              update_CRC. Additions and revisions
 *                              are marked with dhl for clarity
 *10/18/91 Jean-Georges Fritsch fixed bug in update_CRC(),
 *                              II_CRC_calc() and I_CRC_calc()
 * 2/11/92  W. Joseph Carter    Ported new code to Macintosh.  Most
 *                              important fixes involved changing
 *                              16-bit ints to long or unsigned in
 *                              bit alloc routines for quant of 65535
 *                              and passing proper function args.
 *                              Removed "Other Joint Stereo" option
 *                              and made bitrate be total channel
 *                              bitrate, irrespective of the mode.
 *                              Fixed many small bugs & reorganized.
 * 3/20/92 Jean-Georges Fritsch  fixed bug in start-of-frame search
 * 6/15/92 Juan Pineda          added refill_buffer(bs) "n"
 *                              initialization
 * 7/08/92 Susanne Ritscher     MS-DOS, MSC6.0 port fixes
 * 7/27/92 Mike Li               (re-)Port to MS-DOS
 * 8/19/92 Soren H. Nielsen     Fixed bug in I_CRC_calc and in
 *                              II_CRC_calc.  Added function: new_ext
 *                              for better MS-DOS compatability
 * 3/10/93 Kevin Peterson       changed aiff_read_headers to handle
 *                              chunks in any order.  now returns
 *                              position of sound data in file.
 * 3/31/93 Jens Spille          changed IFF_* string compares to use
 *                              strcmp()
 * 5/30/93 Masahiro Iwadare	?? the previous modification does not
 *				  work. recovered to the original. ??
 * 8/27/93 Seymour Shlien,      Fixes in Unix and MSDOS ports,
 *         Daniel Lauzon,
 *         Bill Truerniet
 * 2004/7/29 Steven Schultz     Cleanup & modernize.  Remove unused
 *                              (unreferenced) code.
 * 2005/10/4 Steven Schultz     Continuing cleanup.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include        <stdlib.h>
#include        <string.h>
#include	<ctype.h>
#include        "common.h"

/*
 *  Global Variable Definitions
*/

const char *mode_names[4] = { "stereo", "j-stereo", "dual-ch", "single-ch" };
const char *layer_names[3] = { "I", "II", "III" };

double  s_freq[4] = {44.1, 48, 32, 0};

int     bitrate[3][15] = {
          {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
          {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
          {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
        };

double multiple[64] = {
2.00000000000000, 1.58740105196820, 1.25992104989487,
1.00000000000000, 0.79370052598410, 0.62996052494744, 0.50000000000000,
0.39685026299205, 0.31498026247372, 0.25000000000000, 0.19842513149602,
0.15749013123686, 0.12500000000000, 0.09921256574801, 0.07874506561843,
0.06250000000000, 0.04960628287401, 0.03937253280921, 0.03125000000000,
0.02480314143700, 0.01968626640461, 0.01562500000000, 0.01240157071850,
0.00984313320230, 0.00781250000000, 0.00620078535925, 0.00492156660115,
0.00390625000000, 0.00310039267963, 0.00246078330058, 0.00195312500000,
0.00155019633981, 0.00123039165029, 0.00097656250000, 0.00077509816991,
0.00061519582514, 0.00048828125000, 0.00038754908495, 0.00030759791257,
0.00024414062500, 0.00019377454248, 0.00015379895629, 0.00012207031250,
0.00009688727124, 0.00007689947814, 0.00006103515625, 0.00004844363562,
0.00003844973907, 0.00003051757813, 0.00002422181781, 0.00001922486954,
0.00001525878906, 0.00001211090890, 0.00000961243477, 0.00000762939453,
0.00000605545445, 0.00000480621738, 0.00000381469727, 0.00000302772723,
0.00000240310869, 0.00000190734863, 0.00000151386361, 0.00000120155435,
1E-20
};

/*
 * Using the decoded info the appropriate possible quantization per
 * subband table is loaded
*/

int pick_table(fr_ps)   /* choose table, load if necess, return # sb's */
frame_params *fr_ps;
{
        int table, lay, ws, bsp, br_per_ch, sfrq;
        int sblim = fr_ps->sblimit;     /* return current value if no load */

        lay = fr_ps->header->lay - 1;
        bsp = fr_ps->header->bitrate_index;
        br_per_ch = bitrate[lay][bsp] / fr_ps->stereo;
        ws = fr_ps->header->sampling_frequency;
        sfrq = s_freq[ws];
        /* decision rules refer to per-channel bitrates (kbits/sec/chan) */
        if ((sfrq == 48 && br_per_ch >= 56) ||
            (br_per_ch >= 56 && br_per_ch <= 80)) table = 0;
        else if (sfrq != 48 && br_per_ch >= 96) table = 1;
        else if (sfrq != 32 && br_per_ch <= 48) table = 2;
        else table = 3;
        if (fr_ps->tab_num != table) {
           if (fr_ps->tab_num >= 0)
              mem_free((void **)&(fr_ps->alloc));
           fr_ps->alloc = (al_table *) mem_alloc(sizeof(al_table),
                                                         "alloc");
           sblim = read_bit_alloc(fr_ps->tab_num = table, fr_ps->alloc);
        }
        return sblim;
}

int js_bound(lay, m_ext)
int lay, m_ext;
{
static int jsb_table[3][4] =  { { 4, 8, 12, 16 }, { 4, 8, 12, 16},
                                { 0, 4, 8, 16} };  /* lay+m_e -> jsbound */

    if(lay<1 || lay >3 || m_ext<0 || m_ext>3) {
		mjpeg_error_exit1("js_bound bad layer/modext (%d/%d)", lay, m_ext);
    }
    return(jsb_table[lay-1][m_ext]);
}

void hdr_to_frps(fr_ps) /* interpret data in hdr str to fields in fr_ps */
frame_params *fr_ps;
{
layer *hdr = fr_ps->header;     /* (or pass in as arg?) */

    fr_ps->actual_mode = hdr->mode;
    fr_ps->stereo = (hdr->mode == MPG_MD_MONO) ? 1 : 2;
    if (hdr->lay == 2)          fr_ps->sblimit = pick_table(fr_ps);
    else                        fr_ps->sblimit = SBLIMIT;
    if(hdr->mode == MPG_MD_JOINT_STEREO)
        fr_ps->jsbound = js_bound(hdr->lay, hdr->mode_ext);
    else
        fr_ps->jsbound = fr_ps->sblimit;
    /* alloc, tab_num set in pick_table */
}

#ifdef WRITE_DEBUG_INFO
void WriteHdr(fr_ps s)
frame_params *fr_ps;
FILE *s;
{
layer *info = fr_ps->header;

   fprintf(s, "HDR:  s=FFF, id=%X, l=%X, ep=%X, br=%X, sf=%X, pd=%X, ",
           info->version, info->lay, !info->error_protection,
           info->bitrate_index, info->sampling_frequency, info->padding);
   fprintf(s, "pr=%X, m=%X, js=%X, c=%X, o=%X, e=%X\n",
           info->extension, info->mode, info->mode_ext,
           info->copyright, info->original, info->emphasis);
   fprintf(s, "layer=%s, tot bitrate=%d, sfrq=%.1f, mode=%s, ",
           layer_names[info->lay-1], bitrate[info->lay-1][info->bitrate_index],
           s_freq[info->sampling_frequency], mode_names[info->mode]);
   fprintf(s, "sblim=%d, jsbd=%d, ch=%d\n",
           fr_ps->sblimit, fr_ps->jsbound, fr_ps->stereo);
   fflush(s);
}

void WriteBitAlloc(bit_alloc, f_p, s)
unsigned int bit_alloc[2][SBLIMIT];
frame_params *f_p;
FILE *s;
{
int i,j;
int st = f_p->stereo;
int sbl = f_p->sblimit;
int jsb = f_p->jsbound;

    fprintf(s, "BITA ");
    for(i=0; i<sbl; ++i) {
        if(i == jsb) fprintf(s,"-");
        for(j=0; j<st; ++j)
            fprintf(s, "%1x", bit_alloc[j][i]);
    }
    fprintf(s, "\n");   fflush(s);
}

void WriteScale(bit_alloc, scfsi, scalar, fr_ps, s)
unsigned int bit_alloc[2][SBLIMIT], scfsi[2][SBLIMIT], scalar[2][3][SBLIMIT];
frame_params *fr_ps;
FILE *s;
{
int stereo  = fr_ps->stereo;
int sblimit = fr_ps->sblimit;
int lay     = fr_ps->header->lay;
int i,j,k;

        if(lay == 2) {
            fprintf(s, "SFSI ");
            for (i=0;i<sblimit;i++) for (k=0;k<stereo;k++)
                if (bit_alloc[k][i])  fprintf(s,"%d",scfsi[k][i]);
            fprintf(s, "\nSCFs ");
            for (k=0;k<stereo;k++) {
                for (i=0;i<sblimit;i++)
                    if (bit_alloc[k][i])
                        switch (scfsi[k][i]) {
                          case 0: for (j=0;j<3;j++)
                                  fprintf(s,"%2d%c",scalar[k][j][i],
                                          (j==2)?';':'-');
                                  break;
                          case 1:
                          case 3: fprintf(s,"%2d-",scalar[k][0][i]);
                                  fprintf(s,"%2d;",scalar[k][2][i]);
                                  break;
                          case 2: fprintf(s,"%2d;",scalar[k][0][i]);
                        }
                fprintf(s, "\n");
            }
        }
        else{   /* lay == 1 */
            fprintf(s, "SCFs ");
            for (i=0;i<sblimit;i++) for (k=0;k<stereo;k++)
                if (bit_alloc[k][i])  fprintf(s,"%2d;",scalar[k][0][i]);
            fprintf(s, "\n");
        }
}

void WriteSamples(ch, sample, bit_alloc, fr_ps, s)
int ch;
unsigned int sample[SBLIMIT];
unsigned int bit_alloc[SBLIMIT];
frame_params *fr_ps;
FILE *s;
{
int i;
int stereo = fr_ps->stereo;
int sblimit = fr_ps->sblimit;

        fprintf(s, "SMPL ");
        for (i=0;i<sblimit;i++)
                if ( bit_alloc[i] != 0)
                    fprintf(s, "%d:", sample[i]);
        if(ch==(stereo-1) )     fprintf(s, "\n");
        else                    fprintf(s, "\t");
}
#endif

int NumericQ(s) /* see if a string lookd like a numeric argument */
char *s;
{
char    c;

    while( (c = *s++)!='\0' && isspace((int)c)) /* strip leading ws */
        ;
    if( c == '+' || c == '-' )
        c = *s++;               /* perhaps skip leading + or - */
    return isdigit((int)c);
}

#ifdef WRITE_DEBUG_INFO
int BitrateIndex(layr, bRate)   /* convert bitrate in kbps to index */
int     layr;           /* 1 or 2 */
int     bRate;          /* legal rates from 32 to 448 */
{
int     index = 0;
int     found = 0;

    while(!found && index<15)   {
        if(bitrate[layr-1][index] == bRate)
            found = 1;
        else
            ++index;
    }
    if(found)
        return(index);
    else {
        fprintf(stderr, "BitrateIndex: %d (layer %d) is not a legal bitrate\n",
                bRate, layr);
        return(-1);     /* Error! */
    }
}

int SmpFrqIndex(sRate)  /* convert samp frq in Hz to index */
long sRate;             /* legal rates 32000, 44100, 48000 */
{
    if(sRate == 44100L)
        return(0);
    else if(sRate == 48000L)
        return(1);
    else if(sRate == 32000L)
        return(2);
    else {
        fprintf(stderr, "SmpFrqIndex: %ld is not a legal sample rate\n", sRate);
        return(-1);      /* Error! */
    }
}

#endif

/*
 *  Allocate number of bytes of memory equal to "block".
*/

void  *mem_alloc(block, item)
unsigned long   block;
const char      *item;
{
    void    *ptr;

    ptr = (void *) calloc(1, block);

    if (ptr == NULL)
        mjpeg_error_exit1("Unable to allocate %s\n", item);
    return(ptr);
}

/*
 *  Free memory pointed to by "*ptr_addr".
*/

void    mem_free(ptr_addr)
void    **ptr_addr;
{

    if (*ptr_addr != NULL){
        free(*ptr_addr);
        *ptr_addr = NULL;
    }
}

/*
 *  bit_stream.c package
 *  Author:  Jean-Georges Fritsch, C-Cube Microsystems
*/

/*/
  This package provides functions to write (exclusive or read)
  information from (exclusive or to) the bit stream.

  If the bit stream is opened in read mode only the get functions are
  available. If the bit stream is opened in write mode only the put
  functions are available.
*/

/*open_bit_stream_w(); open the device to write the bit stream into it    */
/*open_bit_stream_r(); open the device to read the bit stream from it     */
/*close_bit_stream();  close the device containing the bit stream         */
/*alloc_buffer();      open and initialize the buffer;                    */
/*desalloc_buffer();   empty and close the buffer                         */
/*back_track_buffer();     goes back N bits in the buffer                 */
/*unsigned int get1bit();  read 1 bit from the bit stream                 */
/*unsigned long getbits(); read N bits from the bit stream                */
/*unsigned long byte_ali_getbits();   read the next byte aligned N bits from*/
/*                                    the bit stream                        */
/*unsigned long look_ahead(); grep the next N bits in the bit stream without*/
/*                            changing the buffer pointer                   */
/*put1bit(); write 1 bit from the bit stream  */
/*put1bit(); write 1 bit from the bit stream  */
/*putbits(); write N bits from the bit stream */
/*byte_ali_putbits(); write byte aligned the next N bits into the bit stream*/
/*unsigned long sstell(); return the current bit stream length (in bits)    */
/*int end_bs(); return 1 if the end of bit stream reached otherwise 0       */
/*int seek_sync(); return 1 if a sync word was found in the bit stream      */
/*                 otherwise returns 0                                      */

/* refill the buffer from the input device when the buffer becomes empty    */
void refill_buffer(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
   register int i=bs->buf_size-2-bs->buf_byte_idx;
   register unsigned long n=1;
   register int index=0;
   char val[2];

   while ((i>=0) && (!bs->eob)) {

      if (bs->format == BINARY)
         n = fread(&bs->buf[i--], sizeof(unsigned char), 1, bs->pt);

      else {
         while((index < 2) && n) {
            n = fread(&val[index], sizeof(char), 1, bs->pt);
            switch (val[index]) {
                  case 0x30:
                  case 0x31:
                  case 0x32:
                  case 0x33:
                  case 0x34:
                  case 0x35:
                  case 0x36:
                  case 0x37:
                  case 0x38:
                  case 0x39:
                  case 0x41:
                  case 0x42:
                  case 0x43:
                  case 0x44:
                  case 0x45:
                  case 0x46:
                  index++;
                  break;
                  default: break;
            }
         }

         if (val[0] <= 0x39)   bs->buf[i] = (val[0] - 0x30) << 4;
                 else  bs->buf[i] = (val[0] - 0x37) << 4;
         if (val[1] <= 0x39)   bs->buf[i--] |= (val[1] - 0x30);
                 else  bs->buf[i--] |= (val[1] - 0x37);
         index = 0;
      }

      if (!n) {
         bs->eob= i+1;
      }

    }
}

/* empty the buffer to the output device when the buffer becomes full */
void empty_buffer(bs, minimum)
Bit_stream_struc *bs;   /* bit stream structure */
int minimum;            /* end of the buffer to empty */
{
   register int i;

#if BS_FORMAT == BINARY
   for (i=bs->buf_size-1;i>=minimum;i--)
      fwrite(&bs->buf[i], sizeof(unsigned char), 1, bs->pt);
#else
   for (i=bs->buf_size-1;i>=minimum;i--) {
       char val[2];
       val[0] = he[((bs->buf[i] >> 4) & 0x0F)];
       val[1] = he[(bs->buf[i] & 0x0F)];
       fwrite(val, sizeof(char), 2, bs->pt);
   }
#endif

   for (i=minimum-1; i>=0; i--)
       bs->buf[bs->buf_size - minimum + i] = bs->buf[i];

   bs->buf_byte_idx = bs->buf_size -1 - minimum;
   bs->buf_bit_idx = 8;
}

/* open the device to write the bit stream into it */
void open_bit_stream_w(bs, bs_filenam, size)
Bit_stream_struc *bs;   /* bit stream structure */
char *bs_filenam;       /* name of the bit stream file */
int size;               /* size of the buffer */
{
	if ( strcmp(bs_filenam, "-" ) == 0 )
	{
		bs->pt = stdout;
	}
	else if ((bs->pt = fopen(bs_filenam, "wb")) == NULL) {
		fprintf(stderr,"Could not create \"%s\".\n", bs_filenam);
		exit(1);
	}
	alloc_buffer(bs, size);
	bs->buf_byte_idx = size-1;
	bs->buf_bit_idx=8;
	bs->totbit=0;
	bs->mode = WRITE_MODE;
	bs->eob = FALSE;
	bs->eobs = FALSE;
}

/* open the device to read the bit stream from it */
void open_bit_stream_r(bs, bs_filenam, size)
Bit_stream_struc *bs;   /* bit stream structure */
char *bs_filenam;       /* name of the bit stream file */
int size;               /* size of the buffer */
{
   register unsigned long n;
   register unsigned char flag = 1;
   unsigned char val;

   if ((bs->pt = fopen(bs_filenam, "rb")) == NULL) {
      printf("Could not find \"%s\".\n", bs_filenam);
      exit(1);
   }

   do {
     n = fread(&val, sizeof(unsigned char), 1, bs->pt);
     switch (val) {
      case 0x30:
      case 0x31:
      case 0x32:
      case 0x33:
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
      case 0x38:
      case 0x39:
      case 0x41:
      case 0x42:
      case 0x43:
      case 0x44:
      case 0x45:
      case 0x46:
      case 0xa:  /* \n */
      case 0xd:  /* cr */
      case 0x1a:  /* sub */
          break;

      default: /* detection of an binary character */
          flag--;
          break;
     }

   } while (flag & n);

   if (flag) {
      printf ("the bit stream file %s is an ASCII file\n", bs_filenam);
      bs->format = ASCII;
   }
   else {
      bs->format = BINARY;
      printf ("the bit stream file %s is a BINARY file\n", bs_filenam);
   }

   fclose(bs->pt);

   if ((bs->pt = fopen(bs_filenam, "rb")) == NULL) {
      printf("Could not find \"%s\".\n", bs_filenam);
      exit(1);
   }

   alloc_buffer(bs, size);
   bs->buf_byte_idx=0;
   bs->buf_bit_idx=0;
   bs->totbit=0;
   bs->mode = READ_MODE;
   bs->eob = FALSE;
   bs->eobs = FALSE;
}

/*close the device containing the bit stream after a read process*/
void close_bit_stream_r(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
   fclose(bs->pt);
   desalloc_buffer(bs);
}

/*close the device containing the bit stream after a write process*/
void close_bit_stream_w(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
   putbits(bs, 0, 7);
   empty_buffer(bs, bs->buf_byte_idx + 1);
   fclose(bs->pt);
   desalloc_buffer(bs);
}

/*open and initialize the buffer; */
void alloc_buffer(bs, size)
Bit_stream_struc *bs;   /* bit stream structure */
int size;
{
   bs->buf = (unsigned char *) mem_alloc(size*sizeof(unsigned
              char), "buffer");
   bs->buf_size = size;
}

/*empty and close the buffer */
void desalloc_buffer(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
   free(bs->buf);
}

int putmask[9]={0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};
int clearmask[9]={0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x0};

void back_track_buffer(bs, N) /* goes back N bits in the buffer */
Bit_stream_struc *bs;   /* bit stream structure */
int N;
{
   int tmp = N - (N/8)*8;
   register int i;

   bs->totbit -= N;
   for (i=bs->buf_byte_idx;i< bs->buf_byte_idx+N/8-1;i++) bs->buf[i] = 0;
   bs->buf_byte_idx += N/8;
   if ( (tmp + bs->buf_bit_idx) <= 8) {
      bs->buf_bit_idx += tmp;
   }
   else {
      bs->buf_byte_idx ++;
      bs->buf_bit_idx += (tmp - 8);
   }
   bs->buf[bs->buf_byte_idx] &= clearmask[bs->buf_bit_idx];
}

int mask[8]={0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

/*read 1 bit from the bit stream */
unsigned int get1bit(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
   unsigned int bit;
   register int i;

   bs->totbit++;

   if (!bs->buf_bit_idx) {
        bs->buf_bit_idx = 8;
        bs->buf_byte_idx--;
        if ((bs->buf_byte_idx < MINIMUM) || (bs->buf_byte_idx < bs->eob)) {
             if (bs->eob)
                bs->eobs = TRUE;
             else {
                for (i=bs->buf_byte_idx; i>=0;i--)
                  bs->buf[bs->buf_size-1-bs->buf_byte_idx+i] = bs->buf[i];
                refill_buffer(bs);
                bs->buf_byte_idx = bs->buf_size-1;
             }
        }
   }
   bit = bs->buf[bs->buf_byte_idx]&mask[bs->buf_bit_idx-1];
   bit = bit >> (bs->buf_bit_idx-1);
   bs->buf_bit_idx--;
   return(bit);
}

/*write 1 bit from the bit stream */
void put1bit(bs, bit)
Bit_stream_struc *bs;   /* bit stream structure */
int bit;                /* bit to write into the buffer */
{
   bs->totbit++;

   bs->buf[bs->buf_byte_idx] |= (bit&0x1) << (bs->buf_bit_idx-1);
   bs->buf_bit_idx--;
   if (!bs->buf_bit_idx) {
       bs->buf_bit_idx = 8;
       bs->buf_byte_idx--;
       if (bs->buf_byte_idx < 0)
          empty_buffer(bs, MINIMUM);
       bs->buf[bs->buf_byte_idx] = 0;
   }
}

/*look ahead for the next N bits from the bit stream */
unsigned long look_ahead(bs, N)
Bit_stream_struc *bs;   /* bit stream structure */
int N;                  /* number of bits to read from the bit stream */
{
 unsigned long val=0;
 register int j = N;
 register int k, tmp;
 register int bit_idx = bs->buf_bit_idx;
 register int byte_idx = bs->buf_byte_idx;

 if (N > MAX_LENGTH)
    printf("Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);

 while (j > 0) {
    if (!bit_idx) {
        bit_idx = 8;
        byte_idx--;
    }
    k = MIN (j, bit_idx);
    tmp = bs->buf[byte_idx]&putmask[bit_idx];
    tmp = tmp >> (bit_idx-k);
    val |= tmp << (j-k);
    bit_idx -= k;
    j -= k;
 }
 return(val);
}

/*read N bit from the bit stream */
unsigned long getbits(bs, N)
Bit_stream_struc *bs;   /* bit stream structure */
int N;                  /* number of bits to read from the bit stream */
{
 unsigned long val=0;
 register int i;
 register int j = N;
 register int k, tmp;

 if (N > MAX_LENGTH)
    printf("Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);

 bs->totbit += N;
 while (j > 0) {
   if (!bs->buf_bit_idx) {
        bs->buf_bit_idx = 8;
        bs->buf_byte_idx--;
        if ((bs->buf_byte_idx < MINIMUM) || (bs->buf_byte_idx < bs->eob)) {
             if (bs->eob)
                bs->eobs = TRUE;
             else {
                for (i=bs->buf_byte_idx; i>=0;i--)
                   bs->buf[bs->buf_size-1-bs->buf_byte_idx+i] = bs->buf[i];
                refill_buffer(bs);
                bs->buf_byte_idx = bs->buf_size-1;
             }
        }
   }
   k = MIN (j, bs->buf_bit_idx);
   tmp = bs->buf[bs->buf_byte_idx]&putmask[bs->buf_bit_idx];
   tmp = tmp >> (bs->buf_bit_idx-k);
   val |= tmp << (j-k);
   bs->buf_bit_idx -= k;
   j -= k;
 }
 return(val);
}

/*write N bits into the bit stream */
void putbits(bs, val, N)
Bit_stream_struc *bs;   /* bit stream structure */
unsigned int val;       /* val to write into the buffer */
int N;                  /* number of bits of val */
{
 register int j = N;
 register int k, tmp;

 if (N > MAX_LENGTH)
    printf("Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);

 bs->totbit += N;
 while (j > 0) {
   k = MIN(j, bs->buf_bit_idx);
   tmp = val >> (j-k);
   bs->buf[bs->buf_byte_idx] |= (tmp&putmask[k]) << (bs->buf_bit_idx-k);
   bs->buf_bit_idx -= k;
   if (!bs->buf_bit_idx) {
       bs->buf_bit_idx = 8;
       bs->buf_byte_idx--;
       if (bs->buf_byte_idx < 0)
          empty_buffer(bs, MINIMUM);
       bs->buf[bs->buf_byte_idx] = 0;
   }
   j -= k;
 }
}

/*write N bits byte aligned into the bit stream */
void byte_ali_putbits(bs, val, N)
Bit_stream_struc *bs;   /* bit stream structure */
unsigned int val;       /* val to write into the buffer */
int N;                  /* number of bits of val */
{
 unsigned long aligning;

 if (N > MAX_LENGTH)
    printf("Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);
 aligning = sstell(bs)%8;
 if (aligning)
     putbits(bs, (unsigned int)0, (int)(8-aligning)); 

 putbits(bs, val, N);
}

/*read the next bute aligned N bits from the bit stream */
unsigned long byte_ali_getbits(bs, N)
Bit_stream_struc *bs;   /* bit stream structure */
int N;                  /* number of bits of val */
{
 unsigned long aligning;

 if (N > MAX_LENGTH)
    printf("Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);
 aligning = sstell(bs)%8;
 if (aligning)
    getbits(bs, (int)(8-aligning));

 return(getbits(bs, N));
}

/*return the current bit stream length (in bits)*/
unsigned long sstell(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
  return(bs->totbit);
}

/*return the status of the bit stream*/
/* returns 1 if end of bit stream was reached */
/* returns 0 if end of bit stream was not reached */
int end_bs(bs)
Bit_stream_struc *bs;   /* bit stream structure */
{
  return(bs->eobs);
}

/*this function seeks for a byte aligned sync word in the bit stream and
  places the bit stream pointer right after the sync.
  This function returns 1 if the sync was found otherwise it returns 0  */
int seek_sync(bs, sync, N)
Bit_stream_struc *bs;   /* bit stream structure */
long sync;      /* sync word maximum 32 bits */
int N;          /* sync word length */
{
 unsigned long aligning;
 unsigned long val;
 long maxi = (int)pow(2.0, (FLOAT)N) - 1;

 aligning = sstell(bs)%ALIGNING;
 if (aligning)
    getbits(bs, (int)(ALIGNING-aligning));

  val = getbits(bs, N);
  while (((val&maxi) != sync) && (!end_bs(bs))) {
        val <<= ALIGNING;
        val |= getbits(bs, ALIGNING);
  }

 if (end_bs(bs)) return(0);
 else return(1);
}

/*
 *  CRC error protection package
*/

void I_CRC_calc(fr_ps, bit_alloc, crc)
frame_params *fr_ps;
unsigned int bit_alloc[2][SBLIMIT];
unsigned int *crc;
{
        int i, k;
        layer *info = fr_ps->header;
        int stereo  = fr_ps->stereo;
        int jsbound = fr_ps->jsbound;

        *crc = 0xffff; /* changed from '0' 92-08-11 shn */
        update_CRC(info->bitrate_index, 4, crc);
        update_CRC(info->sampling_frequency, 2, crc);
        update_CRC(info->padding, 1, crc);
        update_CRC(info->extension, 1, crc);
        update_CRC(info->mode, 2, crc);
        update_CRC(info->mode_ext, 2, crc);
        update_CRC(info->copyright, 1, crc);
        update_CRC(info->original, 1, crc);
        update_CRC(info->emphasis, 2, crc);

        for (i=0;i<SBLIMIT;i++)
                for (k=0;k<((i<jsbound)?stereo:1);k++)
                        update_CRC(bit_alloc[k][i], 4, crc);
}

void II_CRC_calc(fr_ps, bit_alloc, scfsi, crc)
frame_params *fr_ps;
unsigned int bit_alloc[2][SBLIMIT], scfsi[2][SBLIMIT];
unsigned int *crc;
{
        int i, k;
        layer *info = fr_ps->header;
        int stereo  = fr_ps->stereo;
        int sblimit = fr_ps->sblimit;
        int jsbound = fr_ps->jsbound;
        al_table *alloc = fr_ps->alloc;

        *crc = 0xffff; /* changed from '0' 92-08-11 shn */
        update_CRC(info->bitrate_index, 4, crc);
        update_CRC(info->sampling_frequency, 2, crc);
        update_CRC(info->padding, 1, crc);
        update_CRC(info->extension, 1, crc);
        update_CRC(info->mode, 2, crc);
        update_CRC(info->mode_ext, 2, crc);
        update_CRC(info->copyright, 1, crc);
        update_CRC(info->original, 1, crc);
        update_CRC(info->emphasis, 2, crc);

        for (i=0;i<sblimit;i++)
                for (k=0;k<((i<jsbound)?stereo:1);k++)
                        update_CRC(bit_alloc[k][i], (*alloc)[i][0].bits, crc);

        for (i=0;i<sblimit;i++)
                for (k=0;k<stereo;k++)
                        if (bit_alloc[k][i])
                                update_CRC(scfsi[k][i], 2, crc);
}

void update_CRC(data, length, crc)
unsigned int data, length, *crc;
{
        unsigned int  masking, carry;

        masking = 1 << length;

        while((masking >>= 1)){
                carry = *crc & 0x8000;
                *crc <<= 1;
                if (!carry ^ !(data & masking))
                        *crc ^= CRC16_POLYNOMIAL;
        }
        *crc &= 0xffff;
}
