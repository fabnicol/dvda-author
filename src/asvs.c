/*

AUDIO_SV.IFO

00000000   44 56 44 41 55 44 49 4F  41 53 56 53 00 Y0 00 12  DVDAUDIOASVS....
00000010   00 00 00 02 00 00 00 X0  53 00 00 00 00 00 00 00  ........S.......
00000020   00 10 80 80 00 10 80 80  00 10 80 80 00 10 80 80  ................
00000030   00 10 80 80 00 10 80 80  00 10 80 80 00 10 80 80  ................
00000040   00 10 80 80 00 10 80 80  00 10 80 80 00 10 80 80  ................
00000050   00 10 80 80 00 10 80 80  00 10 80 80 00 10 80 80  ................
00000060   01 00 00 01 00 00 00 00  00 00 00 00 00 00 00 00  ................


* fixed for whatever track in group 1 (just one pic),
with X0 size of bmp pic in sectors -1.
* Y0 = number of titles in disc (2 bytes)
* X0 = total size of pics (bmp) in sectors -1, perhaps 1 to three bytes before.
* Table starting 0x60, for each group and title
Offset     bytes     value
00         1         number of tracks in title (1-based)
02         2         track rank (1-based) at start of title. When several tracks in title, track rank for next title
                     is current rank + number of tracks in current title
06         2         sector start in VOB: 0, sizeof(pic1VOB), +=sizeof(pic2VOB), ..., +=sizeof(pic (n-1)VOB)
with current title starting at pic n


* apparently no group indexing


*/
#include "commonvars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#ifndef __WIN32__
#include <unistd.h>
#endif
#include "structures.h"
#include "c_utils.h"
#include "winport.h"
#include "auxiliary.h"
#include "asvs.h"


extern globalData globals;
int create_asvs(char* audiotsdir,int naudio_groups, uint8_t *numtitles, uint16_t ** ntitlepics,  uint8_t sectors_asvs, pic* img)
{

    uint8_t titleset=0, title=0;
    uint8_t asvs[sectors_asvs*2048];
    memset(asvs,0,sectors_asvs*2048);

    uint16_t pict=0, npics=0, totnumtitles=0, k, j, t;
    errno=0;
    uint32_t totpicsectors=0;

    memcpy(asvs,"DVDAUDIOASVS",12);

    uint16_copy(&asvs[0xE], 0x0012);  // DVD Spec
    asvs[0x13]=2; // unknown
    asvs[0x18]=0x53; // unknown
    asvs[0x19]=0x1; // activates buttons // numbre of menus ?

    uint32_copy(&asvs[0x20], (uint32_t) strtoul(img->activetextcolor_palette,NULL,16));   // This palette is taken as is from a commercial DVD: unselected text (display)
    //uint32_copy(&asvs[0x24], (uint32_t) 0x80E6807F);
    uint32_copy(&asvs[0x28], (uint32_t) strtoul(img->activebgcolor_palette,NULL,16)); // album, group headers and highlighted text
    uint32_copy(&asvs[0x2C], (uint32_t) strtoul(img->activehighlightcolor_palette,NULL,16)); //highlight motif
    uint32_copy(&asvs[0x30], (uint32_t) strtoul(img->activeselectfgcolor_palette,NULL,16)); // select action text only
   /*
    uint32_copy(&asvs[0x34], 0x007C6355);
    uint32_copy(&asvs[0x38], 0x006ADDCA);
    uint32_copy(&asvs[0x3C], 0x00AA10A6);
    uint32_copy(&asvs[0x40], 0x00296EF0);
    uint32_copy(&asvs[0x44], 0x002E9E9C);
    uint32_copy(&asvs[0x48], 0x0050D58E);
    uint32_copy(&asvs[0x4C], 0x00EB8080);
    uint32_copy(&asvs[0x50], 0x00AA8080);
    uint32_copy(&asvs[0x54], 0x007E8080);
    uint32_copy(&asvs[0x58], 0x00538080);
    uint32_copy(&asvs[0x5C], 0x00108080);  possible palettes */
    k=0x60;
    t=0x378;

int loop=0;

    while  ((titleset < naudio_groups) && (title < numtitles[titleset]))
    {


        npics=ntitlepics[titleset][title];
        if (npics)
        {
		asvs[k]=npics;
		k+=2;
		uint16_copy(&asvs[k], pict+1);   // 1-based
		pict+=npics;
		k+=2;
		uint32_copy(&asvs[k], totpicsectors);
		for (j=0; j < npics; j++)
		{
		    if (j) uint16_copy(&asvs[t], totpicsectors);
		    t+=2;
		    totpicsectors+=img->stillpicvobsize[j];     //pict [] is 0-based: pict[0] for first track
		    if (totpicsectors > 1024) foutput(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Exceeding stillpic buffer limit (2 MB) at pict #%d.\n", j);
		}
		k+=4;
		img->stillpicvobsize+=npics;
		totnumtitles++;
        }

        title++;

        if (title == numtitles[titleset])
        {
            titleset++;
            title=0;
        }
        loop++;

    }

    uint16_copy(&asvs[0xC], totnumtitles);
    uint32_copy(&asvs[0x14], /* size of VOB associated with : change TODO */ totpicsectors-1);

    create_file(audiotsdir, "AUDIO_SV.IFO", asvs, sectors_asvs*2048);
    create_file(audiotsdir, "AUDIO_SV.BUP", asvs, sectors_asvs*2048);
    fflush(NULL);
    return errno;
}
