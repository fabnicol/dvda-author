#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED


#include "stream_decoder.h"
#include "inttypes.h"
#include "commonvars.h"

typedef struct
{
    uint8_t  nextractgroup;
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
    FILE** channel_fp;
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
    uint8_t* channel_header_size;
    uint8_t type;
    uint8_t bitspersample;
    uint8_t channels;
    bool   mergeflag;
    bool   dvdv_compliant;
    // L&T Fedkamp addition
    uint8_t cga;
    uint8_t newtitle;
    uint8_t contin_track;
    // L&T Feldkamp addition (multichannel)
    uint16_t sampleunitsize;
    uint32_t samplerate;
    uint32_t first_sector;
    uint32_t last_sector;
    // L&T Feldkamp addition (multichannel)
    uint32_t lpcm_payload;
    uint8_t firstpackdecrement;
    uint32_t SCRquantity;
    uint32_t firstpack_audiopesheaderquantity;
    uint32_t midpack_audiopesheaderquantity;
    uint32_t lastpack_audiopesheaderquantity;
    uint8_t firstpack_lpcm_headerquantity;
    uint8_t midpack_lpcm_headerquantity;
    uint8_t lastpack_lpcm_headerquantity;
    uint8_t firstpack_pes_padding;
    uint8_t midpack_pes_padding;
    // L&T Feldkamp addition (multichannel)
    uint64_t numsamples;
    uint64_t numbytes; // theoretical audio size
    uint64_t wav_numbytes; // wav audio size
    uint64_t file_size; // file size on disc
    uint64_t *channel_size; // channel size on disc
    uint64_t bytesperframe;
    uint64_t bytespersecond;
    uint64_t first_PTS;
    uint64_t PTS_length;
    audio_input_t* audio;  // Used whilst decoding.
    char *filename;
    char **given_channel;
} fileinfo_t;

typedef struct
{
    bool manual;
    bool active;
    uint8_t starteffect;
    uint8_t endeffect;
    uint8_t lag;
    uint16_t onset;
} stilloptions;

typedef struct
{
    bool refresh;
    bool loop;
    bool hierarchical;
    bool active;
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
    char  *outfile;
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
    bool nooutput;
    bool runmkisofs;
    bool autoplay;
    bool text;
    bool silence;
    bool enable_lexer;
    bool logfile;
    bool loghtml;
    bool logdecode;
    bool videozone;
    bool videolinking;
    bool playlist;
    bool cga;
    bool end_pause;
    bool maxverbose;
    bool veryverbose;
    bool debugging;
    bool padding;
    bool padding_continuous;
    bool lossy_rounding;
    bool rungrowisofs;
#ifndef WITHOUT_sox
    bool sox_enable;
#endif

    bool fixwav_enable;
    bool fixwav_virtual_enable;
    bool fixwav_automatic; /* automatic behaviour */
    bool fixwav_prepend; /* do not prepend a header */
    bool fixwav_in_place; /* do not correct in place */
    bool fixwav_cautious; /* be cautious on overwrites */
    bool fixwav_interactive; /* interactive */
    bool fixwav_padding; /* padding */
    bool fixwav_prune; /* prune */
    bool fixwav_force;
    uint32_t textablesize;
    uint32_t topmenusize;
    uint32_t *grouptextsize;
    uint32_t *tracktextsize;
    uint32_t backgroundmpgsize;
    uint32_t backgroundpicsize;
    uint32_t *soundtracksize;
    uint32_t topmenu_slidesize;
    uint32_t highlightpicsize;
    uint32_t selectpicsize;
    uint32_t imagepicsize;
    uint32_t backgroundcolorssize;
    char* fixwav_suffix; /* output suffix for corrected files */
    char* fixwav_parameters;

    char* xml;
    char** spu_xml;
    char* cdrecorddevice;
    char** aobpath;
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
