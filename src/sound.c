#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sound.h"
#if HAVE_libfixwav
#include "fixwav.h"
#include "fixwav_manager.h"
#endif
#include "c_utils.h"
#include "auxiliary.h"
#include "launch_manager.h"
#include <errno.h>
#include <stdlib.h>
#ifndef __WIN32__
#include <unistd.h>
#endif
extern globalData globals;

// Checks whether video soundtracks comply with standard for AUDIO_TS.VOB authoring

int audit_soundtrack(char* path, _Bool strict)
{
#if HAVE_libfixwav
    path_t *s=parse_filepath(path);
    errno=0;
    if (s->isfile)
    {
        WaveHeader waveheader;
        WaveData wavedata=
        {
            path,
            strdup("useless"),
            NULL,
            NULL,
            1, /* automatic behaviour */
            0,
            0,
            0,
            0,
            1,
            0,
            1, /* whether header should be fixed virtually */
            0,  /* repair status */
            0, /* padbytes */
            0, /* pruned bytes */
        };

        fixwav(&wavedata, &waveheader);
        
        if (strict)
        {
            if ((waveheader.sample_fq == 48000) && (waveheader.bit_p_spl == 16) && (waveheader.channels == 2))
            {
                if (globals.veryverbose) foutput("%s", "[MSG]  LPCM requirements [fq=48k, bps=16, c=2] are satisfied by soundtrack input\n");
                errno=0;
            }
            else
            {
                foutput("%s", "[ERR]  LPCM requirements [fq=48k, bps=16, c=2] are not satisfied by soundtrack input\n");
                errno=1;
            }
       }
       else
        {
            if ((waveheader.sample_fq == 48000 || waveheader.sample_fq == 96000) 
             && (waveheader.bit_p_spl == 16 || waveheader.bit_p_spl == 24))
            {
                if (globals.veryverbose) foutput("%s", "[MSG]  LPCM requirements [fq=48|96k, bps=16|24] are satisfied by soundtrack input\n");
                errno=0;
            }
            else
            {
                foutput("%s", "[ERR]  LPCM requirements [fq=48|96k, bps=16|24] are not satisfied by soundtrack input\n");
                errno=1;
            }
       }
    }
    else
    {
        foutput("[ERR]  File %s does not exist.\n", path);
        errno=1;
    }

    free(s);
#endif
    return errno;

}

char* lplex=NULL;

int lplex_initialise()
{
#if HAVE_lplex
    errno=0;
    lplex=create_binary_path(lplex, LPLEX, SEPARATOR LPLEX_BASENAME);
    if(!lplex) return -1;
  
#endif
}

int launch_lplex_soundtrack(const pic* img, const char* create_mode)
{
#if HAVE_lplex    
    
    if (-1 == lplex_initialise()) return -1;
  
    char *args0[12]= {LPLEX_BASENAME, "--create", create_mode, "--verbose", (globals.debugging)?"true":"false", "--workPath", globals.settings.tempdir, "-x", "false", "--video", img->norm, "seamless"};  
    int u, menu, tot=0;
    img->backgroundmpg=calloc(img->nmenus, sizeof(char*));
    for (menu=0; menu < img->nmenus; menu++)
    {
        if ((img->topmenu_nslides[menu] > 1) && img->nmenus > 1)
        {
            foutput("%s\n", "[WAR]  Software limitation: you cannot author discs\n       with several slides for several menus.\n       Resetting slide number to 1.\n");
            img->topmenu_nslides[menu] =1;
        }

        char* args[img->topmenu_nslides[menu]*3+12+1];
        for (u=0; u < 12; u++) args[u]=args0[u];
        for (u=0; u < img->topmenu_nslides[menu]; u++)
        {
            args[12+tot]="jpg";
            args[12+tot+1]=img->topmenu_slide[menu][u];
            args[12+tot+2]=img->soundtrack[menu][u];
            tot +=3;
        }

        args[12+tot]=NULL;
        if (globals.debugging)
        {
            foutput("[INF]  Launching lplex to create top menu #%d with soundtrack...\n", menu);
            foutput("[INF]  with command line %s\n", get_full_command_line(args));
        }

        change_directory(globals.settings.workdir);
        run(lplex, args, 0);
        tot=0;
        path_t* aux=parse_filepath(img->soundtrack[menu][0]);

        if (aux->directory == NULL)
        {
            free(aux); // resorting to relative filenames withing current working dir
            aux=parse_filepath(globals.settings.workdir);
            if (aux->filename == NULL)
            {
                foutput("%s", "[ERR]  Use non-root audio folder, with appropriate access rights.\n");
                return -1;
            }
            else
            {
                aux->directory=aux->filename;
                foutput("[ING]  Using filepaths relative to %s.\n", globals.settings.workdir);
            }
        }

        char adjacent[2*strlen(aux->directory)+strlen(globals.settings.tempdir)+4+20+2+1];

        sprintf(adjacent, "%s%s%s%s%s%s%s", globals.settings.tempdir, SEPARATOR, aux->directory, "_DVD", SEPARATOR, aux->directory, "_DVD_title_01-00.mpg");

#ifndef __WIN32__

        // This is crucial for *nix otherwise lplex still holds the file streams blocked (tested)

        sync();

        // End of *nix code
#endif

        char* dest=copy_file2dir(adjacent, globals.settings.tempdir); // automatic renaming of dest
        img->backgroundmpg[menu]=strdup(dest);
        free(aux);
        free(dest);
    }
#endif
    return errno;
}

/*  Create disc hybrid using track paths of priorly converted (16-24 bits/48-96 kHz) audio files */

int launch_lplex_hybridate(const pic* img, const char* create_mode,
                           const char** trackpath,int* ntracks,int ntitlesets)

{
#if HAVE_lplex

    if (-1 == lplex_initialise()) return -1;
    
    char *args0[12]= {LPLEX_BASENAME, "--create", create_mode, "--verbose", (globals.debugging)?"true":"false", "--workPath", globals.settings.tempdir, "-x", "false", "--video", img->norm, "seamless"};
    
    int group, tr, argssize=0;
    
    for (group=0; group < ntitlesets; group++)
    {
      for (tr=0; tr < ntracks[group]; tr++)
      {
          argssize += ntracks[group][tr]+ (tr)+ (img->video_slide[group][tr] != NULL)*2;
      }
    }
        
    char* args[12+argssize+1];
    int tot=0;
        
    for (u=0; u < 12; u++) args[u]=args0[u];
    
    for (group=0; tr < ntitlesets; group++)
    {
      if (group) args[tot]="ts";
      
      for (tr=0; tr < ntracks[group]; tr++)
      {
        if (img->video_slide[group][tr]) 
        {
          args[12+tot]="jpg";
          agrs[12+tot+1]=img->video_slide[group][tr];
        }
        args[12+tot+2]=trackpath[group][track];
        tot +=3;
      }
    }
    
    args[12+tot]=NULL; 
    
    if (globals.debugging)
        {
            foutput("%s","[INF]  Launching lplex to create hybrid...\n");
            foutput("[INF]  with command line %s\n", get_full_command_line(args));
        }

        change_directory(globals.settings.workdir);
        run(lplex, args, 0);
        tot=0;
        
#if 0        
        path_t* aux=parse_filepath(img->soundtrack[menu][0]);
        
        if (aux->directory == NULL)
        {
            free(aux); // resorting to relative filenames withing current working dir
            aux=parse_filepath(globals.settings.workdir);
            if (aux->filename == NULL)
            {
                foutput("%s", "[ERR]  Use non-root audio folder, with appropriate access rights.\n");
                return -1;
            }
            else
            {
                aux->directory=aux->filename;
                foutput("[ING]  Using filepaths relative to %s.\n", globals.settings.workdir);
            }
        }

        char adjacent[2*strlen(aux->directory)+strlen(globals.settings.tempdir)+4+20+2+1];

        sprintf(adjacent, "%s%s%s%s%s%s%s", globals.settings.tempdir, SEPARATOR, aux->directory, "_DVD", SEPARATOR, aux->directory, "_DVD_title_01-00.mpg");

#ifndef __WIN32__

        // This is crucial for *nix otherwise lplex still holds the file streams blocked (tested)

        sync();

        // End of *nix code
#endif

        char* dest=copy_file2dir(adjacent, globals.settings.tempdir); // automatic renaming of dest
        img->backgroundmpg[menu]=strdup(dest);
        free(aux);
        free(dest);

#endif
#endif
    return errno;
}







