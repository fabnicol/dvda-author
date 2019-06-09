#ifndef HAVE_MENU_H
#define HAVE_MENU_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "structures.h"

#if !HAVE_core_BUILD

int  generate_background_mpg(pic* img);
int prepare_overlay_img(char* text, int8_t group, pic *img, char* command, char* command2, int menu, char* albumcolor);
int  launch_spumux(pic* img);
int  launch_dvdauthor();
int mogrify_img(char* text, int8_t group, int8_t track, pic *img, uint8_t maxnumtracks, char* command, char* command2,  int8_t offset, char* textcolor);
int  generate_menu_pics(pic* img, uint8_t ngroups, uint8_t *numtracks, uint8_t maxntracks);
int create_stillpic_directory(char* string, int32_t count);
void create_activemenu(pic* img);
int create_mpg(pic* img, uint16_t rank, char* mp2track, char* tempfile);
uint16_t x(uint8_t group, uint8_t ngroups);
uint16_t y(uint8_t track, uint8_t maxnumtracks);
void initialize_binary_paths(char level);
void menu_characteristics_coherence_test(pic* img, uint8_t ngroups);
void compute_pointsize(pic* img, uint16_t maxtracklength, uint8_t maxnumtracks);
#endif
#endif // HAVE_MENU_C
