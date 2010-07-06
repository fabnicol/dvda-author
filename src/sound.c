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

extern globalData globals;

int audit_soundtrack(char* path)
{

            path_t *s=parse_filepath(path);
            errno=0;
            if (s->exists)
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
                { printf("[ERR]  File %s does not exist.\n", path); errno=1; }

           free(s);

return errno;
}

int launch_lplex_soundtrack(pic* img)

{
errno=0;
int u, menu;
char* lplex=NULL;
lplex=create_binary_path(lplex, LPLEX, SEPARATOR LPLEX_BASENAME);

if(!lplex) return -1;

char **args=calloc(2*img->nmenus+12+1+1, sizeof(char*));

char *args0[12]={LPLEX_BASENAME, "--create=", "mpeg", "--verbose=", (globals.debugging)?"true":"false", "--workPath=", globals.settings.tempdir, "-x", "false", "--video=", img->norm, "seamless"};

for (u=0; u < 12; u++) args[u]=args0[u];
for (menu=0; menu < img->nmenus; menu++)
{
args[12+2*menu]=strdup("jpg");
args[12+2*menu+1]=img->backgroundpic[menu];
args[12+2*img->nmenus]=img->soundtrack;
args[12+2*img->nmenus+1]=NULL;
}

printf("[INF]  Launching lplex to create top menu #%d with soundtrack...\n", menu);
run(lplex, args, 0);

return errno;
}




