#ifndef LAUNCH_FIXWAV_H_INCLUDED
#define LAUNCH_FIXWAV_H_INCLUDED
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "c_utils.h"

void create_output_filename(char* filename, char** buffer);



#define FIXWAV_HEADER    foutput("\n\n%s%s%s\n\n", "__________________________  FIXWAV ",VERSION," __________________________");

#endif // LAUNCH_FIXWAV_H_INCLUDED
