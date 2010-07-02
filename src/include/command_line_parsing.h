#ifndef COMMAND_LINE_PARSING_H
#define COMMAND_LINE_PARSING_H

#include "structures.h"
#include "commonvars.h"
/* dealing with the unfortunate bug of SoX versions < 14.3
  SoX redefines getopt_log in its source code and this causes runtime stack overflows owing to dvda-author code calling the SoX intervening function instead of the right one
  bug corrected by SoX developers as of end of May 2009 by replacing getopt_long with lsx_getopt_long
  does not arise if in-tree code is built however.
  libibertyc will be compiled with a name replacement for getopt_long (getopt_long_surrogate)  */
#ifdef HAVE_SOX_BUG
#ifdef getopt_long
#undef getopt_long
#define getopt_long getopt_long_surrogate
#endif
#endif

command_t *command_line_parsing(int , char* const argv[],  command_t *command);

#ifndef WITHOUT_FIXWAV
#include "fixwav_manager.h"
void fixwav_parsing(char *ssopt);
#endif


void ats2wav_parsing(const char * arg, extractlist* extract);
void extract_list_parsing(const char *arg, extractlist* extract);
void still_options_parsing(char *ssopt, pic* img);
#endif
