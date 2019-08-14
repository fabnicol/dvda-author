#include "ffmpeg.h"
#include "../libavcodec/mlplayout.h"
#include <stdio.h> 
#include <sys/stat.h>

int main(int argc, char** argv)
{
// -v 48 for debug
char* tab[10] = {"ffmpeg_lib", "-v", "48", "-i", "/home/fab/Dev/dvda-author/Docs/a2_24_44.mlp", "-strict", "-2", "-f", "null", "-"};
puts("searching...");  

ffmpeg_lib(10, &tab[0]);

if (argc > 1) exit(0);
puts("getting...");  

struct MLP_LAYOUT *m = calloc(MAX_AOB_SECTORS, sizeof(struct MLP_LAYOUT));
struct stat st;

if (stat(tab[4], &st)) /*failure*/
     return -1; // when file does not exist or is not accessible

get_mlp_layout(m, st.st_size / 1968 + 3); // 1968 is the minimum payload. +3 for last sector buffer

fprintf(stdout, "Size of file: %d B, %d sectors\n", st.st_size, st.st_size / 2048);

fprintf(stderr, "%s ; %s ; %s; %s; \n", "PKT_POS_PTS", "NB_SAMPLES_PTS",  "NB_SAMPLES_SCR", "RANK");  

for (int i = 0;  i < MAX_AOB_SECTORS; ++i) {
    if (i && m[i].pkt_pos == 0) return 0;
    fprintf(stderr, "%u ; %u ; %u ; %u \n", m[i].pkt_pos, m[i].nb_samples, m[i].nb_samples >= 40 ?  m[i].nb_samples - 40 : 0, m[i].rank);
}
free(m);
return -1;
}
