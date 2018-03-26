#ifndef LPLEX_PCH_INCLUDED
#define LPLEX_PCH_INCLUDED

#ifdef __cplusplus

using namespace std;

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <climits>
#include <streambuf>
#include <strstream>
#include <vector>
#include <algorithm>

#endif

#include <unistd.h>
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
                                 // prevents #error from dvdread/ifo_types.h
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef UINT8_MAX
#define UINT8_MAX 0xff /* 255U */
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xffff /* 65535U */
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_read.h>

#include <FLAC++/all.h>
#include <md5/md5.h>
#include <FLAC/format.h>
//#include <jpeglib.h>
#include <vlc_bits.h>


#endif
