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
#include "c_utils.h"

#include "winport.h"
#include "multichannel.h"
#include "auxiliary.h"

extern globalData globals;

unsigned char wav_header[44]= {'R','I','F','F',   //  0 - ChunkID
                               0,0,0,0,            //  4 - ChunkSize (filesize-8)
                               'W','A','V','E',    //  8 - Format
                               'f','m','t',' ',    // 12 - SubChunkID
                               16,0,0,0,           // 16 - SubChunk1ID  // 16 for PCM
                               1,0,                // 20 - AudioFormat (1=16-bit)
                               2,0,                // 22 - NumChannels
                               0,0,0,0,            // 24 - SampleRate in Hz
                               0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
                               4,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
                               16,0,               // 34 - BitsPerSample
                               'd','a','t','a',    // 36 - Subchunk2ID
                               0,0,0,0             // 40 - Subchunk2Size
                              };


#if 0

void calc_size(_fileinfo_t* info)
{
    uint64_t x=0;
    
    info->numsamples=(info->pts_length*info->samplerate)/90000;
    if (info->samplerate)
        x=(90000*info->numsamples)/info->samplerate;
    else
    {
        foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Found null samplerate. Exiting...\n");
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
        printf("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Unsupported bits per sample\n");
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
        printf("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" Unsupported number of channels, skipping file...\n");
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
        printf("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  WAV standards do not support files > 4 GB--exiting...\n");
        exit(EXIT_FAILURE);
    }
    
    filesize=info->numbytes;
    
    if (filesize == 0)
    {
        printf(""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  filename: %s\n       filesize is null, closing file...\n", filename);
        fclose(info->fpout);
        return;
    }
    
    if (globals.debugging) printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  IFO file: %s\n       IFO file size: %"PRIu64"\n", filename, info->numbytes);
    
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
        printf( ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Read %d bytes\n", nbytesread);
    
    fclose(file);
    
    if (memcmp(buf,"DVDAUDIO-ATS",12)!=0)
    {
        printf(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  %s is not an ATSI file (ATS_XX_0.IFO)\n",filename);
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
                            printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Extracting %s\n       %dHz, %d bits/sample, %d channels - %lld samples\n",outfile,files[t].samplerate,files[t].bitspersample,files[t].channels,files[t].numsamples);
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
                        fprintf(stderr, ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Wrote %d bytes yielding %lld/%lld\n",nbytesread,files[t].byteswritten,files[t].numbytes);
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
                                    
                                    printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Extracting %s\n       %dHz, %d bits/sample, %d channels - %lld samples\n",outfile,files[t].samplerate,files[t].bitspersample,files[t].channels,files[t].numsamples);
                                    
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


