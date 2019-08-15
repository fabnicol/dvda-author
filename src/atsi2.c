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
#include "atsi.h"


extern globalData globals;
    
    
int get_afmt(fileinfo_t* info, audioformat_t* audioformats, int* numafmts) {
  int i;
  int found;

  found = 0;
  i = 0;

  while (i < *numafmts && ! found)
  {
    if (info->samplerate == audioformats[i].samplerate
        && info->bitspersample == audioformats[i].bitspersample
        && info->channels == audioformats[i].channels)
    {
      found = 1;
    }
    else
    {
      i++;
    }
  }

  if (! found)
  {
    audioformats[i].samplerate = info->samplerate;
    audioformats[i].channels = info->channels;
    audioformats[i].cga = info->cga;
    audioformats[i].type = info->type;
    audioformats[i].bitspersample = info->bitspersample;
    (*numafmts)++;
  }

  if (*numafmts == 9)
  {
      EXIT_ON_RUNTIME_ERROR_VERBOSE("DVD-Audio discs cannot manage more than 8 different audio formats per group.\n       Resample tracks or create a new group.")
  }

  return(i);
}

static inline uint16_t downmix_coeff(float db)
{
    //    ...
    //    0xFF off
    //    0xFE = -61.8 dB
    //    ...
    //    0xFE - l = -61.8 + l x 0.4 dB   0 < l < 47 (0x2F)
    //    ...
    //    0xFF - 0x30 = 0xCF = 207 = -42.9 dB
    //    l < 8
    //    0xCF - l = -42.9 + l x 0.4 dB
    //    ...
    //    -42.9, -42.5, -42.1, -41.7, -41.3, -40.9, -40.5, -40.1
    //    ...
    //    0xCF - 7  =  0xC8 = 200 = -40.1 dB
    //    0xC7 = 199 = -41.2 dB
    //    l < 7
    //    0xC7 -l = -41.2 + l x 0.2 dB
    //    -41.2, -41, -40.8, -40.6, -40.4, -40.2, -40
    //    0xC1 = -40 dB
    //    0xC0 = -39.7 dB
    //    l < 14
    //    0xC0 - l = -39.7  + l x 0.2 dB
    //    l < 14
    //    0xB2 - l = -36.8  + l x 0.2 dB
    //    l < 14
    //    0xA4 - l = -33.9  + l x 0.2 dB
    //    l < 15
    //    0x96 - l = -31    + l x 0.2 dB
    //    l < 14
    //    0x87 - l = -27.9  + l x 0.2 dB
    //    l < 14
    //    0x79 - l = -25    + l x 0.2 dB
    //    l < 15
    //    0x6B - l = -22.1  + l x 0.2 dB
    //    l < 14
    //    0x5C - l = -19    + l x 0.2 dB
    //    l < 14
    //    0x4E - l = -16.1  + l x 0.2 dB
    //    l < 14
    //    0x40 - l = -13.2  + l x 0.2 dB
    //    l < 15
    //    0x31 - l = -10.1  + l x 0.2 dB
    //    l < 14
    //    0x23 - l = -7.2   + l x 0.2 dB
    //    l < 14
    //    0x15 - l = -4.3   + l x 0.2 dB
    //    l < 8
    //    0x07 - l = -1.4   + l x 0.2 dB
    //    0x00  = -0 dB

 if (db < 0 || db > 100.0) return 0;

 uint16_t c = 0;
 if (db >= 0 && db <= 1.4)  c =  (uint16_t) db * 5;
 else
 if (db <= 4.3)    c =  (uint16_t) (0x15 - (4.3 - db) / 0.2) ;
 else
 if (db <= 7.2)    c =  (uint16_t) (0x23 - (7.2 - db) / 0.2) ;
 else
 if (db <= 10.1)   c =  (uint16_t) (0x31 - (10.1 - db) / 0.2) ;
 else
 if (db <= 13.2)   c =  (uint16_t) (0x40 - (13.2 - db) / 0.2) ;
 else
 if (db <= 16.1)   c =  (uint16_t) (0x4E - (16.1 - db) / 0.2) ;
 else
 if (db <= 19)     c =  (uint16_t) (0x5C - (19 - db) / 0.2) ;
 else
 if (db <= 22.1)   c =  (uint16_t) (0x6B - (22.1 - db) / 0.2) ;
 else
 if (db <= 25)     c =  (uint16_t) (0x79 - (25 - db) / 0.2) ;
 else
 if (db <= 27.9)   c =  (uint16_t) (0x87 - (27.9 - db) / 0.2) ;
 else
 if (db <= 31)     c =  (uint16_t) (0x96 - (31 - db) / 0.2) ;
 else
 if (db <= 33.9)   c =  (uint16_t) (0xA4 - (33.9 - db) / 0.2) ;
 else
 if (db <= 36.8)   c =  (uint16_t) (0xB2 - (36.8 - db) / 0.2) ;
 else
 if (db <= 39.7)   c =  (uint16_t) (0xC0 - (39.7 - db) / 0.2) ;
 else
 if (db <= 41.2)
 {
     if (db == 40.1) c = 0xC8;
     else
     if (db == 40.5) c = 0xC9;
     else
     if (db == 40.9) c = 0xCA;
     else
     c =  (uint16_t) (0xC7 - (41.2 - db) / 0.2) ;
 }
 else
 if (db <= 42.9)   c =  (uint16_t) (0xCF - (42.9 - db) / 0.4) ;
 else
 if (db <= 61.8)   c =  (uint16_t) (0xFE - (61.8 - db) / 0.4) ;
 else
 if (db == 100.0) c = 0xFF; // off

 return c;
}



int create_atsi(command_t *command, char* audiotsdir,uint8_t titleset,uint8_t* atsi_sectors, uint16_t * ntitlepics)
{
    #define files command->files[titleset]
    #define ntracks command->ntracks[titleset]
    #define img command->img
    #define db command->db

    int i,j,k,t,x;
    char basename[CHAR_BUFSIZ+12+1];
    // PATCH 09.07  ATS_ files do not need 3 sectors in most cases
    uint8_t atsi[2048*3];
    int numtitles;
    int ntitletracks[99];
    uint64_t title_length;
    int numafmts = 0;
    //TODO: is 18 a maximum?
    audioformat_t audioformats[18];
    static uint16_t trackcount, pictitlecount;
    bool maybe_multichannel = false;

    memset(atsi, 0, sizeof(atsi));
    memcpy(&atsi[0], "DVDAUDIO-ATS", 12);
    uint16_copy(&atsi[0x20], 0x0012);  // DVD Specifications version
    uint32_copy(&atsi[0x80], 0x07ff); // End byte address of ATSI_MAT
    uint32_copy(&atsi[0xCC], 1);      // Start sector of ATST_PGCI_UT

    i = 256;
    j = 0;
    numtitles = 0;
    ntitletracks[numtitles] = 0;

    while (j < ntracks)
    {
        ntitletracks[numtitles] = 1;

        get_afmt(&files[j], audioformats, &numafmts);

        ++j;

        while ((j < ntracks) && (j == 0 || ! files[j].newtitle) )
        {
            ++ntitletracks[numtitles];
            ++j;
        }
        ++numtitles;
    }

    for (j = 0; j < numafmts; ++j)
    {
        // TODO: CHECK this as 0X1 was observed (MLP) for menuless disc (3/16/96 DW, G1T3, 2/24/44 DVDA-C).

        uint16_copy(&atsi[i], (audioformats[j].type == AFMT_MLP) << 8);

        // Following loop table is OK for MLP too

        i+=2;

        if (files[j].channels > 2)
        {
            maybe_multichannel = true;

            switch (audioformats[j].bitspersample)
            {
                case 16:
                    atsi[i] = 0x00;
                    break;

                case 20:
                    atsi[i] = 0x11;
                    break;

                case 24:
                    atsi[i] = 0x22;
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
                    atsi[i] = 0x0f;
                    break;

                case 20:
                    atsi[i] = 0x1f;
                    break;

                case 24:
                    atsi[i] = 0x2f;
                    break;

                default:
                    break;
            }
        }
        i++;
        if (files[j].channels > 2)
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

        ++i;

        atsi[i] = files[j].cga;
        ++i;

        atsi[i] = 0x00; // ??? Unknown part of audio format
        // PATCH 09.07: off-by-one error (i+=11)
        i += 11; // ??? Padding
        // EOP
    }

    // downmix coefficients
    //[200806] : if a menu is generated: uint8_copy(&atsi[336],0x01); // TODO check ?
    // is inserted even if non-surround

    if (maybe_multichannel)
    {
        if (! db[0].custom_table)
        {
            for (int w = 0; w < 16; ++w)
            {
        //    uint16_copy(&atsi[384 + w * 18],0x0000); no-op
              uint16_copy(&atsi[386 + w * 18],0x1eff);
              uint16_copy(&atsi[388 + w * 18],0xff1e);
              uint16_copy(&atsi[390 + w * 18],0x2d2d);
              uint16_copy(&atsi[392 + w * 18],0x3cff);
              uint16_copy(&atsi[394 + w * 18],0xff3c);
              uint16_copy(&atsi[396 + w * 18],0x4b4b);
        //    uint16_copy(&atsi[398 + w * 18],0x0000);  no-op
        //    uint16_copy(&atsi[400 + w * 18],0x0000);  no-op
            }
        }
        else
        {
            i = 0x180;
            for (int w = 0; w < 16; ++w)
            {
        //    uint16_copy(&atsi[i + w * 18],0x0000); no-op
              atsi[i + 2 + w * 18]  =  downmix_coeff(db[w].Lf_l);  // Left front speaker channel to left stereo speaker
              atsi[i + 3 + w * 18]  =  downmix_coeff(db[w].Lf_r);  // Left front speaker channel to right stereo speaker
              atsi[i + 4 + w * 18]  =  downmix_coeff(db[w].Rf_l);       // Right front speaker channel to left stereo speaker
              atsi[i + 5 + w * 18]  =  downmix_coeff(db[w].Rf_r);       // Right front speaker channel to right stereo speaker
              atsi[i + 6 + w * 18]  =  downmix_coeff(db[w].C_l);        // Center speaker channel to left stereo speaker
              atsi[i + 7 + w * 18]  =  downmix_coeff(db[w].C_r);        // Center speaker channel to right stereo speaker
              atsi[i + 8 + w * 18]  =  downmix_coeff(db[w].S_l);        // Left surround or Surround speaker channel to left stereo speaker
              atsi[i + 9 + w * 18]  =  downmix_coeff(db[w].S_r);       // Left surround or Surround speaker channel to left stereo speaker
              atsi[i + 10 + w * 18] =  downmix_coeff(db[w].Rs_l);      // Right surround to left stereo speaker
              atsi[i + 11 + w * 18] =  downmix_coeff(db[w].Rs_r);      // Right surround to right stereo speaker
              atsi[i + 12 + w * 18] =  downmix_coeff(db[w].LFE_l);     // Low-frequency effects (subwoofer) to left stereo speaker
              atsi[i + 13 + w * 18] =  downmix_coeff(db[w].LFE_r);     // Low-frequency effects (subwoofer) to right stereo speaker
        //    uint16_copy(&atsi[i + 14 + w * 18],0x0000);  no-op
        //    uint16_copy(&atsi[i + 15 + w * 18],0x0000);  no-op
            }
        }
    }

    /* SECTOR 2 */

    i = 0x800;
    uint16_copy(&atsi[i], numtitles);

    // [200806] The number numtitles must be equal to number of audio zone titles plus video zone titles linked to. Gapless tracks are packed in the same title.

    // Padding
    i += 8;

    for (j = 0; j < numtitles; ++j)
    {
        uint16_copy(&atsi[i], 0x8000 + (j + 1) * 0x100);
        ++i;
        uint16_t format_flag;
        if (files[j].type == AFMT_MLP)
        {
          format_flag = files[j].channels > 2 ? 0x0101 : 0x0001;
        }
        else
        {
          format_flag = 0x0100;
        }

        // MLP looks like 0x0101, when multichannel, 0x0001 when stereo or mono, PCM unfirmly 0x0100
        uint16_copy(&atsi[i], format_flag);

        // To be filled later - pointer to a following table.
        i += 7;
    }

    k = 0;
    int s = 0;
    
    for (j = 0; j < numtitles; ++j)
    {
        uint32_copy(&atsi[0x808 + 8 * j + 4], i - 0x800);

        uint16_copy(&atsi[i], 0x0000); // Unknown
        i += 2;
        
        atsi[i] = ntitletracks[j];
        ++i;
        
        atsi[i] = ntitletracks[j];
        ++i;
        
        title_length = 0;
        
        for (t = 0; t < ntitletracks[j]; ++t)
        {
            title_length += files[k + t].PTS_length;
        }
        
        uint32_copy(&atsi[i], title_length);
        i += 4;
        
        uint16_copy(&atsi[i], 0x0000);  // Unknown
        i += 2;
        
        uint16_copy(&atsi[i], 0x0010);                 // Pointer to PTS table
        i += 2;
        
        uint16_copy(&atsi[i], 16 + 20 * ntitletracks[j]);  // Pointer to sector table
        i+=2;
        
        //PATCH 09.09
        if (img->count || img->stillvob || img->active)
            uint16_copy(&atsi[i], 16 + 32 * ntitletracks[j]);                 // Pointer to a stillpic data table
        
        i+=2;

        /* Timestamp and sector records */

        for (t = 0; t < ntitletracks[j]; ++t)
        {
            // These seem to be pointers to a lookup table in the first sector of the ATSI

            x = get_afmt(&files[k], audioformats, &numafmts);
            x = (x * 8) << 8;
            
            if (t == 0)
            {
                x |= 0xc000;
            }
            
            // Downmix table index

            if (files[j].channels < 2 || files[k + t].downmix_table_rank == 0) //  0x10 for stereo or mono // or if no downmix table, so means: "no downmix"
            {
               x |= 0x0010;
            }
            else
            {
               x |= files[k + t].downmix_table_rank - 1; //0x0 to 0x0F, rank of downmix table (0-based)
            }

            uint16_copy(&atsi[i], x);
            i += 2;
            
            uint16_copy(&atsi[i], 0x0000);
            i += 2;
            
            atsi[i] = t + 1;
            ++i;
            
            atsi[i] = 0x00;
            ++i;
            
            uint32_copy(&atsi[i], files[k + t].first_PTS);  // Modify calc_PS_start !
            i += 4;
            
            uint32_copy(&atsi[i], files[k + t].PTS_length);

            i += 10;
        }
        
        /* Sector pointer records */
        
        for (t = 0; t < ntitletracks[j]; ++t)
        {
            atsi[i] = 0x01;
            i += 4;
            
            uint32_copy(&atsi[i], files[k + t].first_sector);
            i += 4;
            
            uint32_copy(&atsi[i], files[k + t].last_sector);
            i += 4;
        }
        
        if (img->count || img->stillvob || img->active)
        {

            /* Stills records */

            uint16_t r, u=0,  trackcount_save=trackcount;
            s+=(j)? ntitlepics[j-1]  : 0;
            
            if (ntitlepics[j]) 
                ++pictitlecount;
            
            if (globals.veryverbose) foutput(MSG_TAG "pictitlecount=%d for ntitlepics[%d]=%d\n", 
                                             pictitlecount,
                                             j,
                                             ntitlepics[j]);
             
            for (r = 0; r < ntitletracks[j]; ++r)
            {
                ++trackcount;
                
                if ((ntitlepics[j] == 0) && (img->npics[trackcount-1] == 0)) continue;  //  This might be taken off in some unclear cases.
        
                atsi[i++]=pictitlecount;  // title-with-pics rank (1-based)
                
                if ((img->options) && (img->options[s]) && (img->options[s]->manual))
                  atsi[i]=0x04;
                i++;
                uint16_copy(&atsi[i], 0x06*ntitletracks[j]); // track rank index
                i+=2;
                if (globals.veryverbose) foutput(MSG_TAG "ntitlepics[%d]=%d, ntitletracks[%d]=%d\n", j, ntitlepics[j], j, ntitletracks[j]);
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
                       foutput(INF "Skipping track with no pics, t=%d, trackcount=%d\n", t, trackcount);
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
                  else if ((img->options == NULL) || (img->options[s] == NULL) || (img->options[s]->manual == 0))
                     uint32_copy(&atsi[i], pictrackcount*90000); // defaulting to 1 second intervals
                  i+=4;
                  if ((img->options) && (img->options[s+u]))
                    atsi[i]=img->options[s+u]->starteffect;
                  i++;
                  if ((img->options) && (img->options[s+u]))
                    atsi[i]=img->options[s+u]->endeffect;
                  i++;
                  u++;
                  pictrackcount++;               }

            }
        }

        k+=ntitletracks[j];
    }

    // Pointer to following data

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

    uint32_copy(&atsi[12], files[ntracks - 1].last_sector + 2 * *atsi_sectors); // Pointer to last sector in ATS (i.e. numsectors-1)
    uint32_copy(&atsi[28],  *atsi_sectors - 1); // Last sector in ATSI
    uint32_copy(&atsi[196], *atsi_sectors);      // Start sector of ATST_AOBS

    int nb_atsi_files = 0;

    STRING_WRITE_CHAR_BUFSIZ(basename, "ATS_%02d_0.IFO",titleset+1)

    nb_atsi_files += create_file(audiotsdir, basename, atsi, 2048*(*atsi_sectors));

    STRING_WRITE_CHAR_BUFSIZ(basename, "ATS_%02d_0.BUP",titleset+1)

    nb_atsi_files += create_file(audiotsdir, basename, atsi, 2048*(*atsi_sectors));

    return(nb_atsi_files);
    #undef files
    #undef ntracks
    #undef img
    #undef db
}
