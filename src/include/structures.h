#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED


#include "stream_decoder.h"
#include "inttypes.h"



typedef struct
{
    uint8_t  nextractgroup[1];
    uint8_t  extracttitleset[9];
    uint8_t  extracttrackintitleset[9][99];
} extractlist;


typedef struct
{
    uint8_t bitspersample;
    uint8_t channels;
    uint8_t cga;
    uint32_t samplerate;
} audioformat_t;


typedef struct
{
    FILE* fp;
    FLAC__StreamDecoder* flac;
    // Used for FLAC decoding:
    uint8_t buf[1024*256];
    uint32_t n;
    uint32_t eos;
    uint32_t bytesread;
} audio_input_t;

typedef struct
{
    uint8_t header_size;
    uint8_t type;
    uint8_t bitspersample;
    uint8_t channels;
    // L&T Fedkamp addition
    uint8_t joingap;
    uint8_t single_track;
    uint8_t contin;
    uint8_t contin_track;
    uint8_t cga;
    uint8_t join_flag;
    //uint8_t byteorder_testmode;
    uint8_t newtitle;
    uint8_t headerlength;
    uint8_t padd;
    // L&T Feldkamp addition (multichannel)
    uint8_t offset;
    uint8_t rmdr;
    // L&T Feldkamp addition (multichannel)
    uint16_t sampleunitsize;
    uint32_t samplerate;
    uint32_t first_sector;
    uint32_t last_sector;
    // L&T Feldkamp addition (multichannel)
    uint32_t lpcm_payload;
    uint32_t firstpackdecrement;
    uint32_t SCRquantity;
    uint32_t firstpack_audiopesheaderquantity;
    uint32_t midpack_audiopesheaderquantity;
    uint32_t firstpack_lpcm_headerquantity;
    uint32_t midpack_lpcm_headerquantity;
    uint32_t firstpack_pes_padding;
    uint32_t midpack_pes_padding;
    // L&T Feldkamp addition (multichannel)
    uint64_t numsamples;
    uint64_t numbytes; // theoretical file size
    uint64_t file_size; // file size on disc
    uint64_t bytesperframe;
    uint64_t bytespersecond;
    uint64_t first_PTS;
    uint64_t PTS_length;
    audio_input_t* audio;  // Used whilst decoding.
    char *filename;
} fileinfo_t;

typedef struct
{
    _Bool manual;
    _Bool active;
    uint8_t starteffect;
    uint8_t endeffect;
    uint8_t lag;
    uint16_t onset;
} stilloptions;

typedef struct
{
    _Bool refresh;
    _Bool loop;
    _Bool hierarchical;
    _Bool active;
    char** highlightpic;
    char** selectpic;
    char** imagepic;
    char** backgroundpic; // The background of the top menu, type is .jpg. There can be many.
    char* blankscreen;    // In principe blank for adding titles yet can have some background, type is .png
    char** backgroundmpg;
    char** backgroundcolors;
    char* activeheader;
    char** topmenu;
    char*** topmenu_slide;
    char* stillvob;
    char* tsvob;
    char*** soundtrack;
    char* audioformat;
    char* albumcolor;
    char* groupcolor;
    char* arrowcolor;

    char* textcolor_pic;
    char* bgcolor_pic;
    char* highlightcolor_pic;
    char* selectfgcolor_pic;

    char* activetextcolor_palette;
    char* activebgcolor_palette;
    char* activehighlightcolor_palette;
    char* activeselectfgcolor_palette;

    char* textcolor_palette;
    char* bgcolor_palette;
    char* highlightcolor_palette;
    char* selectfgcolor_palette;

    char* textfont;
    char* screentextchain;
    char* framerate;
    char* norm;
    char* aspect;
    char* aspectratio;
    uint8_t pointsize;
    uint8_t fontwidth;
    int8_t highlightformat;
    uint8_t h;
    uint8_t min;
    uint8_t sec;
    uint8_t action;
    uint8_t nmenus;
    uint8_t ncolumns;
    uint16_t count;
    uint16_t* npics;
    uint16_t* topmenu_nslides;
    uint32_t* stillpicvobsize;
    uint32_t* menuvobsize;
    stilloptions** options;

} pic;

typedef struct
{
    uint8_t ngroups;
    uint8_t n_g_groups;
    uint8_t nplaygroups;
    uint8_t *playtitleset;
    uint8_t nvideolinking_groups;
    uint8_t maximum_VTSI_rank;
    uint8_t *VTSI_rank;
    uint8_t *ntracks;
    pic*    img;
    fileinfo_t **files;
    char** textable;
}command_t;




typedef struct
{
    uint8_t ngroups;
    uint8_t *ntracks;

} parse_t;

typedef struct
{
    char  *settingsfile;
    char  *logfile;
    char  *indir;
    char  *outdir;
    char  *lplexoutdir;
    char  *workdir;
    char  *tempdir;
    char  *lplextempdir;
    char  *linkdir;
    char  *bindir;
    char  *datadir;
    char  *fixwav_database;
    char  *dvdisopath;
    char  *stillpicdir;
} defaults ;

typedef struct
{
    int8_t topmenu;
    _Bool nooutput;
    _Bool runmkisofs;
    _Bool autoplay;
    _Bool text;
    _Bool silence;
    _Bool enable_lexer;
    _Bool logfile;
    _Bool loghtml;
    _Bool videozone;
    _Bool videolinking;
    _Bool playlist;
    _Bool cga;
    _Bool end_pause;
    _Bool maxverbose;
    _Bool veryverbose;
    _Bool debugging;
    #if 0
    _Bool padding;
    _Bool padding_continuous;
    _Bool minimal_padding;
    _Bool lossy_rounding;
    #endif
    _Bool rungrowisofs;
#ifndef WITHOUT_sox
    _Bool sox_enable;
#endif
#ifndef WITHOUT_libfixwav
    _Bool fixwav_enable;
    _Bool fixwav_virtual_enable;
    _Bool fixwav_automatic; /* automatic behaviour */
    _Bool fixwav_prepend; /* do not prepend a header */
    _Bool fixwav_in_place; /* do not correct in place */
    _Bool fixwav_cautious; /* be cautious on overwrites */
    _Bool fixwav_interactive; /* not interactive */
    _Bool fixwav_padding; /* padding */
    _Bool fixwav_prune; /* prune */
    _Bool fixwav_force;
    char* fixwav_suffix; /* output suffix for corrected files */
    char* fixwav_parameters;
#endif
    char* xml;
    char** spu_xml;
    char* cdrecorddevice;
    FILE *journal;
    uint16_t access_rights;
    defaults settings;

} globalData ;



typedef struct
{
    uint16_t nlines;
    char **commandline;
} lexer_t;

typedef struct
{
    uint8_t samg;
    uint8_t amg;
    uint8_t asvs;
    uint8_t atsi[9];
    uint32_t stillvob;
    uint32_t topvob;

} sect;

#endif // STRUCTURES_H_INCLUDED
