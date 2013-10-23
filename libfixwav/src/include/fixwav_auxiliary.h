#ifndef FIXWAV_AUXILIARY_INCLUDED
#define FIXWAV_AUXILIARY_INCLUDED




#include "fixwav_manager.h"


void initialise_globals_fixwav(_Bool silence, _Bool logfile, FILE* journal);
_Bool isok();
void get_input( char* buf );




#endif // FIXWAV_AUXILIARY_INCLUDED
