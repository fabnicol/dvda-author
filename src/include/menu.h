#ifndef HAVE_MENU_C
#define HAVE_MENU_C




int  generate_background_mpg(pic* img,  uint8_t ngroups, uint8_t* ntracks);
int  launch_spumux(pic* img);
int  launch_dvdauthor();
int mogrify_img(char* text, int8_t group, int8_t track, pic *img, uint8_t maxnumtracks, char* command, char* command2,  int8_t offset, char* textcolor);
int  generate_menu_pics(pic* img, uint8_t ngroups, uint8_t *numtracks, uint8_t maxntracks);
int create_stillpic_directory(char* string, uint32_t count);
void create_activemenu(pic* img,uint16_t totntracks);
uint16_t x(uint8_t group, uint8_t ngroups);
uint16_t y(uint8_t track, uint8_t maxnumtracks);
#endif // HAVE_MENU_C
