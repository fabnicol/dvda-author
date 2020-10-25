#if !HAVE_XML_H
#define HAVE_XML_H
#include "c_utils.h"
int  generate_amgm_xml(uint8_t ngroups, uint8_t *ntracks, pic* img, globalData*);
int  generate_spumux_xml(command_t* command, uint8_t ngroups, uint8_t *ntracks, pic* img, globalData* );
#endif
