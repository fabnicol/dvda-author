/* ==========================================================================
*
*   checkParameters.c
*   user_control function was originally designed by Pigiron, 2007.
*   auto_control and regular_test functions: Copyright Fabrice Nicol, 2008.
*
*   Description: processes core audio parameters in two alternative modes.
*        uses simple heuristics to automate header patching, or enters
*        user's core audio parameters (bit rate, sample rate, channels)
* ========================================================================== */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include    <stdio.h>
#include    <stdlib.h>
#include    <locale.h>
#include    <stdint.h>
#include    <inttypes.h>
#include    <math.h>
#include    "fixwav_auxiliary.h"
#include    "checkParameters.h"
#include    "fixwav.h"
#include    "fixwav_manager.h"
#include    "c_utils.h"
#include    "structures.h"
#include "libiberty.h"


extern globalData globals;

int user_control(WaveData *info, WaveHeader *header)
{

  char buf[FIXBUF_LEN];
  int repair=GOOD_HEADER;
  unsigned int bps;
  bool ok=false;

  /* The Subchunk1 Number of Channels */

  if (info->interactive)
    {
      if (!info->prepend)
      {  // useless if prepending, as 'header' info is always wrong
          if (globals.debugging) foutput( "\n%s\n", "[INT]  Is the file recorded in " );
          switch ( header->channels )
            {
            case 1:
              if (globals.debugging) foutput( "%s", "Mono? [y/n] " );
              break;
            case 2:
              if (globals.debugging) foutput( "%s", "Stereo?  [y/n] " );
              break;

            default:
              if (globals.debugging) foutput( "%d channels?  [y/n] ", header->channels );

            }
            ok=isok();
      }

      if (!ok)
        {
          if (globals.debugging) foutput( "%s", "[INT]  Enter number of channels... 1=Mono, 2=Stereo, etc: " );
          fflush(stdout);
          get_input(buf);
          header->channels = (uint16_t) atoi(buf);
          repair = BAD_HEADER;
        }

      /* The Sample Rate is the number of samples per second */
      if (!info->prepend)
      {
          if (globals.debugging) foutput( "[INT]  Is the number of samples per second = %"PRIu32"?  [y/n] ", header->dwSamplesPerSec
 );
          ok=isok();
      }

      if (!ok)
        {
          if (globals.debugging) foutput( "%s", "[INT]  Enter number of samples per second in kHz (e.g. 44.1) : " );
          fflush(stdout);
          get_input(buf);
          header->dwSamplesPerSec
 = (uint32_t) floor(1000*atof(buf));
          repair = BAD_HEADER;
        }

      /* The number of bits per sample */
      if (!info->prepend)
      {
          if (globals.debugging) foutput( "[INT]  Is the number of bits per sample = %d?  [y/n] ", header->wBitsPerSample );
          ok=isok();
      }

      if (!ok)
        {
          if (globals.debugging) foutput( "%s", "[INT]  Enter number of bits per sample:  " );
          fflush(stdout);
          get_input(buf);
          header->wBitsPerSample = (uint16_t) atoi(buf);
          repair = BAD_HEADER;
        }

    }

  /* The bytes per second = SampleRate * NumChannels * BitsPerSample/8 */
  bps = header->dwSamplesPerSec
 * header->channels * (header->wBitsPerSample / 8);

  // forcing interactive mode if null audio
  if (bps == 0)
    {
      info->interactive=TRUE;
      return (info->repair=user_control(info, header));
    }

  if ( header->nAvgBytesPerSec == bps )
    {
      // Patch again version 0.1.1: -Saple Rate ...offset 24  + Bytes per second ...offset 28
      if (globals.debugging) foutput("%s\n",  MSG_TAG "Found correct Subchunk1 Bytes per Second at offset 28" );
    }
  else
    {
      if (!info->prepend) if (globals.debugging) foutput("%s\n",  MSG_TAG "Subchunk1 Bytes per Second at offset 28 is incorrect\n"INF "... repairing" );
      header->nAvgBytesPerSec = bps;
      repair = BAD_HEADER;
    }

  /* The number of bytes per sample = NumChannels * BitsPerSample/8 */
  if ( header->nBlockAlign == header->channels * (header->wBitsPerSample / 8) )
    {
      if (globals.debugging) foutput("%s\n",  MSG_TAG "Found correct Subchunk1 Bytes Per Sample at offset 32" );
    }
  else
    {
      if (!info->prepend) if (globals.debugging) foutput("%s\n",  MSG_TAG "Subchunk1 Bytes Per Sample at offset 32 is incorrect\n"INF "... repairing" );
      header->nBlockAlign = header->channels * (header->wBitsPerSample / 8);
      repair = BAD_HEADER;
    }

  return (info->repair=repair);
}

/* Aug. 2016 patch.
 *
 * So far fixwav was not concerned with WAV_FORMAT_EXTENSIBLE.
 * Correct implementation of multichannel requires implementing this however.
 * MS specifications for WAV state that WAV_FORMAT_EXTENSIBLE should be used
 * whenever one of the following cases is met:
 * - PCM data has more than 16 bits/sample
 * - the number of channel is more than 2
 * - the mappings from channels to speakers is specified. */


int auto_control(WaveData *info, WaveHeader *header)
{
  /* This implementation of the algoritm restricts it to the 44.1 kHz and 48 kHz families although this is not
  out of logical necessity */

  int regular[6]={0};

  /* initializing */

  if (globals.debugging) foutput("%s\n", INF "Checking header -- automatic mode...");

  /* The following function may deduce the number of bytes per second and of bytes per sample
   * from other constants considered as given and thereby recover partially mangled headers
   * Mathematical conditions apply, see comments at EOF : it does not work for 3/6-channel audio
   * or 20-bit channel samples. */

  regular_test(header, regular);

  bool regular_wBitsPerSample  = regular[1];
  bool regular_dwSamplesPerSec
  = regular[2];
  bool regular_nBlockAlign = regular[3];
  bool regular_nAvgBytesPerSec = regular[4];
  bool regular_channels   = regular[5];

  /* Checking whether there is anything to be done at all */

  if (header->nAvgBytesPerSec == (header->dwSamplesPerSec * header->wBitsPerSample * header->channels) / 8
      && header->nBlockAlign == (header->channels * header->wBitsPerSample) / 8
      && (regular[0] == 5 || header->channels % 3 == 0 || header->wBitsPerSample  == 20)
     )
    {
      if (globals.debugging) foutput("%s\n", MSG_TAG "Core parameters need not be repaired");
      return(info->repair = GOOD_HEADER);
    }

  /* Always repairing from now on except when bailing out */

  info->repair=BAD_HEADER;

  /* Set of assumptions (R) + (3), see comment below */
  /* Uniqueness of solution requires (R) */

  if ((regular[0] < 3 && header->channels % 3 != 0 && header->wBitsPerSample != 20)
          || header->channels % 3 == 0 || header->wBitsPerSample == 20)
  {
    goto bailing_out;
  }

  if (regular_channels && header->channels % 3 != 0 && header->wBitsPerSample != 20)
    {
      /* channel number considered a parameter, variables between curly brackets */

      // {N, S} case

      if (regular_wBitsPerSample && regular_dwSamplesPerSec
)
        {
          header->nAvgBytesPerSec = (header->dwSamplesPerSec
 * header->wBitsPerSample * header->channels)/8;
          header->nBlockAlign = (header->channels * header->wBitsPerSample)/8;
          /* Now double-checking */
          regular_test(header, regular);
          if (regular[0] == 5)  return (info->repair);
        }

      // {N, B}
      if (regular_nBlockAlign && regular_dwSamplesPerSec
)
        {
          header->wBitsPerSample  = header->channels ? (header->nBlockAlign * 8) / header->channels: 0;
          header->nAvgBytesPerSec = (header->dwSamplesPerSec
 * header->wBitsPerSample * header->channels)/8;
          /* Now double-checking */
          regular_test(header, regular);
          if (regular[0] == 5)  return (info->repair);
        }

      // {S, F}

      if (regular_nAvgBytesPerSec && regular_wBitsPerSample )
        {
          header->nBlockAlign  = (header->wBitsPerSample * header->channels)/8;
          header->dwSamplesPerSec = (header->wBitsPerSample && header->channels)? (header->nAvgBytesPerSec * 8)/(header->wBitsPerSample * header->channels) : 0 ;
          regular_test(header, regular);
          if (regular[0] == 5) return (info->repair);
        }

      // {S, B}
      if ((regular_nAvgBytesPerSec) && (regular_dwSamplesPerSec
 ))
        {
          header->nBlockAlign  = (header->wBitsPerSample * header->channels)/8;
          header->wBitsPerSample   =  ( header->channels && header->dwSamplesPerSec)? (8 * header->nAvgBytesPerSec)/( header->channels * header->dwSamplesPerSec) : 0 ;
          regular_test(header, regular);
          if (regular[0] == 5) return (info->repair);
        }

      // {F, B}
      if ((regular_nAvgBytesPerSec) && (regular_nBlockAlign ))
        {
          header->wBitsPerSample   = (header->nBlockAlign * 8 )/ header->channels;
          header->dwSamplesPerSec  = (header->wBitsPerSample && header->channels)? (header->nAvgBytesPerSec * 8)/(header->wBitsPerSample * header->channels) : 0;
          regular_test(header, regular);
          if (regular[0] == 5) return (info->repair);
        }

    }
  /* Now consider cases in which number of channels is corrupt */

// {N,C}

  if (regular_nBlockAlign && regular_wBitsPerSample && regular_dwSamplesPerSec)
    {
      header->channels   = (header->nBlockAlign * 8) / header->wBitsPerSample;
      header->nAvgBytesPerSec = (header->dwSamplesPerSec * header->wBitsPerSample * header->channels)/8;
      regular_test(header, regular);
      if (regular[0] == 5)  return (info->repair);
    }

// {S, C}

  if (regular_nAvgBytesPerSec && regular_wBitsPerSample && regular_dwSamplesPerSec)
    {
      header->nBlockAlign = header->dwSamplesPerSec? header->nAvgBytesPerSec/header->dwSamplesPerSec
 : 0;
      header->channels   = (header->nBlockAlign * 8 )/ header->wBitsPerSample;
      regular_test(header, regular);
      if (regular[0] == 5)  return (info->repair);
    }

  /* Special non-linear (hyperbolic) cases: XY= constant, yet a single solution under the set of assumtions */

// {F, C}

  if (regular_nAvgBytesPerSec && regular_wBitsPerSample && regular_nBlockAlign)
    {
      header->dwSamplesPerSec = header->nBlockAlign? header->nAvgBytesPerSec / header->nBlockAlign : 0;
      header->channels  = (header->nBlockAlign * 8) / header->wBitsPerSample;

      regular_test(header, regular);
      if (regular[0] == 5)  return (info->repair);
    }

// {C, B}
// The theorem below proves unicity of the {C, B} solution: it suffices to loop on C and break once found one.
  if (regular_nAvgBytesPerSec && regular_nBlockAlign && regular_dwSamplesPerSec)
    {
      // Satisfying constaint on constants ?
      if (header->nAvgBytesPerSec != header->dwSamplesPerSec * header->nBlockAlign) goto bailing_out;

      for (header->channels = 1; header->channels < 6 ; header->channels++)
        {
          if (header->channels == 3) continue;
          header->wBitsPerSample   = (header->channels)? (header->nBlockAlign*8) / header->channels:0;
          regular_test(header, regular);
          if (regular[0] == 5) return (info->repair);
        }
    }

  /* Now we are left with the unfortunate {N, F} case...or non-regular solutions: bailing out */

bailing_out:

  if (header->wBitsPerSample == 20 || header->channels % 3 == 0)
  {
      if (globals.debugging) foutput("\n%s\n", WAR "Special automatic header recovery does not apply to 3/6-channel or 20-bit-audio.");
  }
  if (globals.debugging) foutput("\n%s\n", WAR "Sorry, automatic mode cannot be used:\n       not enough information left in header.");


  return (info->repair);

}

void regular_test(WaveHeader *head, int* regular)
{
  int i, j, k, l;

  if (head == NULL) if (globals.debugging) foutput("%s\n", ERR "NULL wave header !");

  bool regular_channels  = (head->channels >= 1) && (head->channels < 6);
  bool regular_wBitsPerSample = (head->wBitsPerSample == 16) + (head->wBitsPerSample == 24);
  bool regular_dwSamplesPerSec;

  if (head->dwSamplesPerSec)
    regular_dwSamplesPerSec = (head->dwSamplesPerSec % 44100 == 0) + (head->dwSamplesPerSec % 48000 == 0);
  else
    regular_dwSamplesPerSec = 0;

  /* bit rates other than 16, 24 and 3 channels are not considered */

  bool regular_nBlockAlign=(head->nBlockAlign == 2*16/8)+(head->nBlockAlign == 2*24/8)
                           +(head->nBlockAlign == 3*16/8)+(head->nBlockAlign == 3*24/8)
                           +(head->nBlockAlign == 4*16/8)+(head->nBlockAlign == 4*24/8)
                           +(head->nBlockAlign == 5*16/8)+(head->nBlockAlign == 5*24/8)
                           +(head->nBlockAlign == 16/8)+(head->nBlockAlign == 24/8);

  bool regular_nAvgBytesPerSec=0;


  for (i=1; i < 6; i++)
    for (j=0; j < 3; j++)
      for (k=0; k < 3; k++)
        for (l=16; l < 32; l+=8)
          {
            if ( (j+k) && (j*k == 0) )
              if ( head->nAvgBytesPerSec == (uint32_t) (i* ((j* 44100) + (k* 48000)) * l /8 ))
                {
                  regular_nAvgBytesPerSec=1;
                  break;
                }
          }


  regular[0]=regular_wBitsPerSample + regular_dwSamplesPerSec + regular_nBlockAlign + regular_nAvgBytesPerSec + regular_channels;
  regular[1]=regular_wBitsPerSample;
  regular[2]=regular_dwSamplesPerSec ;
  regular[3]=regular_nBlockAlign;
  regular[4]=regular_nAvgBytesPerSec;
  regular[5]=regular_channels;

  return;

}


/*************************************************************************************************
*	About automatic mode
*	--------------------
*

The automatic mode algorithm
----------------------------

    A set of well-formed audio characteristics (R) is first defined (implementations
    may be restrictive for practical purposes as above), variables in (R) will
    henceforth be called regular variables.

    The algorithm is based on the two equations on regular variables,

            (1) N - F C B/8 = 0
            (2) S -   C B/8   = 0

            where N is the number of bytes per second
                  S    the number of bytes per sample (all channels)
                  C    the number of channels
                  B    the number of bits per sample channel
                  F    sampling frequency in Hz

    Assumptions on header state are:

   (3) three out of the five above variables are assumed to be correct, and considered as parameters.


Mathematical discussion
-----------------------

    Let D={N,S, C, B, F}. The above system of equations (1) and (2) form a linear system with two
    unknown variables if the pair of variables is among this list:
    {N,S}, {N,C}, {N, B}, {S, F}, {S, C}, {S, B}, as the determinant is not null.
    In these cases, there is a single solution to the linear system.

    However, the determinant is either null, or the system is not linear, for the following pairs of
    unknown variables:
    {N, F}, {F, C}, {F, B}, {C, B}, out of the 10 possible pairs.
    In these cases yet, S is always known and, following (3), considered as a parameter.
    As there must be a solution, the problem thus boils down to proving unicity under the set of assumptions.
    From (2) it can be shown that, for a pair of solutions {(N, S, C, B, F), (N', S, C', B', F')}:

            (4) B/B' = k, where k = C'/C

    The {F, C} and {F, B} cases are straightforward and the solution is unique. However for {C, B} the set
    of three constants {F, N, S} is linked by the equation F = N S, hence (2) is hyperbolic.
    For this case we now add the following assumptions on variables, which define a stricter set (R'):
    (R)	- number of channels is strictly positive and not a multiple of 3,
        - bit rate is either 16, 20 or 24 and if 20, channels are different from 4 or 5.

    Variables satisfying (R') in this case will be called regular variables equally.

    Now,   for B, B' in {16, 20, 24}, and C, C' and C' > C, chosen without loss of generality, in {1, 2, 4, 5},
    B/B' > 1 hence {B', B} = {16, 20} or {16, 24} or {20, 24}, entailing C'/C = 3/2 or 5/4 or 6/5
    with C in {1, 2, 4, 5} anc C' in {2, 4, 5}, C' > C.

    This set of constraints cannot be satisfied under (R). Ab absurdo, it ensues that C' = C, then B' = B out of (4).

    Out of the five cases at hand, N is a known correct parameter except in the four cases, hence N' = N,
    whence F' = F out of (1).

    There remains the {N, F} case, which should be
    very rare, and added to the set of header assumptions as below:

            (3') (header assumptions, revised): Three parameters are known to be correct, other than {S, C, B}.

    In the {S, C, B} case, the algorithm will bail out.

Algorithm
---------

    The algorithm first tests whether all five C variables read from the file header are within the bounds
    of (R), the set of regular values for this mode. If there are fewer than two such variables out of five,
    fixwav reverts to manual mode.
    Then setting the channel number, two regular variables are selected other than {S, B}.
    If this is not possible,  fixwav bails out.
    The other two variables are calculated out of (1) and (2), then tested to be within the bounds of (R).
    Should the test fail, fixwav looks for other possible combinations of known parameters.
    The above theorem ensures that there is just one solution: the first regular values are the only ones
    under the set of assumptions.
    In the {C, B} case, the linear constraint on constants is checked and the stricter conditions (R') are
    enforced, bailing out if they are not satisfied. Then the one remaining equation is
    solved by looping on the number of channels C: the above theorem ensures that the first regular pair
    is the only one solution.
    When all options have failed, fixwav bails out to manual mode, otherwise it returns BAD_header->
    info values are modified as global variables.

Important note
--------------

    1. The algorithm assumes that if the constants are regular, then they are correct values.
    Should this assumption be erroneous, wrong corrections can be made that satisfy all mathematical constraints.
    User checking is therefore advised when option -a is used (please refrain from using silent mode -q
    in conjunction with -a).
    Example of "wrong" correction: C = 1, S = 3, B = 24, F = 96 kHz, instead of C = 2, S = 6, B = 24, F = 48 kHz.

    2. The above theorem holds in the 20-bit case except for one minor subcase (C = 4 or C = 5). The implementation
    ignores 20-bit configurations for simplicity.-

    <added by Fabrice Nicol,  May 2008, corrected Aug.2016 >




*
******************************************************************************************************************/











































































































































