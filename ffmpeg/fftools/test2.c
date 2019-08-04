#include "ffmpeg.h"
#include "../libavcodec/mlplayout.h"
#include <stdio.h> 

int main(int argc, char** argv)
{

char* tab[8] = {"ffmpeg_lib", "-v", "-8", "-i", "/home/fab/Build3/ffmpeg/a2_16_44.mlp", "-f", "null", "-"};
puts("searching...");  
ffmpeg_lib(8, &tab[0]);
if (argc > 1) exit(0);
puts("getting...");  
struct MLP_LAYOUT* m = get_mlp_layout();
if (m == NULL) {
printf("%s\n", "m is null"); 
return -2;
}
puts("printing...");
for (int i = 0;  i < MAX_AOB_SECTORS; ++i) {
if (i && m[i].pkt_pos == 0) return 0;
fprintf(stderr, "%u ; %u ; %u \n", m[i].pkt_pos, m[i].nb_samples, m[i].rank);  
}
return -1;
}
