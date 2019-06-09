#ifndef FIXWAV_AUXILIARY_INCLUDED
#define FIXWAV_AUXILIARY_INCLUDED




#include "fixwav_manager.h"


void initialise_globals_fixwav(bool silence, bool logfile, FILE* journal);
bool isok();
void get_input( char* buf );




#endif // FIXWAV_AUXILIARY_INCLUDED
