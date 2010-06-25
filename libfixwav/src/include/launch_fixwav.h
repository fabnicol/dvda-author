#ifndef LAUNCH_FIXWAV_H_INCLUDED
#define LAUNCH_FIXWAV_H_INCLUDED
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

void create_output_filename(char* filename, char** buffer);



// Undefining VERSION config.h if lying around and working with IDE
#if defined VERSION && defined __CB__
#undef VERSION
#endif
#ifndef VERSION
#define VERSION  "0.2"
#endif

#define FIXWAV_HEADER    printf("\n\n%s%s%s\n\n", "__________________________  FIXWAV ",VERSION," __________________________");

#endif // LAUNCH_FIXWAV_H_INCLUDED
