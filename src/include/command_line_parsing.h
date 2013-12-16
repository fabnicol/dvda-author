#ifndef COMMAND_LINE_PARSING_H
#define COMMAND_LINE_PARSING_H

#include "structures.h"
#include "commonvars.h"
/* dealing with the unfortunate bug of SoX versions < 14.3
  SoX redefines getopt_log in its source code and this causes runtime stack overflows owing to dvda-author code calling the SoX intervening function instead of the right one
  bug corrected by SoX developers as of end of May 2009 by replacing getopt_long with lsx_getopt_long
  does not arise if in-tree code is built however.
  libibertyc will be compiled with a name replacement for getopt_long (getopt_long_surrogate)  */


#define ALLOWED_OPTIONS_PRINT  "0123456789aA:b:B:c:dD:e:E:f::F::g:G:hH:i:I::j:J:k:K:l:L:m::M:nN:o:O:p:P::qQ:r::R:s:S::tT:u:U::vV:w:Wx:X:y:Y:z:Z:"

// Note on options
    // Allowing for 30 non-print characters in command_line_parsing.c 
    // for short options only
    // Note that the :/:: diacritics are only needed for short options and that long options argument status 
    // is defined in struct longopts in command_line_parsing.c, so
    // this trick is OK if only long options are used for non-print short options.
    
command_t *command_line_parsing(int , char* const argv[],  command_t *command);
void process_dvd_video_zone(command_t* command);
#include "fixwav_manager.h"
void fixwav_parsing(char *ssopt);

void ats2wav_parsing(const char * arg, extractlist* extract, char* player);
void extract_list_parsing(const char *arg, extractlist* extract);
void still_options_parsing(char *ssopt, pic* img);
//void parse_double_entry_command_line(char*** DOUBLE_ARRAY, uint8_t* COUNTER_ARRAY, uint8_t* TOTAL, short audit_flag);
#endif
