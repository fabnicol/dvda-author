#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sound.h"
#include "fixwav.h"
#include "c_utils.h"
#include "auxiliary.h"
#include "fixwav_manager.h"
#include "launch_manager.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

extern globalData globals;

int audit_soundtrack(char* path)
{

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

        if ((waveheader.sample_fq == 48000) && (waveheader.bit_p_spl == 16) && (waveheader.channels == 2))
        {
            if (globals.veryverbose) printf("%s", "[MSG]  LPCM requirements [fq=48k, bps=16, c=2] are satisfied by soundtrack input\n");
            errno=0;
        }
        else
        {
            printf("%s", "[ERR]  LPCM requirements [fq=48k, bps=16, c=2] are not satisfied by soundtrack input\n");
            errno=1;
        }
    }
    else
    {
        printf("[ERR]  File %s does not exist.\n", path);
        errno=1;
    }

    free(s);

    return errno;

}

int launch_lplex_soundtrack(pic* img)
{
    errno=0;
    int u, menu, tot=0;
    char* lplex=NULL;
    lplex=create_binary_path(lplex, LPLEX, SEPARATOR LPLEX_BASENAME);

    img->backgroundmpg=calloc(img->nmenus, sizeof(char*));

    if(!lplex) return -1;


    char *args0[12]= {LPLEX_BASENAME, "--create", "mpeg", "--verbose", (globals.debugging)?"true":"false", "--workPath", globals.settings.tempdir, "-x", "false", "--video", img->norm, "seamless"};


    for (menu=0; menu < img->nmenus; menu++)
    {
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

        printf("[INF]  Launching lplex to create top menu #%d with soundtrack...\n", menu);
        get_command_line(args);
        change_directory(globals.settings.workdir);

        run(lplex, args, 0);

        tot=0;

        path_t* aux=parse_filepath(img->soundtrack[menu][0]);

        //path_t* aux=parse_filepath("/home/fab/A.jpg");


        if (aux->directory == NULL)
        {
                free(aux); // resorting to relative filenames withing current working dir
                aux=parse_filepath(globals.settings.workdir);
                if (aux->filename == NULL)
                  { printf("%s", "[ERR]  Use non-root audio folder, with appropriate access rights.\n"); return -1;}
                else
                {
                  aux->directory=aux->filename;
                  printf("[ING]  Using filepaths relative to %s.\n", globals.settings.workdir);
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

    return errno;
}




