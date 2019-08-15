#include "ffmpeg.h"

int main(int argc, char** argv)
{

char* tab[2] = {"ffmpeg_lib", "-h"};
  
return ffmpeg_lib(2, &tab[0]);

}
