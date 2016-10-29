#ifndef MANAGER_H
#define MANAGER_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE   64
#endif

#define LITTLE_ENDIAN_WRITE_2_bytes(Y, X)  uint16_copy_reverse(Y, X);

#define _LITTLE_ENDIAN_WRITE_2_bytes(Y, X) LITTLE_ENDIAN_WRITE_2_bytes(Y, X)\
                                           Y+=2;
                                           
#define LITTLE_ENDIAN_WRITE_4_bytes(Y, X)  uint32_copy_reverse(Y, X);

#define _LITTLE_ENDIAN_WRITE_4_bytes(Y, X) LITTLE_ENDIAN_WRITE_4_bytes(Y, X)\
                                           Y+=4;

typedef struct
  {
    /* pointers */
    char* infile;
    FILE* INFILE;
    char* outfile;
    FILE* OUTFILE;
    char* database;
    char* filetitle;

    /* global behavior booleans are set to zero by default, being global */
    _Bool automatic;  /* whether automatic processing mode is selected */
    _Bool prepend;  /* whether new header is prepended to raw data or overwrites old header */
    _Bool in_place; /* whether old file is overwritten */
    _Bool cautious; /* whether to ask user before overwrite */
    _Bool interactive; /* whether interactive dialogs will be used */
    /* global diagnosis values */
    _Bool padding; /* whether files should be end-padded */
    _Bool prune; /* whether files ending with 00 should be pruned */
    _Bool virtual;
    short int repair;
    uint32_t padbytes;
    uint32_t prunedbytes;

    /* header substructure */

  } WaveData;

typedef struct
  {
    _Bool       is_extensible;
    _Bool       has_fact;
    uint8_t     ichunks;
    uint8_t*    header_in;
    uint8_t*    header_out;
    uint16_t     header_size_in; /* size of header */
    uint16_t     header_size_out; /* size of header */
    uint16_t	sc_format;	/* should be 1 for PCM-code */
    uint16_t	channels;	/* 1 Mono, 2 Stereo */
    uint16_t	byte_p_spl;	/* samplesize*/
    uint16_t	bit_p_spl;	/* 8, 12, 16, or 24 bit */

    uint32_t	chunk_id;	/* 'RIFF' */
    uint32_t	chunk_size;	/* filelen */
    uint32_t	chunk_format;	/* 'WAVE' */

    uint32_t	sub_chunk;	/* 'fmt ' */
    uint32_t	sc_size;	/* length of sub_chunk = 16 */
    uint32_t	sample_fq;	/* frequence of sample */
    uint32_t	byte_p_sec; /* bytes per second */
    uint16_t    wavext;     /* wav extension = 0 */
    uint32_t    fact_chunk; /* 'fact'*/
    uint32_t    fact_length; /* length of fact chunk - 8 in bytes = 4*/
    uint32_t    n_spl;       /* number of samples written out */
    uint32_t	data_chunk;	/* 'data' */
    uint32_t	data_size;	/* samplecount */
    /* RIFF info chunks to be parsed: INAM, IART, ICMT, ICOP, ICRD, IGNR */
    uint8_t INAM[MAX_LIST_SIZE];
    uint8_t IART[MAX_LIST_SIZE];
    uint8_t ICMT[MAX_LIST_SIZE];
    uint8_t ICOP[MAX_LIST_SIZE];
    uint8_t ICRD[MAX_LIST_SIZE];
    uint8_t IGNR[MAX_LIST_SIZE];
 
  } WaveHeader;


/* Function declarations */


WaveHeader    *fixwav(WaveData *info, WaveHeader *header);
int repair_wav(WaveData *info, WaveHeader *header );
void print_fixwav_help();


#endif
