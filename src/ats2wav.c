/*
  
ats2wav - extract uncompressed LPCM audio from a DVD-Audio disc

Copyright Dave Chapman <dave@dchapman.com> 2005,
revised by Fabrice Nicol <fabnicol@users.sourceforge.net> 2008
for Windows compatibility, directory output and dvda-author integration.

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


#include "ats2wav.h"
#include "ats.h"
#include "c_utils.h"

#include "winport.h"
#include "multichannel.h"
#include "auxiliary.h"
#include "fixwav_manager.h"

extern globalData globals;
extern uint8_t channels[21];

unsigned char wav_header[80]= {'R','I','F','F',   //  0 - ChunkID
                               0,0,0,0,            //  4 - ChunkSize (filesize-8)
                               'W','A','V','E',    //  8 - Format
                               'f','m','t',' ',    // 12 - SubChunkID
                               40,0,0,0,           // 16 - SubChunk1ID  // 40 for WAVE_FORMAT_EXTENSIBLE
                               0xFE, 0xFF,         // 20 - AudioFormat (1=16-bit)
                               2,0,                // 22 - NumChannels
                               0,0,0,0,            // 24 - SampleRate in Hz
                               0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
                               4,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
                               0x10,0,             // 34 - BitsPerSample (16)
                               0x16,0,             // 36 - cbSize Size of extension (22)
                               0x10,0,             // 38 - wValidBitsPerSample (16 bits)
                               0,0,0,0,            // 40 - dwChannelMask
                               0x01,0x00,          // 44 - subFormat 0x1 for PCM
                               0,0,0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71,                    // 46 - GUID (fixed string)
                               'f','a','c','t',    // 60 - "fact"
                               4,0,0,0,            // 64 - ckSize (4)
                               0,0,0,0,            // 68 - dwSampleLength = NumChannels * ckSize/(BitsPerSample/8)
                               'd','a','t','a',    // 72 - ckID
                               0,0,0,0             // 76 - ckSize -> // 79
                              };

// Reverse table (to be used to convert AOBs to WAVs

static const uint8_t  T[2][6][36]=
     {{ {0},
        {0},
        {5, 4, 7, 6, 1, 0, 9, 8, 11, 10, 3, 2},
        {9, 8, 11, 10, 1, 0, 3, 2, 13, 12, 15, 14, 5, 4 ,7, 6},
        {13, 12, 15, 14, 1, 0, 3, 2, 5, 4, 17, 16, 19, 18, 7, 6, 9, 8, 11, 10},
        {9, 8, 11, 10, 1, 0, 3, 2, 13, 12, 15, 14, 17, 16, 19, 18, 5, 4, 7, 6, 21, 20, 23, 22}},
      {{4,  1,  0,  5,  3,  2},
        {8, 1, 0, 9, 3, 2, 10, 5, 4, 11, 7, 6},
        {14, 7, 6, 15, 9, 8, 4, 1, 0, 16, 11, 10, 17, 13, 12, 5, 3, 2},
        {20, 13, 12, 21, 15, 14, 8, 1, 0, 9, 3, 2, 22, 17, 16, 23, 19, 18, 10, 5, 4, 11, 7, 6},
        {26, 19, 18, 27, 21, 20, 12, 1, 0, 13, 3, 2, 14, 5,  4,  28, 23, 22, 29, 25, 24, 15,  7, 6,  16, 9, 8, 17, 11, 10},
        {28, 13, 12, 29, 15, 14,  8, 1, 0,  9, 3, 2, 30, 17, 16, 31, 19, 18, 32, 21, 20, 33, 23, 22, 10, 5, 4, 11, 7,  6, 34, 25, 24, 35, 27, 26 }}
    };

static void deinterleave_24_bit_sample_extended(uint8_t channels, int count, uint8_t *buf)
{
    // Processing 16-bit case
    int i, size=channels*6;
    // Requires C99
    uint8_t _buf[size];

    for (i=0; i < count ; i += size)
        permutation(buf+i, _buf, 1, channels, T, size);

}

static void deinterleave_sample_extended(uint8_t channels, int count, uint8_t *buf)
{
    // Processing 16-bit case
    int x,i, size=channels*4;
    // Requires C99
    uint8_t _buf[size];

    switch (channels)
    {
    case 1:
    case 2:
        for (i=0;i<count;i+= 2 )
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

static void convert_buffer(fileinfo_t *info, uint8_t *buf, int count)
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

inline static int get_pes_packet_audio(FILE *fp, fileinfo_t *info, uint8_t *audio_buf)
{
    int position;
    //static int cc;  // Continuity counter - reset to 0 when pack_in_title=0

    static uint64_t pack_in_title;
    static uint64_t fpout_size;
    static int title;
    int audio_bytes;
    uint8_t PES_packet_len_bytes[2];

    if (fp == NULL) return 0;
    uint64_t offset0 = ftello(fp);

    fseeko(fp, offset0 + 14, SEEK_SET);

    uint8_t buf[4];
    fread(buf, 4, 1, fp);

    FILE *fpout;

    /* got to system header and read first 4 bytes to detect whether pack is start or not */
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB)
    {
        position = FIRST_PACK;
        fpout_size = 0;
        pack_in_title = 0;
         ++title;

        char Title[14] = {0};
        sprintf(Title, "title_%d.wav", title);

        info->filename = filepath(globals.settings.outdir, Title);

        if ((fpout = fopen(info->filename, "wb")) == NULL)
        {
            foutput("[ERR]  Could not open audio file %s\n.        Exiting...\n", info->filename);
            exit(-7);
        }
    }
    else
    {
        /* go to end of sector : if end of file, then last pack, idem if new pack detected ; otherwise middle pack */
        int res = fseeko(fp, 2044, SEEK_CUR);

        if (res != 0)
        {
            position = LAST_PACK;
        }
        else
        {
            int n = fread(buf, 1, 4, fp);

            if (n != 4 || (n == 4 && buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB))
            {
                position = LAST_PACK;
            }
            else
            {
                position = MIDDLE_PACK;
            }
        }
        ++pack_in_title;
    }

    // offset mark here

    // first pack: 14 + (system header, first pack: 18) + (audio_pes_header, first pack extra: 3) + audi_pes_header: 14 + skip lpcm header content to rach sample size info: 7

    fseeko(fp, offset0 + 35 + (position == FIRST_PACK ? 21 : 0), SEEK_SET);

    /***       +14     ***/
        //read_pack_header(fp, &SCR);

    /***       +18  if first  ***/
        // read_system_header(fp);

    /* skipping read_audio_pes_header to identify info */

    /***       info->first/mid/last/pack_lpcm_headerquantity + 4    ***/

    uint8_t sample_size[1] = {0};
    uint8_t sample_rate[1] = {0};

    /* offset_count += */   fread(sample_size, 1, 1, fp);
    /* offset_count += */   fread(sample_rate, 1, 1, fp);

    uint8_t high_nibble = (sample_rate[0] & 0xf0) >> 4;

    switch(high_nibble)
    {
        case 0:
            info->samplerate = 48000;
            break;
        case 0x1:
            info->samplerate = 96000;
            break;
        case 0x2:
            info->samplerate = 192000;
            break;
        case 0x8:
            info->samplerate = 44100;
            break;
        case 0x9:
            info->samplerate = 88200;
            break;
        case 0xa:
            info->samplerate = 176400;
            break;
        default:
            break;
    }

    if (sample_size[0] == 0x0f || sample_size[0] == 0x2f)
    {
        if ((sample_rate[0] & 0xf) != 0xf)
        {
            foutput("%s", "[ERR]  Coherence_test : sample_rate and sample_size are incoherent (no 0xf lower nibble).\n");
        }
    }
    else
    if (sample_size[0] == 0x00 || sample_size[0] == 0x22)
    {
        if ((sample_rate[0] & 0xf) != high_nibble)
        {
            foutput("%s", "[ERR]  Coherence_test : sample_rate and sample_size are incoherent (lower nibble != higher nibble).\n");
        }
    }

    info->bitspersample = (sample_size[0] == 0x2f || sample_size[0] == 0x22) ? 24 : 16;

    fseeko(fp, 1, SEEK_CUR);

    uint8_t channel_assignment[1] = {0};

    fread(channel_assignment, 1, 1, fp);

    info->cga = channel_assignment[0];

    info->channels = (channel_assignment[0] < 21) ? channels[channel_assignment[0]] : 0;

    /* info->PTS_length, info->numsamples and info->numbytes will be unusable, other info fileds OK */

    calc_info(info);

    /* getting back to audio_pes_header start */
    /* AFTER having parsed read_lpcm_header, which is used to identify info members */

    uint64_t offset1 = ftello(fp);

    fseeko(fp, offset0 + 14 + (position == FIRST_PACK) * 18, SEEK_SET);

    switch(position)
    {
        case FIRST_PACK :
            audio_bytes = info->lpcm_payload - info->firstpackdecrement;
            if (globals.maxverbose) fprintf(stderr, DBG "%s %d bytes %d ch %d bits %d kHz\n", "First pack", audio_bytes, info->channels, info->bitspersample, info->samplerate);
            /* skipping audio_pes_header and lpcm_header */
            fseeko(fp, 21 + info->firstpack_lpcm_headerquantity, SEEK_CUR);
            break;

        case MIDDLE_PACK :
            audio_bytes=info->lpcm_payload;
            if (globals.maxverbose) fprintf(stderr, DBG "%s %d bytes %d ch %d bits %d kHz\n", "Middle pack", audio_bytes, info->channels, info->bitspersample, info->samplerate);
            /* skipping audio_pes_header and lpcm_header */
            fseeko(fp, 18 + info->midpack_lpcm_headerquantity, SEEK_CUR);
            break;

        case LAST_PACK :
            /* skipping audio_pes_header start to get to PES_packet_len_bytes --> audio_bytes */
            fseeko(fp, 4, SEEK_CUR);
            if (globals.maxverbose) fprintf(stderr, DBG "Reading PES_plb at offset: %llu\n", ftello(fp));
            fread(PES_packet_len_bytes, 1, 2, fp);
            if (globals.maxverbose) fprintf(stderr, DBG "With values: PES_packet_len_bytes[0] = %d, PES_packet_len_bytes[1] = %d \n", PES_packet_len_bytes[0], PES_packet_len_bytes[1]);
            audio_bytes = (PES_packet_len_bytes[0] << 8 | PES_packet_len_bytes[1]) - info->lastpack_audiopesheaderquantity;
            if (globals.maxverbose) fprintf(stderr, DBG "%s %d bytes %d ch %d bits %d kHz -- PES_plb = %d, info->lastpack_audiopesheaderquantity = %d\n", "Last pack", audio_bytes, info->channels, info->bitspersample, info->samplerate, PES_packet_len_bytes[0] << 8 | PES_packet_len_bytes[1], info->lastpack_audiopesheaderquantity);
            /* skipping rest of audio_pes_header, i.e 8 bytes + lpcm_header */
            fseeko(fp, 12 + info->lastpack_lpcm_headerquantity, SEEK_CUR);
            break;
    }

    offset1 = ftello(fp);
    int res = fread(audio_buf, 1, audio_bytes, fp);
    if (globals.maxverbose) fprintf(stderr, DBG "%lu: Reading %d bytes\n", offset1, res);
    convert_buffer(info, audio_buf, res);

    int nbyteout = fwrite(audio_buf, 1, res, fpout);

    fpout_size += nbyteout;

    if (position == LAST_PACK)
    {
        fseeko(fpout, 0, SEEK_END); // normally no-op yet sometimes useful.
        uint64_t check_size = ftello(fpout);
        if (check_size != fpout_size)
        {
            foutput(ERR  "Audio decoding outfile mismatch. Decoded %lu bytes yet file size audio is %lu bytes.\n", fpout_size, check_size);
        }

        foutput(MSG "Writing %s (%.2Lf MB)...\n", info->filename, (long double) check_size / (long double) (1024 * 1024));
        fclose(fpout);
        info->file_size = check_size;
    }

    fseeko(fp, offset0 + 2048, SEEK_SET);

    return(position);
}


int get_ats_audio()
{
    FILE* fp;

    uint8_t audio_buf[2048];
    uint64_t pack = 0;
    fileinfo_t files;
    int result = 0;

    fp=fopen(globals.aobpath,"rb");

    if (fp == NULL)
    {
        foutput("%s%s%s", ERR "Could not open AOB file *", globals.aobpath,"*\n");
        return(-1);
    }

    do
    {
        result  = get_pes_packet_audio(fp, &files, audio_buf);
        if (result  == LAST_PACK || result  == FIRST_PACK || result  == MIDDLE_PACK)
        {
            ++pack;
        }
    }
    while (result != LAST_PACK);

    foutput(MSG "Read %lu PES packets.\n", pack);

    WaveData info;
    WaveHeader header;

    info.automatic = true;
    info.prepend = true;
    info.in_place = false;
    info.prune = false;
    info.infile = filestat(false, 0, files.filename, NULL);
    info.outfile = filestat(false, 0, filepath(globals.settings.tempdir, "temp"), NULL);

    header.wBitsPerSample = files.bitspersample;
    header.channels = files.channels;
    header.dwSamplesPerSec = files.samplerate;

    WaveHeader* res;

    res = fixwav(&info, &header);

    if (res == NULL) return(-1);

    errno = 0;
    unlink(filename(info.infile));
    rename(filename(info.outfile), filename(info.infile));
    if (errno) perror(ERR);

    return(errno);
}


#if 0

void calc_size(_fileinfo_t* info)
{
    uint64_t x=0;
    
    info->numsamples=(info->pts_length*info->samplerate)/90000;
    if (info->samplerate)
        x=(90000*info->numsamples)/info->samplerate;
    else
    {
        foutput("%s", ERR "Found null samplerate. Exiting...\n");
        clean_exit(EXIT_FAILURE);
    }
    
    // Adjust for rounding errors:
    if (x < info->pts_length)
    {
        info->numsamples++;
    }
    /*  PATCH
     *  Need for real disc size
     */
    info->numbytes=(info->numsamples*info->channels*info->bitspersample)/8;
    
    
    //     info->numbytes=read_file_size(info->fpout)
}

int setinfo(_fileinfo_t* info, uint8_t buf[4])
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
        printf("%s", ERR "Unsupported bits per sample\n");
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
        printf("%s\n", ERR "Unsupported number of channels, skipping file...\n");
        return(0);
    }
    
    info->channels = T[buf[3]] ;
    
    calc_size(info);
    return(1);
    
}

unsigned int process_data(_fileinfo_t* info, uint8_t* buf, unsigned int count)
{
    unsigned int n;
    
    
    n= (info->byteswritten+count > info->numbytes) ? info->numbytes-info->byteswritten : count;
    
    
    if (!globals.nooutput) fwrite(buf,n,1,info->fpout);
    
    info->byteswritten+=n;
    
    return(n);
}




static void wav_open(_fileinfo_t* info, char* outfile)
{
    
    info->fpout=secure_open(outfile,"wb");
    
    if (!globals.nooutput) fwrite(wav_header,sizeof(wav_header),1,info->fpout);
}


static void wav_close(_fileinfo_t* info , const char* filename)
{
    
    uint64_t filesize=0;
    
    
    if (filesize > UINT32_MAX)
    {
        printf("%s", ERR "WAV standards do not support files > 4 GB--exiting...\n");
        exit(EXIT_FAILURE);
    }
    
    filesize=info->numbytes;
    
    if (filesize == 0)
    {
        printf(WAR "filename: %s\n       filesize is null, closing file...\n", filename);
        fclose(info->fpout);
        return;
    }
    
    if (globals.debugging) printf(MSG "IFO file: %s\n       IFO file size: %"PRIu64"\n", filename, info->numbytes);
    
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



void process_wav_file(char* outfile, const char* outdir, int length, _fileinfo_t * files, int t)
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
#endif



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

int ats2wav(const char GCC_UNUSED * filename, const char GCC_UNUSED *outdir)
{
    
#if 0
    FILE* file=NULL;
    //FILE* fp=NULL;
    
    //unsigned int payload_length=0, ats=1;
    unsigned int t=0, ntracks=0;
    _fileinfo_t files[99];
    //int length=strlen(outdir);
    //int i,k ;
    uint8_t buf[BUFFER_SIZE];
    //uint64_t delta=0;
    uint16_t nbytesread=0; // size must be >= BUFFER_SIZE
    //char outfile[length+1+14];
    //char atstemplate[512]= {0};
    
    /* First check the DVDAUDIO-ATS tag at start of ATS_XX_0.IFO */
    
    file=secure_open(filename, "rb");
    
    
    nbytesread=fread(buf,1,sizeof(buf), file);
    
    if (globals.debugging)
        printf( INF "Read %d bytes\n", nbytesread);
    
    fclose(file);
    
    if (memcmp(buf,"DVDAUDIO-ATS",12)!=0)
    {
        printf(ERR "%s is not an ATSI file (ATS_XX_0.IFO)\n",filename);
        return(EXIT_FAILURE);
    }
    
    printf("%c", '\n');
    
    
    /* now scan tracks to be extracted */
    
    
    ntracks=scan_ats_ifo(files, buf);
    
    if (globals.maxverbose) EXPLAIN("%s%d%s\n", "scanning ", ntracks, "tracks")
            
            //fp=open_aob( fp,  filename,  atstemplate,  ats);
            
            for (t=0; t<ntracks; t++)
    {
        files[t].started=0;
        files[t].byteswritten=0;
    }
    
    t=0;
    
    
    //uint16_t offset=0;
    //_Bool fileend=0;

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
                        process_wav_file(outfile, outdir, length, files, t);
                        
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
                            printf(INF "Extracting %s\n       %dHz, %d bits/sample, %d channels - %lld samples\n",outfile,files[t].samplerate,files[t].bitspersample,files[t].channels,files[t].numsamples);
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
                        fprintf(stderr, MSG "Wrote %d bytes yielding %lld/%lld\n",nbytesread,files[t].byteswritten,files[t].numbytes);
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
                                    
                                    printf(INF "Extracting %s\n       %dHz, %d bits/sample, %d channels - %lld samples\n",outfile,files[t].samplerate,files[t].bitspersample,files[t].channels,files[t].numsamples);
                                    
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
#endif
    return(EXIT_SUCCESS);
}


