/*

File:    atsi.c
Purpose: Create an Audio Titleset Information file

dvda-author  - Author a DVD-Audio DVD

(C) Dave Chapman <dave@dchapman.com> 2005

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "errno.h"
#include "structures.h"
#include "audio2.h"
#include "c_utils.h"
#include "commonvars.h"
#include "structures.h"
#include "auxiliary.h"


extern globalData globals;



int get_afmt(fileinfo_t* info, audioformat_t* audioformats, int* numafmts)
{
    int i;
    int found;

    found=0;

    i=0;
    // PATCH 24.08.2009
    if (!info->newtitle)
    {
        while ((i < *numafmts) && (!found))
        {
            if ((info->samplerate==audioformats[i].samplerate) && (info->bitspersample==audioformats[i].bitspersample) && (info->channels==audioformats[i].channels)&& (info->cga==audioformats[i].cga))
            {
                found=1;
            }
            else
            {
                i++;
            }
        }
        if (!found)
        {
            audioformats[i].samplerate=info->samplerate;
            audioformats[i].channels=info->channels;
            audioformats[i].bitspersample=info->bitspersample;
            audioformats[i].cga=info->cga;
            (*numafmts)++;
        }
    }
    else
    {
        audioformats[i].samplerate=info->samplerate;
        audioformats[i].channels=info->channels;
        audioformats[i].bitspersample=info->bitspersample;
        audioformats[i].cga=info->cga;
    }
    return(i);
}

int create_atsi(command_t *command, char* audiotsdir,uint8_t titleset,uint8_t* atsi_sectors, uint16_t * ntitlepics)
{
    #define files command->files[titleset]
    #define ntracks command->ntracks[titleset]
    #define img command->img

    int i,j,k,t,x;
    char basename[CHAR_BUFSIZ+12+1];
    // PATCH 09.07  ATS_ files do not need 3 sectors in most cases
    uint8_t atsi[2048*3];
    int numtitles;
    int ntitletracks[99];
    uint64_t title_length;
    int numafmts=0;
    audioformat_t audioformats[18];
    static uint16_t trackcount, pictitlecount;

    memset(atsi,0,sizeof(atsi));
    memcpy(&atsi[0],"DVDAUDIO-ATS",12);
    uint16_copy(&atsi[32],0x0012);  // DVD Specifications version
    uint32_copy(&atsi[128],0x07ff); // End byte address of ATSI_MAT
    uint32_copy(&atsi[204],1);      // Start sector of ATST_PGCI_UT

    i=256;
    j=0;
    numtitles=0;
    ntitletracks[numtitles]=0;
    while (j < ntracks)
    {

        ntitletracks[numtitles]=1;

        k=get_afmt(&files[j],audioformats,&numafmts);
        j++;

        while ((j < ntracks) && (!files[j].newtitle) && (files[j].samplerate==files[j-1].samplerate) && (files[j].bitspersample==files[j-1].bitspersample) && (files[j].channels==files[j-1].channels))
        {
            ntitletracks[numtitles]++;
            j++;
        }
        numtitles++;
    }



    for (j=0;j<numafmts;j++)
    {
        uint16_copy(&atsi[i],0x0000);  // [200806] 0x0000 if a menu is not generated; otherwise sector pointer from start of audio zone (AUDIO_PP.IFO to last sector of audio system space (here AUDIO_TS.IFO)
        i+=2;
        if (files[j].channels>2)
        {

            switch (audioformats[j].bitspersample)
            {

            case 16:
                atsi[i]=0x00;
                break;
            case 20:
                atsi[i]=0x11;
                break;
            case 24:
                atsi[i]=0x22;
                break;
            default:
                break;
            }

        }
        else
        {

            switch (audioformats[j].bitspersample)
            {

            case 16:
                atsi[i]=0x0f;
                break;
            case 20:
                atsi[i]=0x1f;
                break;
            case 24:
                atsi[i]=0x2f;
                break;
            default:
                break;
            }
        }
        i++;
        if (files[j].channels>2)
        {

            switch (audioformats[j].samplerate)
            {

            case 48000:
                atsi[i]=0x00;
                break;
            case 96000:
                atsi[i]=0x11;
                break;
            case 192000:
                atsi[i]=0x22;
                break;
            case 44100:
                atsi[i]=0x88;
                break;
            case 88200:
                atsi[i]=0x99;
                break;
            case 176400:
                atsi[i]=0xaa;
                break;
            default:
                break;
            }
        }
        else
        {
            switch (audioformats[j].samplerate)
            {

            case 48000:
                atsi[i]=0x0f;
                break;
            case 96000:
                atsi[i]=0x1f;
                break;
            case 192000:
                atsi[i]=0x2f;
                break;
            case 44100:
                atsi[i]=0x8f;
                break;
            case 88200:
                atsi[i]=0x9f;
                break;
            case 176400:
                atsi[i]=0xaf;
                break;
            default:
                break;
            }
        }

        i++;

        atsi[i]= audioformats[j].cga;
        i++;

        atsi[i]=0x00; // ??? Unknown part of audio format
        // PATCH 09.07: off-by-one error (i+=11)
        i+=11; // ??? Padding
        // EOP
    }

    // downmix coefficients
    //[200806] : if a menu is generated: uint8_copy(&atsi[336],0x01);
    uint16_copy(&atsi[384],0x0000);
    uint16_copy(&atsi[386],0x1eff);
    uint16_copy(&atsi[388],0xff1e);
    uint16_copy(&atsi[390],0x2d2d);
    uint16_copy(&atsi[392],0x3cff);
    uint16_copy(&atsi[394],0xff3c);
    uint16_copy(&atsi[396],0x4b4b);
    uint16_copy(&atsi[398],0x0000);


    /* SECTOR 2 */

    i=0x800;
    uint16_copy(&atsi[i],numtitles);
    // [200806] The number numtitles must be equal to number of audio zone titles plus video zone titles linked to. Gapless tracks are packed in the same title.
    i+=2;
    i+=2; // Padding
    i+=4;

    for (j=0;j<numtitles;j++)
    {
        uint16_copy(&atsi[i],0x8000+(j+1)*0x100);
        i+=2;
        uint16_copy(&atsi[i],0x0000); // Unknown.  Maybe 0x0100 for Stereo, 0x0000 for surround
        i+=2;

        // To be filled later - pointer to a following table.
        i+=4;
    }

    k=0;
    int s=0;
    for (j=0;j<numtitles;j++)
    {
        uint32_copy(&atsi[0x808+8*j+4],i-0x800);

        uint16_copy(&atsi[i],0x0000); // Unknown
        i+=2;
        atsi[i]=ntitletracks[j];
        i++;
        atsi[i]=ntitletracks[j];
        i++;
        title_length=0;
        for (t=0;t<ntitletracks[j];t++)
        {
            title_length+=files[k+t].PTS_length;
        }
        uint32_copy(&atsi[i],title_length);
        i+=4;
        uint16_copy(&atsi[i],0x0000);  // Unknown
        i+=2;
        uint16_copy(&atsi[i],0x0010);                 // Pointer to PTS table
        i+=2;
        uint16_copy(&atsi[i],16+20*ntitletracks[j]);  // Pointer to sector table
        i+=2;
        //PATCH 09.09
        if ((img->count) || (img->stillvob) || (img->active))
            uint16_copy(&atsi[i], 16+32*ntitletracks[j]);                 // Pointer to a stillpic data table
        i+=2;

        /* Timestamp and sector records */

        for (t=0;t<ntitletracks[j];t++)
        {

            // These seem to be pointers to a lookup table in the first sector of the ATSI
            x=get_afmt(&files[k],audioformats,&numafmts);
            x=((x*8)<<8)|0x0010;
            if (t==0)
            {
                x|=0xc000;
            }
            uint16_copy(&atsi[i],x);
            i+=2;
            uint16_copy(&atsi[i],0x0000);
            i+=2;
            atsi[i]=t+1;
            i++;
            atsi[i]=0x00;
            i++;
            uint32_copy(&atsi[i],files[k+t].first_PTS);
            i+=4;
            uint32_copy(&atsi[i],files[k+t].PTS_length);
            i+=10;

        }
        /* Sector pointer records */
        for (t=0;t<ntitletracks[j];t++)
        {
            atsi[i]=0x01;
            i+=4;
            uint32_copy(&atsi[i],files[k+t].first_sector);
            i+=4;
            uint32_copy(&atsi[i],files[k+t].last_sector);
            i+=4;
        }
        if ((img->count) || (img->stillvob) || (img->active))
        {

            /* Stills records */

            uint16_t r, u=0,  trackcount_save=trackcount;
            s+=(j)? ntitlepics[j-1]  : 0;
            if (ntitlepics[j]) pictitlecount++;
             if (globals.veryverbose) printf("[MSG]  pictitlecount=%d for ntitlepics[%d]=%d\n", pictitlecount,j,ntitlepics[j]);
	    for (r=0; r < ntitletracks[j]; r++)
	    {
		trackcount++;
		if ((ntitlepics[j] == 0) && (img->npics[trackcount-1] == 0)) continue;  //  This might be taken off in some unclear cases.

		atsi[i++]=pictitlecount;  // title-with-pics rank (1-based)
		if ((img->options) && (img->options[s]) && (img->options[s]->manual))
		  atsi[i]=0x04;
		i++;
		uint16_copy(&atsi[i], 0x06*ntitletracks[j]); // track rank index
		i+=2;
		if (globals.veryverbose) printf("[MSG]  ntitlepics[%d]=%d, ntitletracks[%d]=%d\n", j, ntitlepics[j], j, ntitletracks[j]);
		//if (ntitlepics[j] > ntitletracks[j])  // conditions to be tested
		 uint16_copy(&atsi[i],(ntitletracks[j]-1)*0x6+0x0F+(ntitlepics[j] -1)*0xA ); // track rank index (backup)
		//else
		 //uint16_copy(&atsi[i],(ntitletracks[j]-1)*0x10+0x0F); // track rank index (backup)
		i+=2;
	    }
            trackcount=trackcount_save;

	    for (t=0; t < ntitletracks[j]; t++)
	    {
		trackcount++;
		if (img->npics[trackcount-1] == 0)
		{
	             if (globals.debugging)
		       printf("[INF]  Skipping track with no pics, t=%d, trackcount=%d\n", t, trackcount);
		     continue;
		}
		uint16_t  pictrackcount=0;

		while (pictrackcount < img->npics[trackcount-1])   // while inside a track pic list or if beginning a track that has pics in it
		{

		  atsi[i]=u+1;
		  i+=3;
		  atsi[i]=t+1;
		  i++;

		  if ((img->options) && (img->options[s+u])&&(img->options[s+u]->onset))
		     uint32_copy(&atsi[i],img->options[s+u]->onset*90000);
		  else if ((img->options==NULL) || (img->options[s] == NULL) || (img->options[s]->manual == 0))
		     uint32_copy(&atsi[i],pictrackcount*90000); // defaulting to 1 second intervals
		  i+=4;
		  if ((img->options) && (img->options[s+u]))
		    atsi[i]=img->options[s+u]->starteffect;
		  i++;
		  if ((img->options) && (img->options[s+u]))
		    atsi[i]=img->options[s+u]->endeffect;
		  i++;
		  u++;
		  pictrackcount++;
		}
	    }

        }
        k+=ntitletracks[j];
    }
    uint32_copy(&atsi[0x0804],i-0x801);
    // PATCH 09.07: i > 2048*2 instead of i > 2048
    if (i > 4096)
    {
        *atsi_sectors=3;
    }
    else
    {
        *atsi_sectors=2;
    }

    uint32_copy(&atsi[12],files[ntracks-1].last_sector+(2*(*atsi_sectors))); // Pointer to last sector in ATS (i.e. numsectors-1)
    uint32_copy(&atsi[28],(*atsi_sectors)-1); // Last sector in ATSI
    uint32_copy(&atsi[196],(*atsi_sectors));      // Start sector of ATST_AOBS

    STRING_WRITE_CHAR_BUFSIZ(basename, "ATS_%02d_0.IFO",titleset+1)
    create_file(audiotsdir, basename, atsi, 2048*(*atsi_sectors));
    STRING_WRITE_CHAR_BUFSIZ(basename, "ATS_%02d_0.BUP",titleset+1)
    create_file(audiotsdir, basename, atsi, 2048*(*atsi_sectors));

    return(0);
    #undef files
    #undef ntracks
    #undef img
}
