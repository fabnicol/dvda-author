/* Simple example of using SoX libraries
 *
 * Copyright (c) 2007-8 robs@users.sourceforge.net
 * Modified bay Fabrice Nicol, Copyright 20008 <fabrnicol@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef WITHOUT_sox

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#ifdef NDEBUG /* N.B. assert used with active statements so enable always */
#undef NDEBUG
#endif
#include "c_utils.h"
#include "structures.h"
#include "libsoxconvert.h"
/*
 * Reads input file, applies vol & flanger effects, stores in output file.
 * E.g. example1 monkey.au monkey.aiff
 */

/* NOTE: SoX API noticeably changed as of v. 14.3.0 */




#if ((SOX_LIB_VERSION_CODE >>8 & 0xff) < 3)
#error ---------------------------------------------------------------------------------
#error "ERR "Build: SoX lib is too old --  version must be higher or equal to 14.3.0.
#error       Either reinstall new lib or build from source code with --enable-sox-patch.
#error ---------------------------------------------------------------------------------
#endif

// arcane issue with assert() that justifies this workaround
#define check(X) do { if ((X)==0) { foutput("%s%d\n", ERR "SoX runtime failure, stage ", stage); return -1;} } while(0);

int soxconvert(char * input, char* output, globalData* globals)
{
  int stage = 0;

  static sox_format_t * in, * out; /* input and output files */
  sox_effects_chain_t * chain;
  sox_effect_t * e;
  char * args[10];


   foutput("%s\n", INF "Converting file");

  /* All libSoX applications must start by initialising the SoX library */
  sox_format_init(); // this may fail without much ado

  /* Open the input file (with default parameters) */
  check(in = sox_open_read(input, NULL, NULL, NULL)); stage++;

  /* Open the output file; we must specify the output signal characteristics.
   * Since we are using only simple effects, they are the same as the input
   * file characteristics */
  check(out = sox_open_write(output, &in->signal, NULL, NULL, NULL, NULL)); stage++;

  /* Create an effects chain; some effects need to know about the input
   * or output file encoding so we provide that information here */



  chain = sox_create_effects_chain(&in->encoding, &out->encoding);

  /* The first effect in the effect chain must be something that can source
   * samples; in this case, we use the built-in handler that inputs
   * data from an audio file */
  e = sox_create_effect(sox_find_effect("input"));
  args[0] = (char *)in;
  check(sox_effect_options(e, 1, args) == SOX_SUCCESS); stage++;
  /* This becomes the first `effect' in the chain */
  check(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS); stage++;

  /* The last effect in the effect chain must be something that only consumes
   * samples; in this case, we use the built-in handler that outputs
   * data to an audio file */

  e = sox_create_effect(sox_find_effect("output"));
  args[0] = (char *)out;
  check(sox_effect_options(e, 1, args) == SOX_SUCCESS); stage++;
  check(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS); stage++;

  /* Flow samples through the effects processing chain until EOF is reached */
  sox_flow_effects(chain, NULL, NULL);

  foutput("%s\n", INF "Exiting SoX...");
  /* All done; tidy up: */
  sox_delete_effects(chain);
  sox_delete_effects_chain(chain);

  sox_close(out);
  sox_close(in);
  sox_format_quit();

  return 0;
}


int resample(char* infile, char* outfile, unsigned int channels, unsigned int bitrate, unsigned int samplerate)
{
    static sox_format_t * in, * out; /* input and output files */
      sox_effects_chain_t * chain;
      sox_effect_t * e;
      char * args[10];
      sox_signalinfo_t interm_signal; /* @ intermediate points in the chain. */
      sox_encodinginfo_t out_encoding = {
        SOX_ENCODING_SIGN2,
        bitrate,
        0,
        sox_option_default,
        sox_option_default,
        sox_option_default,
        sox_false
      };
      sox_signalinfo_t out_signal = {
        samplerate,
        channels,
        bitrate,
        0,
        NULL
      };

      assert(sox_init() == SOX_SUCCESS);
      assert(in = sox_open_read(infile, NULL, NULL, NULL));
      assert(out = sox_open_write(outfile, &out_signal, &out_encoding, NULL, NULL, NULL));

      chain = sox_create_effects_chain(&in->encoding, &out->encoding);

      interm_signal = in->signal; /* NB: deep copy */

      e = sox_create_effect(sox_find_effect("input"));
      args[0] = (char *)in, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
      assert(sox_add_effect(chain, e, &interm_signal, &in->signal) == SOX_SUCCESS);
      free(e);

      if (in->signal.rate != out->signal.rate) {
        e = sox_create_effect(sox_find_effect("rate"));
        assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
        assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
        free(e);
      }

      if (in->signal.channels != out->signal.channels) {
        e = sox_create_effect(sox_find_effect("channels"));
        assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
        assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
        free(e);
      }

      e = sox_create_effect(sox_find_effect("output"));
      args[0] = (char *)out, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
      assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
      free(e);

      sox_flow_effects(chain, NULL, NULL);

      sox_delete_effects_chain(chain);
      sox_close(out);
      sox_close(in);
      sox_quit();

      return 0;
}

#endif
