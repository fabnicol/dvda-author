/*

decoder - extract uncompressed LPCM audio from a DVD-Audio disc

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> 2012

Rewrite of Dave Chapman's initial ats2wav code, summer 2012.

The latest version can be found at http://dvd-audio.sourceforge.net

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if 0

#include "libats2wav.h"
#include "c_utils.h"

#include "winport.h"
#include "multichannel.h"
#include "auxiliary.h"
#include <string.h>

extern globalData globals;


static uint8_t  T[2][6][36]= {{ {0}, {0},
        {5, 4, 7, 6, 1, 0, 9, 8, 11, 10, 3, 2},
        {9, 8, 11, 10, 1, 0, 3, 2, 13, 12, 15, 14, 5, 4 ,7, 6},
        {13, 12, 15, 14, 1, 0, 3, 2, 5, 4, 17, 16, 19, 18, 7, 6, 9, 8, 11, 10},
        {9, 8, 11, 10, 1, 0, 3, 2, 13, 12, 15, 14, 17, 16, 19, 18, 5, 4, 7, 6, 21, 20, 23, 22}
    },
    {   {4,  1,  0,  5,  3,  2},
        {8, 1, 0, 9, 3, 2, 10, 5, 4, 11, 7, 6},
        {14, 7, 6, 15, 9, 8, 4, 1, 0, 16, 11, 10, 17, 13, 12, 5, 3, 2},
        {20, 13, 12, 21, 15, 14, 8, 1, 0, 9, 3, 2, 22, 17, 16, 23, 19, 18, 10, 5, 4, 11, 7, 6},
        {26, 19, 18, 27, 21, 20, 12, 1, 0, 13, 3, 2, 14, 5,  4,  28, 23, 22, 29, 25, 24, 15,  7, 6,  16, 9, 8, 17, 11, 10},
        {28, 13, 12, 29, 15, 14,  8, 1, 0,  9, 3, 2, 30, 17, 16, 31, 19, 18, 32, 21, 20, 33, 23, 22, 10, 5, 4, 11, 7,  6, 34, 25, 24, 35, 27, 26 }
    }
};



/*
ALWAYS_INLINE_GCC void calc_size(_fileinfo_t* info)
{
    uint64_t x=0;

    info->numsamples=(info->pts_length*info->samplerate)/90000;
    if (info->samplerate)
        x=(90000*info->numsamples)/info->samplerate;
    else
    {
        foutput("%s", "[ERR]  Found null samplerate. Exiting...\n");
        clean_exit(EXIT_FAILURE);
    }

    // Adjust for rounding errors:
    if (x < info->pts_length)
    {
        info->numsamples++;
    }

    info->numbytes=(info->numsamples*info->channels*info->bitspersample)/8;


//     info->numbytes=read_file_size(info->fpout)
}
*/

/*  REPLACED BY:

int calc_info(fileinfo_t* info)
{
//PATCH: provided for null dividers.
    if ((info->samplerate)*(info->channels) == 0)
    {
        foutput("%s\n", "[ERR]  Null audio characteristics");
        return(NO_AFMT_FOUND);
    }




// assemble numbers for the various combinations
    short int table_index=(info->bitspersample == 24)? 1 : 0 ;

    static uint16_t T[2][6][10]=     // 16-bit table
    {
        {{	2,	2000, 16,  1984,  2010,	2028, 11, 16, 0, 0 },
            {	4,	2000, 16,  1984,  2010,	2028, 11, 16, 0, 0 },
            { 12,	2004, 24,  1980,  2010,	2028, 15, 12, 0, 0 },
            { 16,	2000, 16,  1980,  2010,	2028, 11, 16, 0, 0 },
            { 20,	2000, 20,  1980,  2010, 2028, 15, 16, 0, 0 },
            { 24,	1992, 24,  1992, 1993,  2014, 10, 10, 17, 14}},
        // 24-bit table
        {{	6,	2004, 24,  1980,  2010,	2028, 15, 12,  0, 0 },
            { 12,	2004, 24,  1980,  2010,	2028, 15, 12,  0, 0 },
            { 18,	1998, 18,  1980,  2010,	2026, 15, 16,  0, 2 },
            { 24,	1992, 24,  1968,  1993,	2014, 10, 10, 17, 14 },
            { 30,	1980,  0,  1980,  2010, 2008, 15, 16,  0, 20 },
            { 36,	1980,  0,  1980,  2010, 2008, 15, 16,  0, 20 }}
    };




#define X T[table_index][info->channels-1]


    info->sampleunitsize=X[0];
    info->lpcm_payload=X[1];
    info->firstpackdecrement=X[2];
    info->SCRquantity=X[3];
    info->firstpack_audiopesheaderquantity=X[4];
    info->midpack_audiopesheaderquantity=X[5];
    info->firstpack_lpcm_headerquantity=X[6];
    info->midpack_lpcm_headerquantity=X[7];
    info->firstpack_pes_padding=X[8];
    info->midpack_pes_padding=X[9];

#undef X



    info->bytespersecond=(info->samplerate*info->bitspersample*info->channels)/8;


    switch (info->samplerate)
    {
    case 44100:
    case 48000:
        info->bytesperframe = 5*info->channels*info->bitspersample;
        break;
    case 88200:
    case 96000:
        info->bytesperframe = 10*info->channels*info->bitspersample;
        break;

    case 176400:
    case 192000:
        info->bytesperframe = 20*info->channels*info->bitspersample;
        break;

    }


    info->numsamples=(info->numbytes/info->sampleunitsize)*info->sampleunitsize/(info->channels*info->bitspersample/8);

    info->PTS_length=(90000.0*info->numsamples)/info->samplerate;

    // Patch : padding/pruning is now done in buffers (following version S)

    return(AFMT_WAVE);
}
*/
ALWAYS_INLINE_GCC int setinfo(_fileinfo_t* info, uint8_t buf[4])
{

    switch (buf[0]&0xf0)
    {
    case 0x00:
        info->bitspersample=16;
        break;

    case 0x10:
        info->bitspersample=20;
        break;

    case 0x20:
        info->bitspersample=24;
        break;

    default:
        printf("%s", "[ERR]  Unsupported bits per sample\n");
        info->bitspersample=0;
        break;
    }

    switch (buf[1]&0xf0)
    {
    case 0x00:
        info->samplerate=48000;
        break;

    case 0x10:
        info->samplerate=96000;
        break;

    case 0x20:
        info->samplerate=192000;
        break;

    case 0x80:
        info->samplerate=44100;
        break;

    case 0x90:
        info->samplerate=88200;
        break;

    case 0xa0:
        info->samplerate=176400;
        break;
    }

    int T[21] = {1, 2, 3, 4, 3, 4, 5, 3, 4, 5, 4, 5, 6, 4, 5, 4, 5, 6, 5, 5, 6 };

    if (buf[3] > 20)

    {
        printf("%s\n", "[ERR] Unsupported number of channels, skipping file...\n");
        return(0);
    }

    info->channels = T[buf[3]] ;

    calc_info(info);
    return(1);

}

ALWAYS_INLINE_GCC unsigned int write_data(_fileinfo_t* info, uint8_t* buf, unsigned int count)
{
    unsigned int n;


    n= (info->byteswritten+count > info->numbytes) ? info->numbytes-info->byteswritten : count;


    if (!globals.nooutput) fwrite(buf,n,1,info->fpout);

    info->byteswritten+=n;

    return(n);
}

/*	 Convert LPCM samples to little-endian WAV samples and deinterleave.

Here the interleaving that is performed during dvd_audio authoring is reversed so as to recover the proper byte order
for a wave file.  The transformation for each of the 12 cases is specified by the following.
A "round trip," i.e., authoring followed by extraction, is now illustrated for the 16-bit, 3-channel case.

authoring:
WAV:  0  1  2  3  4  5  6  7  8  9 10 11
AOB:  5  4 11  10 1  0  3  2  7  6  9  8

extraction:
AOB: 0  1  2  3  4  5  6  7  8  9  10  11
WAV: 5  4  7  6  1  0  9  8  11  10  3  2

These values are encoded in T matrix to be found in src/include/multichannel.h

 */

/*
 16-bit 1  channel
AOB: 0  1
WAV: 1  0

 16-bit 2  channel
AOB: 0  1
WAV: 1  0
*/

/*
 16-bit 3  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11
WAV: 5  4  7  6  1  0  9  8  11  10  3  2
*/

/*
 16-bit 4  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15
WAV: 9  8  11  10  1  0  3  2  13  12  15  14  5  4  7  6
*/


/*
 16-bit 5  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19
WAV: 13  12  15  14  1  0  3  2  5  4  17  16  19  18  7  6  9  8  11  10
*/


/*
 16-bit 6  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23
WAV: 9  8  11  10  1  0  3  2  13  12  15  14  17  16  19  18  5  4  7  6  21  20  23  22
*/


/*
 24-bit 1  channel
AOB: 0  1  2  3  4  5
WAV: 4  1  0  5  3  2
*/


/*
 24-bit 2  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11
WAV: 8  1  0  9  3  2  10  5  4  11  7  6
*/


/*
 24-bit 3  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17
WAV: 14  7  6  15  9  8  4  1  0  16  11  10  17  13  12  5  3  2
*/


/*
 24-bit 4  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23
WAV: 20  13  12  21  15  14  8  1  0  9  3  2  22  17  16  23  19  18  10  5  4  11  7  6
*/

/*
 24-bit 5  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29
WAV: 26  19  18  27  21  20  12  1  0  13  3  2  14  5  4  28  23  22  29  25  24  15  7  6  16  9  8  17  11  10
*/
/*
 24-bit 6  channel
AOB: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35
WAV: 28  13  12  29  15  14  8  1  0  9  3  2  30  17  16  31  19  18  32  21  20  33  23  22  10  5  4  11  7  6  34  25  24  35  27  26
*/




ALWAYS_INLINE_GCC static void deinterleave_24_bit_sample_extended(uint8_t channels, int count, uint8_t *buf)
{
    // Processing 16-bit case
    int i, size=channels*6;
    // Requires C99
    uint8_t _buf[size];

    for (i=0; i < count ; i += size)
        permutation(buf+i, _buf, 1, channels, T, size);

}

ALWAYS_INLINE_GCC static void deinterleave_sample_extended(uint8_t channels, int count, uint8_t *buf)
{

    // Processing 16-bit case
    int x,i, size=channels*4;
    // Requires C99
    uint8_t _buf[size];

    switch (channels)
    {
    case 1:
    case 2:
        for (i=0; i<count; i+= 2 )
        {
            x= buf[i ];
            buf[i ] = buf[i+ 1 ];
            buf[i+ 1 ]=x;
        }
        break;

    default:
        for (i=0; i < count ; i += size)
            permutation(buf+i, _buf, 0, channels, T, size);

    }
}



ALWAYS_INLINE_GCC static void convert_buffer(_fileinfo_t* info, uint8_t* buf, int count)
{

    switch (info->bitspersample)
    {

    case 24:


        deinterleave_24_bit_sample_extended(info->channels, count, buf);
        break;

    case 16:


        deinterleave_sample_extended(info->channels, count, buf);
        break;

    default:
        // FIX: Handle 20-bit audio and maybe convert other formats.
        printf("[ERR]  %d bit %d channel audio is not supported\n",info->bitspersample,info->channels);
        return;
        //exit(EXIT_FAILURE);

    }

}







ALWAYS_INLINE_GCC static void wav_close(_fileinfo_t* info , const char* filename)
{

    uint64_t filesize=0;


    if (filesize > UINT32_MAX)
    {
        printf("%s", "[ERR]  WAV standards do not support files > 4 GB--exiting...\n");
        exit(EXIT_FAILURE);
    }

    filesize=info->numbytes;

    if (filesize == 0)
    {
        printf("[WAR]  filename: %s\n       filesize is null, closing file...\n", filename);
        fclose(info->fpout);
        return;
    }

    if (globals.debugging) printf("[MSG]  IFO file: %s\n       IFO file size: %"PRIu64"\n", filename, info->numbytes);

    fseek(info->fpout,0,SEEK_SET);

    // ChunkSize

    uint32_copy_reverse(wav_header+4, filesize-8+44);

    wav_header[22]=info->channels;

    // Samplerate
    uint32_copy_reverse(wav_header+24, info->samplerate);

    // ByteRate

    uint32_copy_reverse(wav_header+28, (info->samplerate*info->bitspersample*info->channels)/8);

    wav_header[32]=(info->channels*info->bitspersample)/8;
    wav_header[34]=info->bitspersample;

    // Subchunk2Size
    uint32_copy_reverse(wav_header+40, (uint32_t) filesize);

    if (!globals.nooutput) fwrite(wav_header,sizeof(wav_header), 1, info->fpout);
    fclose(info->fpout);
}



FILE* open_aob(FILE* fp, const char* filename, char* atstemplate, int ats)
{
    int length=strlen(filename);
    char atsfilename[length+1];


    memcpy(atstemplate,filename,length-5);
    memcpy(&atstemplate[length-5],"%d.AOB",6);
    atstemplate[length+1]=0;
    sprintf(atsfilename,atstemplate,ats);


    fp=fopen(atsfilename,"rb");


    return(fp);
}



int scan_ats_ifo(_fileinfo_t * files, uint8_t *buf)
{
    int i,j,k,t=0,ntracks,ntracks1, numtitles;


    i=2048;
    numtitles=uint16_read(buf+i);

    uint8_t titleptr[numtitles];

    i+=8;
    ntracks=0;

    for (j=0; j<numtitles; j++)
    {

        i+=4;
        titleptr[j]=uint32_read(buf+i);
        i+=4;
    }

    for (j=0; j<numtitles; j++)
    {
        i=0x802+titleptr[j];
        ntracks1=buf[i];
        i+=14;

        t=ntracks;

        for (k=0; k<ntracks1; k++)
        {
            i+=10;
            files[t].pts_length=uint32_read(buf+i);
            i+=10;
            t++;
        }

        t=ntracks;
        /* 12 byte sector records */
        if (globals.debugging)
            for (k=0; k<ntracks1; k++)
            {

                i+=4;
                files[t].first_sector=uint32_read(buf+i);
                i+=4;
                files[t].last_sector=uint32_read(buf+i);
                i+=4;
                t++;
            }

        ntracks+=ntracks1;
    }
    if (globals.debugging)
        for (i=0; i<ntracks; i++)
        {
            printf("     track first sector  last sector   pts length\n     %02d    %12"PRIu64" %12"PRIu64" %12"PRIu64"\n\n",i+1,files[i].first_sector,files[i].last_sector,files[i].pts_length);
        }

    return(ntracks);
}

void write_wav_file(char* outfile, const char* outdir, int length, _fileinfo_t * files, int t)
{
    char tmp[length+1+14];
    memcpy(tmp, outdir, length);

    memcpy(&tmp[length], "/track%02d.wav", 14);
    tmp[length+14]=0;
    sprintf(outfile, tmp ,t+1);
    EXPLAIN_DEV("Creating new wav file: rank =" , t+1)
    EXPLAIN_DEV("Writing default wav header: bytes =", 44)
    wav_open(&files[t],outfile);
    return;
}

int ats2wav(const char* filename, const char* outdir, extractlist *extract)
{
    FILE* file=NULL;
    FILE* fp=NULL;

    unsigned int payload_length=0, ats=1, t=0, ntracks=0;
    _fileinfo_t files[99];
    int length=strlen(outdir);
    int i,k ;
    uint8_t buf[BUFFER_SIZE];
    uint64_t delta=0;
    uint16_t nbytesread=0; // size must be >= BUFFER_SIZE
    char outfile[length+1+14];
    char atstemplate[512]= {0};

    /* First check the DVDAUDIO-ATS tag at start of ATS_XX_0.IFO */

    file=secure_open(filename, "rb");


    nbytesread=fread(buf,1,sizeof(buf), file);

    if (globals.debugging)
        printf( "[INF]  Read %d bytes\n", nbytesread);

    fclose(file);

    if (memcmp(buf,"DVDAUDIO-ATS",12)!=0)
    {
        printf("[ERR]  %s is not an ATSI file (ATS_XX_0.IFO)\n",filename);
        return(EXIT_FAILURE);
    }

    printf("%c", '\n');

    /* now scan tracks to be extracted */

    ntracks=scan_ats_ifo(files, buf);

    EXPLAIN("%s%d%s\n", "scanning ", ntracks, "tracks")

    fp=open_aob( fp,  filename,  atstemplate,  ats);

    for (t=0; t<ntracks; t++)
    {
        files[t].started=0;
        files[t].byteswritten=0;
    }

    t=0;


    uint16_t offset=0;
    _Bool fileend=0;

    while (t < ntracks)
    {

        if ((extract) && (!extract->extracttrackintitleset[ats-1][t]))
        {
            t++;
            continue;
        }

NEXT:

        /* read AOB into buf, when no longer possible, close AOB and try to open a new one, if not possible proceed with read bytes */

        while ((nbytesread=fread(buf+offset,1,2048-offset,fp)) != 2048-offset)
        {
            if (feof(fp))
            {
                fileend=1;
                EXPLAIN("%s\n","Reached en of file. Closing...")
                fclose(fp);
            }
            else
            {
                EXPLAIN("%s\n","Dit not reach of file yet reading issues. Proceeding...")
                break;
            }


            ats++;
            fp=open_aob(fp, filename, atstemplate, ats);
            if (fp == NULL)
            {
                EXPLAIN_DEV("No AOB with rank = ", (int)ats)
                break;
            }

            else
                offset=nbytesread;
            EXPLAIN_DEV("Opening AOB with rank = ", (int)ats)
            EXPLAIN_DEV("At offset = ", offset)
        };

        offset=0;


        i=0;

        /* Now parse buffer looking for 0x000001 sequences */
        while (i < nbytesread-3)
        {

            while ((buf[i]==0x00) && (buf[i+1]==0x00) && (buf[i+2]==0x01))
            {
                i+=3;

                switch (buf[i])
                {
//            case 0xba:
//                i+=11;
//                break;
//            case 0xbb:
//                i+=15;
//                break;

                case 0xbe:

                    EXPLAIN_DEV("Reached OxBE: i=", i)

                case 0xbd:  // Audio PES Packet


                    k=i+3+uint16_read(buf+i+1);

                    i+=6+buf[i+5];


                    if (files[t].started==0)
                    {
                        write_wav_file(outfile, outdir, length, files, t);

                        if (!setinfo(&files[t],&buf[i+7]))
                        {
                            /* skipping file */
                            t++;
                            fclose(fp);
                            /* label jump is preferable here, to avoid testing a skip boolean repetitively for rare cases at end of while loops */
                            goto NEXT;
                            break;
                        }

                        files[t].started=1;
                        if (globals.debugging)
                            printf("[INF]  Extracting %s\n       %dHz, %d bits/sample, %d channels - %lld samples\n",outfile,files[t].samplerate,files[t].bitspersample,files[t].channels,files[t].numsamples);
                    }

                    i+=buf[i+3]+4;

                    payload_length=k-i;

                    EXPLAIN_DEV("Using payload_length = ",(int)payload_length)

                    delta=(payload_length+files[t].byteswritten < files[t].numbytes)? payload_length : files[t].numbytes-files[t].byteswritten;
                    if (fileend) delta=files[t].numbytes-files[t].byteswritten;

                    EXPLAIN_DEV("Filling buffer with:",(int)delta)

                    convert_buffer(&files[t],&buf[i],delta);

                    EXPLAIN_DEV("Now writing bytes to wav file: ",(int)delta)

                    nbytesread=process_data(&files[t],&buf[i],delta);


                    if (globals.veryverbose)
                    {
                        fprintf(stderr, "[MSG]  Wrote %d bytes yielding %lld/%lld\n",nbytesread,files[t].byteswritten,files[t].numbytes);
                    }

                    delta= files[t].numbytes-files[t].byteswritten;

                    if (delta < 0) EXPLAIN_DEV("Extraction issue! Remaining bytes = ", (int)delta)
                        else
                        {
                            if (delta > 0)
                            {


                                EXPLAIN_DEV("Wav file not completed. Remaining bytes = ", (int)delta)
                                if (fileend)
                                    EXPLAIN("%s\n","However, AOB parsing reached EOF. Unknown issue. Closing AOB...")
                                }
                            else
                            {

                                EXPLAIN_DEV("Wav file completed! Remaining bytes =", (int)delta)
                            }
                            if (fileend)
                            {

                                wav_close(&files[t], filename);

                                t++;

                                if (t < ntracks)
                                {
                                    if (nbytesread < payload_length)
                                    {
                                        process_wav_file(outfile, outdir, length, files, t);

                                        files[t].samplerate=files[t-1].samplerate;
                                        files[t].bitspersample=files[t-1].bitspersample;
                                        files[t].channels=files[t-1].channels;
                                        files[t].started=1;
                                        calc_size(&files[t]);

                                        printf("[INF]  Extracting %s\n       %dHz, %d bits/sample, %d channels - %lld samples\n",outfile,files[t].samplerate,files[t].bitspersample,files[t].channels,files[t].numsamples);

                                        nbytesread=process_data(&files[t],&buf[i+nbytesread],payload_length-nbytesread);
                                    }
                                }
                            }
                        }

                    i=k;
                    break;
//                case 0xbe:  // Audio PES Packet
//
//                    i+=3+uint16_read(buf+i+1);
//                    break;
                }

            }
            i++;
        }
    }

    return(EXIT_SUCCESS);
}


aobfile=secure_open(aobfilename, "rb");


    nbytesread=fread(buf,1,sizeof(buf), file);

    if (globals.debugging)
        printf( "[INF]  Read %d bytes\n", nbytesread);

    fclose(file);

    if (memcmp(buf,"DVDAUDIO-ATS",12)!=0)
    {
        printf("[ERR]  %s is not an ATSI file (ATS_XX_0.IFO)\n",filename);
        return(EXIT_FAILURE);
    }

    printf("%c", '\n');




int decode_ats(const char* filename, const char* outdir, extractlist *extract)
{

    FILE* fpout;
    _fileinfo_t* file[9*99];
    char outfile[CHAR_BUFSIZ+15+1];
    int i=0, pack=0, pack_in_file=0, filenum=1;
    uint32_t bytesinbuf=0, n=0, lpcm_payload=0;
    uint8_t audio_buf[AUDIO_BUFFER_SIZE];
    uint64_t pack_in_title=0;


     /* Open the first AOB and initialise the input LPCM buffer */

    STRING_WRITE_CHAR_BUFSIZ(outfile, "%s"SEPARATOR"ATS_%02d_%d.AOB",audiotsdir,titleset,filenum)
    if (!globals.nooutput) fpout=secure_open(outfile,"rb");

    /* Open the first file and initialise the input audio buffer */
    if (audio_open(&files[i], "wb+")!=0)
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Could not open wav file in process_ats.")


    files[i].first_sector=0;

    printf("[INF]  Processing %s\n",outfile);

    while (bytesinbuf < AUDIO_BUFFER_SIZE)
    {
        if (bytesinbuf < lpcm_payload)
        {
            //pack_in_file is not used in write_pes_packet
            n=process_pes_packet(fpout,&files[i],audio_buf,bytesinbuf,pack_in_title,pack_in_file);

            memmove(audio_buf,&audio_buf[n],bytesinbuf-n);
            bytesinbuf -= n;

            pack++;
            pack_in_title++;
            pack_in_file++;
        }

        if ((pack > 0) && ((pack%(512*1024))==0))
        {
            fclose(fpout);
            filenum++;
            STRING_WRITE_CHAR_BUFSIZ(outfile, "%s"SEPARATOR"ATS_%02d_%d.AOB",audiotsdir,titleset,filenum)
            if (!globals.nooutput) fpout=fopen(outfile,"wb+");
        }

        if (bytesinbuf < lpcm_payload)
        {
            n=process_audio(&files[i],&audio_buf[bytesinbuf],sizeof(audio_buf)-bytesinbuf);
            bytesinbuf+=n;
            if (n==0)   /* We have reached the end of the input file */
            {
                files[i].last_sector=pack;
                audio_close(&files[i]);
                i++;
                pack_in_file=-1;

                if (i<ntracks)
                {
                    /* If the current track is a different audio format, we must start a new title. */
                    if ((files[i].samplerate!=files[i-1].samplerate) || (files[i].channels!=files[i-1].channels) || (files[i].bitspersample!=files[i-1].bitspersample) || (files[i].newtitle))
                    {
                        n=process_pes_packet(fpout,&files[i-1],audio_buf,bytesinbuf,pack_in_title,pack_in_file,ioflag); // Empty audio buffer.
                        pack++;
                        pack_in_title=0;
                        pack_in_file=0;
                        bytesinbuf=0;


                        files[i].first_PTS=calc_PTS_start(&files[i],pack_in_title);
                    }
                    else
                    {
                        files[i].first_PTS=calc_PTS_start(&files[i],pack_in_title+1);
                    }

                    files[i].first_sector=files[i-1].last_sector+1;
                    if (audio_open(&files[i], "rb")!=0)
                    {
                        foutput("[ERR]  Could not open %s\n",files[i].filename);
                        EXIT_ON_RUNTIME_ERROR
                    }

                    n=process_audio(&files[i],&audio_buf[bytesinbuf],sizeof(audio_buf)-bytesinbuf);
                    bytesinbuf+=n;
                    foutput("[INF]  Processing %s\n",files[i].filename);
                }
                else
                {
                    /* We have reached the last packet of the last file */
                    if (bytesinbuf==0)
                    {
                        files[i-1].last_sector=pack-1;
                    }
                    else
                    {
                        n=process_pes_packet(fpout,&files[i-1],audio_buf,bytesinbuf,pack_in_title,pack_in_file,ioflag); // Empty audio buffer.
                        bytesinbuf=0;
                        pack++;
                        pack_in_title++;
                    }
                }
            }
        }
    }

    if (files[0].single_track) files[0].last_sector=files[ntracks-1].last_sector;

    return(1-filenum);
}

/* Searches sequence of bytes tab[] in file pointer fp and when reaching it, returns, in last arg, pointer to file offset of last tab byte or NULL if fails to find before EOF is reached or if fp == NULL. */

#endif
