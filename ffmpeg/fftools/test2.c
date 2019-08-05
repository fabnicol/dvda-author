#include "ffmpeg.h"
#include "../libavcodec/mlplayout.h"
#include <stdio.h> 

int main(int argc, char** argv)
{
// -v 48 for debug
char* tab[10] = {"ffmpeg_lib", "-v", "-8", "-i", "/home/fab/Dev/DVDA/audio_mlp/a2_24_44.mlp", "-strict", "-2", "-f", "null", "-"};
puts("searching...");  
ffmpeg_lib(10, &tab[0]);
if (argc > 1) exit(0);
puts("getting...");  
struct MLP_LAYOUT* m = get_mlp_layout();
if (m == NULL) {
printf("%s\n", "m is null"); 
return -2;
}
puts("printing...");
fprintf(stderr, "%s ; %s ; %s; %s; %s \n", "PKT_POS_PTS", "NB_SAMPLES_PTS", "PKT_POS_SRC", "NB_SAMPLES_SRC", "RANK");  
for (int i = 0;  i < MAX_AOB_SECTORS; ++i) {
if (i && m[i].pkt_pos == 0) return 0;
fprintf(stderr, "%u ; %u ; %u \n", m[i].pkt_pos, m[i].nb_samples, /*m[i].pkt_pos_src, m[i].nb_samples_src,*/ m[i].rank);  
}
return -1;
}
