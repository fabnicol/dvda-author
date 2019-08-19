#if !HAVE_XML_H
#define HAVE_XML_H
int  generate_amgm_xml(uint8_t ngroups, uint8_t *ntracks, pic* img);
int  generate_spumux_xml(uint8_t ngroups, uint8_t *ntracks, uint8_t maxnumtracks, pic* img);
#endif
