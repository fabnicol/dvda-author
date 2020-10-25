#include "config.h"

#include "compat.h"

#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <locale.h>
#include <langinfo.h>

/*
    Useful string stuff
*/

void strconcat
  (
    char * dest,
    size_t maxdestlen,
    const char * src
  )
  /* appends null-terminated src onto dest, ensuring length of contents
    of latter (including terminating null) do not exceed maxdestlen. */
  {
    const size_t destlen = strlen(dest);
    size_t srclen = strlen(src);
    assert(destlen < maxdestlen);
    if (srclen > maxdestlen - 1 - destlen)
      {
        srclen = maxdestlen - 1 - destlen;
      } /*if*/
    memcpy(dest + destlen, src, srclen);
    dest[destlen + srclen] = 0;
  } /*strconcat*/

unsigned int strtounsigned
  (
    const char * s,
    const char * what /* description of what I'm trying to convert, for error message */
  )
  /* parses s as an unsigned decimal integer, returning its value. Aborts the
    program on error. */
  {
    char * s_end;
    unsigned long result;
    errno = 0;
    result = strtoul(s, &s_end, 10);
    if (errno == 0)
      {
        if (*s_end != '\0')
          {
            errno = EINVAL;
          }
        else if (result > UINT_MAX)
          {
            errno = ERANGE;
          }
      } /*if*/
    if (errno != 0)
      {
        fprintf(stderr, "ERR:  %d converting %s \"%s\" -- %s\n", errno, what, s, strerror(errno));
        exit(1);
      } /*if*/
    return result;
  } /*strtounsigned*/

int strtosigned
  (
    const char * s,
    const char * what /* description of what I'm trying to convert, for error message */
  )
  /* parses s as a signed decimal integer, returning its value. Aborts the
    program on error. */
  {
    char * s_end;
    long result;
    errno = 0;
    result = strtol(s, &s_end, 10);
    if (errno == 0)
      {
        if (*s_end != '\0')
          {
            errno = EINVAL;
          }
        else if (result < -INT_MAX || result > INT_MAX)
          {
            errno = ERANGE;
          }
      } /*if*/
    if (errno != 0)
      {
        fprintf(stderr, "ERR:  %d converting %s \"%s\" -- %s\n", errno, what, s, strerror(errno));
        exit(1);
      } /*if*/
    return result;
  } /*strtosigned*/

#ifndef HAVE_STRNDUP
char * strndup
  (
    const char * s,
    size_t n
  )
  {
    char * result;
    size_t l = strlen(s);
    if (l > n)
      {
        l = n;
      } /*if*/
    result = malloc(l + 1);
    memcpy(result, s, l);
    result[l] = 0;
    return
        result;
  } /*strndup*/
#endif /*HAVE_STRNDUP*/

char * str_extract_until
  (
    const char ** src,
    const char * delim
  )
  /* scans *src, looking for the first occurrence of a character in delim. Returns
    a copy of the prior part of *src if found, and updates *src to point after the
    delimiter character; else returns a copy of the whole of *src, and sets *src
    to NULL. Returns NULL iff *src is NULL. */
  {
    char * result = NULL; /* to begin with */
    if (*src != NULL)
      {
        const size_t pos = strcspn(*src, delim);
        if (pos < strlen(*src))
          {
            result = strndup(*src, pos);
            *src = *src + pos + strspn(*src + pos, delim);
          }
        else
          {
            result = strdup(*src);
            *src = NULL;
          } /*if*/
      } /*if*/
    return
        result;
  } /*str_extract_until*/

/*
    locale stuff
*/

#ifdef HAVE_ICONV

const char *
    default_charset;
static char
    default_charset_buf[32]; /* big enough? */
static iconv_t
    from_locale = ICONV_NULL;

#endif /*HAVE_ICONV*/

void init_locale()
  /* does locale initialization and obtains the default character set. */
  {
#ifdef HAVE_ICONV
    setlocale(LC_ALL, "");
    strncpy(default_charset_buf, nl_langinfo(CODESET), sizeof default_charset_buf - 1);
    default_charset_buf[sizeof default_charset_buf - 1] = 0;
    default_charset = default_charset_buf;
    // fprintf(stderr, "INFO: default codeset is \"%s\"\n", default_charset); /* debug */
    setlocale(LC_ALL, "C"); /* don't need locale for anything else */
#else
    // fprintf(stderr, "INFO: all text will be interpreted as UTF-8\n"); /* debug */
#endif /*HAVE_ICONV*/
  } /*init_locale*/

char * locale_decode
  (
    const char * localestr
  )
  /* allocates and returns a string containing the UTF-8 representation of
    localestr interpreted according to the user's locale settings. */
  /* not actually used anywhere */
  {
    char * result;
#ifdef HAVE_ICONV
    size_t inlen, outlen;
    char * resultnext;
    if (from_locale == ICONV_NULL)
      {
        from_locale = iconv_open("UTF-8", default_charset);
        if (from_locale == ICONV_NULL)
          {
            fprintf(stderr, "ERR:  Cannot convert from charset \"%s\" to UTF-8\n", default_charset);
            exit(1);
          } /*if*/
      } /*if*/
    inlen = strlen(localestr);
    outlen = inlen * 5; /* should be enough? */
    result = malloc(outlen);
    resultnext = result;
    if (iconv(from_locale, (char **)&localestr, &inlen, &resultnext, &outlen) < 0)
      {
        fprintf
          (
            stderr,
            "ERR:  Error %d -- %s decoding string \"%s\"\n",
            errno, strerror(errno), localestr
          );
        exit(1);
      } /*if*/
    assert(outlen != 0); /* there will be room for ... */
    *resultnext++ = 0; /* ... terminating null */
    result = realloc(result, resultnext - result); /* free unneeded memory */
#else
    result = strdup(localestr);
#endif /*HAVE_ICONV*/
    return result;
  } /*locale_decode*/

#if HAVE_ICONV && LOCALIZE_FILENAMES

static iconv_t
    to_locale = ICONV_NULL;

char * localize_filename(const char * pathname)
  /* converts a filename from UTF-8 to localized encoding. */
  {
    char * result;
    size_t inlen, outlen;
    char * resultnext;
    if (to_locale == ICONV_NULL)
      {
        fprintf(stderr, "INFO: Converting filenames to %s\n", default_charset);
        to_locale = iconv_open(default_charset, "UTF-8");
        if (to_locale == ICONV_NULL)
          {
            fprintf(stderr, "ERR:  Cannot convert from UTF-8 to charset \"%s\"\n", default_charset);
            exit(1);
          } /*if*/
      } /*if*/
    inlen = strlen(pathname);
    outlen = inlen * 5; /* should be enough? */
    result = malloc(outlen);
    resultnext = result;
    if (iconv(to_locale, (char **)&pathname, &inlen, &resultnext, &outlen) < 0)
      {
        fprintf
          (
            stderr,
            "ERR:  Error %d -- %s encoding pathname \"%s\"\n",
            errno, strerror(errno), pathname
          );
        exit(1);
      } /*if*/
    assert(outlen != 0); /* there will be room for ... */
    *resultnext++ = 0; /* ... terminating null */
    result = realloc(result, resultnext - result); /* free unneeded memory */
    return result;
  } /*localize_filename*/

#endif

/*
    vfiles
*/

struct vfile varied_open
  (
    const char * fname,
    int mode, /* either O_RDONLY or O_WRONLY, nothing more */
    const char * what /* description of what I'm trying to open, for error message */
  )
  {
    struct vfile vf;
    int fnamelen;
    if (strcmp(fname, "-") == 0)
      {
        vf.ftype = VFTYPE_REDIR;
        vf.h = mode == O_RDONLY ? stdin : stdout;
      }
    else if (fname[0] == '&' && isdigit(fname[1]))
      {
        vf.ftype = VFTYPE_FILE;
        vf.h = fdopen(atoi(fname + 1), mode == O_RDONLY ? "rb" : "wb");
      }
    else if (mode == O_WRONLY && fname[0] == '|')
      {
        vf.ftype = VFTYPE_PIPE;
        vf.h = popen(fname + 1, "w");
      }
    else if (mode == O_RDONLY && fname[0] != '\0' && fname[fnamelen = strlen(fname) - 1] == '|')
      {
        char * const fcopy = strndup(fname, fnamelen);
        vf.ftype = VFTYPE_PIPE;
        vf.h = popen(fcopy, "r");
        free(fcopy);
      }
    else
      {
        vf.ftype = VFTYPE_FILE;
        vf.h = fopen(fname, mode == O_RDONLY ? "rb" : "wb");
      } /*if*/
    if (vf.h == NULL)
      {
        fprintf(stderr, "ERR:  %d opening %s \"%s\" -- %s\n", errno, what, fname, strerror(errno));
        exit(1);
      } /*if*/
    vf.mode = mode;
    return vf;
  } /*varied_open*/

void varied_close(struct vfile vf)
  {
    if (vf.mode == O_WRONLY)
      {
      /* check for errors on final write before closing */
        if (fflush(vf.h) != 0)
          {
            fprintf(stderr, "ERR:  %d on fflush -- %s\n", errno, strerror(errno));
            exit(1);
          } /*if*/
      } /*if*/
    switch (vf.ftype)
      {
    case VFTYPE_FILE:
        fclose(vf.h);
    break;
    case VFTYPE_PIPE:
        pclose(vf.h);
    break;
    case VFTYPE_REDIR:
    default:
      /* nothing to do */
    break;
      } /*switch*/
  } /*varied_close*/

/*
    parsing of colour specs
*/

typedef struct
  {
    const char * name; /* null marks end of list */
    colorspec color;
  } named_color;
const static named_color predefined_color_names[] =
  /* predefined colour names, copied from <http://www.imagemagick.org/script/color.php> */
  {
    {"snow", {255, 250, 250, 255}},
    {"snow1", {255, 250, 250, 255}},
    {"snow2", {238, 233, 233, 255}},
    {"RosyBrown1", {255, 193, 193, 255}},
    {"RosyBrown2", {238, 180, 180, 255}},
    {"snow3", {205, 201, 201, 255}},
    {"LightCoral", {240, 128, 128, 255}},
    {"IndianRed1", {255, 106, 106, 255}},
    {"RosyBrown3", {205, 155, 155, 255}},
    {"IndianRed2", {238, 99, 99, 255}},
    {"RosyBrown", {188, 143, 143, 255}},
    {"brown1", {255, 64, 64, 255}},
    {"firebrick1", {255, 48, 48, 255}},
    {"brown2", {238, 59, 59, 255}},
    {"IndianRed", {205, 92, 92, 255}},
    {"IndianRed3", {205, 85, 85, 255}},
    {"firebrick2", {238, 44, 44, 255}},
    {"snow4", {139, 137, 137, 255}},
    {"brown3", {205, 51, 51, 255}},
    {"red", {255, 0, 0, 255}},
    {"red1", {255, 0, 0, 255}},
    {"RosyBrown4", {139, 105, 105, 255}},
    {"firebrick3", {205, 38, 38, 255}},
    {"red2", {238, 0, 0, 255}},
    {"firebrick", {178, 34, 34, 255}},
    {"brown", {165, 42, 42, 255}},
    {"red3", {205, 0, 0, 255}},
    {"IndianRed4", {139, 58, 58, 255}},
    {"brown4", {139, 35, 35, 255}},
    {"firebrick4", {139, 26, 26, 255}},
    {"DarkRed", {139, 0, 0, 255}},
    {"red4", {139, 0, 0, 255}},
    {"maroon", {128, 0, 0, 255}},
    {"LightPink1", {255, 174, 185, 255}},
    {"LightPink3", {205, 140, 149, 255}},
    {"LightPink4", {139, 95, 101, 255}},
    {"LightPink2", {238, 162, 173, 255}},
    {"LightPink", {255, 182, 193, 255}},
    {"pink", {255, 192, 203, 255}},
    {"crimson", {220, 20, 60, 255}},
    {"pink1", {255, 181, 197, 255}},
    {"pink2", {238, 169, 184, 255}},
    {"pink3", {205, 145, 158, 255}},
    {"pink4", {139, 99, 108, 255}},
    {"PaleVioletRed4", {139, 71, 93, 255}},
    {"PaleVioletRed", {219, 112, 147, 255}},
    {"PaleVioletRed2", {238, 121, 159, 255}},
    {"PaleVioletRed1", {255, 130, 171, 255}},
    {"PaleVioletRed3", {205, 104, 137, 255}},
    {"LavenderBlush", {255, 240, 245, 255}},
    {"LavenderBlush1", {255, 240, 245, 255}},
    {"LavenderBlush3", {205, 193, 197, 255}},
    {"LavenderBlush2", {238, 224, 229, 255}},
    {"LavenderBlush4", {139, 131, 134, 255}},
    {"maroon", {176, 48, 96, 255}},
    {"HotPink3", {205, 96, 144, 255}},
    {"VioletRed3", {205, 50, 120, 255}},
    {"VioletRed1", {255, 62, 150, 255}},
    {"VioletRed2", {238, 58, 140, 255}},
    {"VioletRed4", {139, 34, 82, 255}},
    {"HotPink2", {238, 106, 167, 255}},
    {"HotPink1", {255, 110, 180, 255}},
    {"HotPink4", {139, 58, 98, 255}},
    {"HotPink", {255, 105, 180, 255}},
    {"DeepPink", {255, 20, 147, 255}},
    {"DeepPink1", {255, 20, 147, 255}},
    {"DeepPink2", {238, 18, 137, 255}},
    {"DeepPink3", {205, 16, 118, 255}},
    {"DeepPink4", {139, 10, 80, 255}},
    {"maroon1", {255, 52, 179, 255}},
    {"maroon2", {238, 48, 167, 255}},
    {"maroon3", {205, 41, 144, 255}},
    {"maroon4", {139, 28, 98, 255}},
    {"MediumVioletRed", {199, 21, 133, 255}},
    {"VioletRed", {208, 32, 144, 255}},
    {"orchid2", {238, 122, 233, 255}},
    {"orchid", {218, 112, 214, 255}},
    {"orchid1", {255, 131, 250, 255}},
    {"orchid3", {205, 105, 201, 255}},
    {"orchid4", {139, 71, 137, 255}},
    {"thistle1", {255, 225, 255, 255}},
    {"thistle2", {238, 210, 238, 255}},
    {"plum1", {255, 187, 255, 255}},
    {"plum2", {238, 174, 238, 255}},
    {"thistle", {216, 191, 216, 255}},
    {"thistle3", {205, 181, 205, 255}},
    {"plum", {221, 160, 221, 255}},
    {"violet", {238, 130, 238, 255}},
    {"plum3", {205, 150, 205, 255}},
    {"thistle4", {139, 123, 139, 255}},
    {"fuchsia", {255, 0, 255, 255}},
    {"magenta", {255, 0, 255, 255}},
    {"magenta1", {255, 0, 255, 255}},
    {"plum4", {139, 102, 139, 255}},
    {"magenta2", {238, 0, 238, 255}},
    {"magenta3", {205, 0, 205, 255}},
    {"DarkMagenta", {139, 0, 139, 255}},
    {"magenta4", {139, 0, 139, 255}},
    {"purple", {128, 0, 128, 255}},
    {"MediumOrchid", {186, 85, 211, 255}},
    {"MediumOrchid1", {224, 102, 255, 255}},
    {"MediumOrchid2", {209, 95, 238, 255}},
    {"MediumOrchid3", {180, 82, 205, 255}},
    {"MediumOrchid4", {122, 55, 139, 255}},
    {"DarkViolet", {148, 0, 211, 255}},
    {"DarkOrchid", {153, 50, 204, 255}},
    {"DarkOrchid1", {191, 62, 255, 255}},
    {"DarkOrchid3", {154, 50, 205, 255}},
    {"DarkOrchid2", {178, 58, 238, 255}},
    {"DarkOrchid4", {104, 34, 139, 255}},
    {"purple", {160, 32, 240, 255}},
    {"indigo", { 75, 0, 130, 255}},
    {"BlueViolet", {138, 43, 226, 255}},
    {"purple2", {145, 44, 238, 255}},
    {"purple3", {125, 38, 205, 255}},
    {"purple4", { 85, 26, 139, 255}},
    {"purple1", {155, 48, 255, 255}},
    {"MediumPurple", {147, 112, 219, 255}},
    {"MediumPurple1", {171, 130, 255, 255}},
    {"MediumPurple2", {159, 121, 238, 255}},
    {"MediumPurple3", {137, 104, 205, 255}},
    {"MediumPurple4", { 93, 71, 139, 255}},
    {"DarkSlateBlue", { 72, 61, 139, 255}},
    {"LightSlateBlue", {132, 112, 255, 255}},
    {"MediumSlateBlue", {123, 104, 238, 255}},
    {"SlateBlue", {106, 90, 205, 255}},
    {"SlateBlue1", {131, 111, 255, 255}},
    {"SlateBlue2", {122, 103, 238, 255}},
    {"SlateBlue3", {105, 89, 205, 255}},
    {"SlateBlue4", { 71, 60, 139, 255}},
    {"GhostWhite", {248, 248, 255, 255}},
    {"lavender", {230, 230, 250, 255}},
    {"blue", { 0, 0, 255, 255}},
    {"blue1", { 0, 0, 255, 255}},
    {"blue2", { 0, 0, 238, 255}},
    {"blue3", { 0, 0, 205, 255}},
    {"MediumBlue", { 0, 0, 205, 255}},
    {"blue4", { 0, 0, 139, 255}},
    {"DarkBlue", { 0, 0, 139, 255}},
    {"MidnightBlue", { 25, 25, 112, 255}},
    {"navy", { 0, 0, 128, 255}},
    {"NavyBlue", { 0, 0, 128, 255}},
    {"RoyalBlue", { 65, 105, 225, 255}},
    {"RoyalBlue1", { 72, 118, 255, 255}},
    {"RoyalBlue2", { 67, 110, 238, 255}},
    {"RoyalBlue3", { 58, 95, 205, 255}},
    {"RoyalBlue4", { 39, 64, 139, 255}},
    {"CornflowerBlue", {100, 149, 237, 255}},
    {"LightSteelBlue", {176, 196, 222, 255}},
    {"LightSteelBlue1", {202, 225, 255, 255}},
    {"LightSteelBlue2", {188, 210, 238, 255}},
    {"LightSteelBlue3", {162, 181, 205, 255}},
    {"LightSteelBlue4", {110, 123, 139, 255}},
    {"SlateGray4", {108, 123, 139, 255}},
    {"SlateGray1", {198, 226, 255, 255}},
    {"SlateGray2", {185, 211, 238, 255}},
    {"SlateGray3", {159, 182, 205, 255}},
    {"LightSlateGray", {119, 136, 153, 255}},
    {"LightSlateGrey", {119, 136, 153, 255}},
    {"SlateGray", {112, 128, 144, 255}},
    {"SlateGrey", {112, 128, 144, 255}},
    {"DodgerBlue", { 30, 144, 255, 255}},
    {"DodgerBlue1", { 30, 144, 255, 255}},
    {"DodgerBlue2", { 28, 134, 238, 255}},
    {"DodgerBlue4", { 16, 78, 139, 255}},
    {"DodgerBlue3", { 24, 116, 205, 255}},
    {"AliceBlue", {240, 248, 255, 255}},
    {"SteelBlue4", { 54, 100, 139, 255}},
    {"SteelBlue", { 70, 130, 180, 255}},
    {"SteelBlue1", { 99, 184, 255, 255}},
    {"SteelBlue2", { 92, 172, 238, 255}},
    {"SteelBlue3", { 79, 148, 205, 255}},
    {"SkyBlue4", { 74, 112, 139, 255}},
    {"SkyBlue1", {135, 206, 255, 255}},
    {"SkyBlue2", {126, 192, 238, 255}},
    {"SkyBlue3", {108, 166, 205, 255}},
    {"LightSkyBlue", {135, 206, 250, 255}},
    {"LightSkyBlue4", { 96, 123, 139, 255}},
    {"LightSkyBlue1", {176, 226, 255, 255}},
    {"LightSkyBlue2", {164, 211, 238, 255}},
    {"LightSkyBlue3", {141, 182, 205, 255}},
    {"SkyBlue", {135, 206, 235, 255}},
    {"LightBlue3", {154, 192, 205, 255}},
    {"DeepSkyBlue", { 0, 191, 255, 255}},
    {"DeepSkyBlue1", { 0, 191, 255, 255}},
    {"DeepSkyBlue2", { 0, 178, 238, 255}},
    {"DeepSkyBlue4", { 0, 104, 139, 255}},
    {"DeepSkyBlue3", { 0, 154, 205, 255}},
    {"LightBlue1", {191, 239, 255, 255}},
    {"LightBlue2", {178, 223, 238, 255}},
    {"LightBlue", {173, 216, 230, 255}},
    {"LightBlue4", {104, 131, 139, 255}},
    {"PowderBlue", {176, 224, 230, 255}},
    {"CadetBlue1", {152, 245, 255, 255}},
    {"CadetBlue2", {142, 229, 238, 255}},
    {"CadetBlue3", {122, 197, 205, 255}},
    {"CadetBlue4", { 83, 134, 139, 255}},
    {"turquoise1", { 0, 245, 255, 255}},
    {"turquoise2", { 0, 229, 238, 255}},
    {"turquoise3", { 0, 197, 205, 255}},
    {"turquoise4", { 0, 134, 139, 255}},
  /* {"cadet blue", { 95, 158, 160, 255}}, */ /* don't allow space in name */
    {"CadetBlue", { 95, 158, 160, 255}},
    {"DarkTurquoise", { 0, 206, 209, 255}},
    {"azure", {240, 255, 255, 255}},
    {"azure1", {240, 255, 255, 255}},
    {"LightCyan", {224, 255, 255, 255}},
    {"LightCyan1", {224, 255, 255, 255}},
    {"azure2", {224, 238, 238, 255}},
    {"LightCyan2", {209, 238, 238, 255}},
    {"PaleTurquoise1", {187, 255, 255, 255}},
    {"PaleTurquoise", {175, 238, 238, 255}},
    {"PaleTurquoise2", {174, 238, 238, 255}},
    {"DarkSlateGray1", {151, 255, 255, 255}},
    {"azure3", {193, 205, 205, 255}},
    {"LightCyan3", {180, 205, 205, 255}},
    {"DarkSlateGray2", {141, 238, 238, 255}},
    {"PaleTurquoise3", {150, 205, 205, 255}},
    {"DarkSlateGray3", {121, 205, 205, 255}},
    {"azure4", {131, 139, 139, 255}},
    {"LightCyan4", {122, 139, 139, 255}},
    {"aqua", { 0, 255, 255, 255}},
    {"cyan", { 0, 255, 255, 255}},
    {"cyan1", { 0, 255, 255, 255}},
    {"PaleTurquoise4", {102, 139, 139, 255}},
    {"cyan2", { 0, 238, 238, 255}},
    {"DarkSlateGray4", { 82, 139, 139, 255}},
    {"cyan3", { 0, 205, 205, 255}},
    {"cyan4", { 0, 139, 139, 255}},
    {"DarkCyan", { 0, 139, 139, 255}},
    {"teal", { 0, 128, 128, 255}},
    {"DarkSlateGray", { 47, 79, 79, 255}},
    {"DarkSlateGrey", { 47, 79, 79, 255}},
    {"MediumTurquoise", { 72, 209, 204, 255}},
    {"LightSeaGreen", { 32, 178, 170, 255}},
    {"turquoise", { 64, 224, 208, 255}},
    {"aquamarine4", { 69, 139, 116, 255}},
    {"aquamarine", {127, 255, 212, 255}},
    {"aquamarine1", {127, 255, 212, 255}},
    {"aquamarine2", {118, 238, 198, 255}},
    {"aquamarine3", {102, 205, 170, 255}},
    {"MediumAquamarine", {102, 205, 170, 255}},
    {"MediumSpringGreen", { 0, 250, 154, 255}},
    {"MintCream", {245, 255, 250, 255}},
    {"SpringGreen", { 0, 255, 127, 255}},
    {"SpringGreen1", { 0, 255, 127, 255}},
    {"SpringGreen2", { 0, 238, 118, 255}},
    {"SpringGreen3", { 0, 205, 102, 255}},
    {"SpringGreen4", { 0, 139, 69, 255}},
    {"MediumSeaGreen", { 60, 179, 113, 255}},
    {"SeaGreen", { 46, 139, 87, 255}},
    {"SeaGreen3", { 67, 205, 128, 255}},
    {"SeaGreen1", { 84, 255, 159, 255}},
    {"SeaGreen4", { 46, 139, 87, 255}},
    {"SeaGreen2", { 78, 238, 148, 255}},
    {"MediumForestGreen", { 50, 129, 75, 255}},
    {"honeydew", {240, 255, 240, 255}},
    {"honeydew1", {240, 255, 240, 255}},
    {"honeydew2", {224, 238, 224, 255}},
    {"DarkSeaGreen1", {193, 255, 193, 255}},
    {"DarkSeaGreen2", {180, 238, 180, 255}},
    {"PaleGreen1", {154, 255, 154, 255}},
    {"PaleGreen", {152, 251, 152, 255}},
    {"honeydew3", {193, 205, 193, 255}},
    {"LightGreen", {144, 238, 144, 255}},
    {"PaleGreen2", {144, 238, 144, 255}},
    {"DarkSeaGreen3", {155, 205, 155, 255}},
    {"DarkSeaGreen", {143, 188, 143, 255}},
    {"PaleGreen3", {124, 205, 124, 255}},
    {"honeydew4", {131, 139, 131, 255}},
    {"green1", { 0, 255, 0, 255}},
    {"lime", { 0, 255, 0, 255}},
    {"LimeGreen", { 50, 205, 50, 255}},
    {"DarkSeaGreen4", {105, 139, 105, 255}},
    {"green2", { 0, 238, 0, 255}},
    {"PaleGreen4", { 84, 139, 84, 255}},
    {"green3", { 0, 205, 0, 255}},
    {"ForestGreen", { 34, 139, 34, 255}},
    {"green4", { 0, 139, 0, 255}},
    {"green", { 0, 128, 0, 255}},
    {"DarkGreen", { 0, 100, 0, 255}},
    {"LawnGreen", {124, 252, 0, 255}},
    {"chartreuse", {127, 255, 0, 255}},
    {"chartreuse1", {127, 255, 0, 255}},
    {"chartreuse2", {118, 238, 0, 255}},
    {"chartreuse3", {102, 205, 0, 255}},
    {"chartreuse4", { 69, 139, 0, 255}},
    {"GreenYellow", {173, 255, 47, 255}},
    {"DarkOliveGreen3", {162, 205, 90, 255}},
    {"DarkOliveGreen1", {202, 255, 112, 255}},
    {"DarkOliveGreen2", {188, 238, 104, 255}},
    {"DarkOliveGreen4", {110, 139, 61, 255}},
    {"DarkOliveGreen", { 85, 107, 47, 255}},
    {"OliveDrab", {107, 142, 35, 255}},
    {"OliveDrab1", {192, 255, 62, 255}},
    {"OliveDrab2", {179, 238, 58, 255}},
    {"OliveDrab3", {154, 205, 50, 255}},
    {"YellowGreen", {154, 205, 50, 255}},
    {"OliveDrab4", {105, 139, 34, 255}},
    {"ivory", {255, 255, 240, 255}},
    {"ivory1", {255, 255, 240, 255}},
    {"LightYellow", {255, 255, 224, 255}},
    {"LightYellow1", {255, 255, 224, 255}},
    {"beige", {245, 245, 220, 255}},
    {"ivory2", {238, 238, 224, 255}},
    {"LightGoldenrodYellow", {250, 250, 210, 255}},
    {"LightYellow2", {238, 238, 209, 255}},
    {"ivory3", {205, 205, 193, 255}},
    {"LightYellow3", {205, 205, 180, 255}},
    {"ivory4", {139, 139, 131, 255}},
    {"LightYellow4", {139, 139, 122, 255}},
    {"yellow", {255, 255, 0, 255}},
    {"yellow1", {255, 255, 0, 255}},
    {"yellow2", {238, 238, 0, 255}},
    {"yellow3", {205, 205, 0, 255}},
    {"yellow4", {139, 139, 0, 255}},
    {"olive", {128, 128, 0, 255}},
    {"DarkKhaki", {189, 183, 107, 255}},
    {"khaki2", {238, 230, 133, 255}},
    {"LemonChiffon4", {139, 137, 112, 255}},
    {"khaki1", {255, 246, 143, 255}},
    {"khaki3", {205, 198, 115, 255}},
    {"khaki4", {139, 134, 78, 255}},
    {"PaleGoldenrod", {238, 232, 170, 255}},
    {"LemonChiffon", {255, 250, 205, 255}},
    {"LemonChiffon1", {255, 250, 205, 255}},
    {"khaki", {240, 230, 140, 255}},
    {"LemonChiffon3", {205, 201, 165, 255}},
    {"LemonChiffon2", {238, 233, 191, 255}},
    {"MediumGoldenRod", {209, 193, 102, 255}},
    {"cornsilk4", {139, 136, 120, 255}},
    {"gold", {255, 215, 0, 255}},
    {"gold1", {255, 215, 0, 255}},
    {"gold2", {238, 201, 0, 255}},
    {"gold3", {205, 173, 0, 255}},
    {"gold4", {139, 117, 0, 255}},
    {"LightGoldenrod", {238, 221, 130, 255}},
    {"LightGoldenrod4", {139, 129, 76, 255}},
    {"LightGoldenrod1", {255, 236, 139, 255}},
    {"LightGoldenrod3", {205, 190, 112, 255}},
    {"LightGoldenrod2", {238, 220, 130, 255}},
    {"cornsilk3", {205, 200, 177, 255}},
    {"cornsilk2", {238, 232, 205, 255}},
    {"cornsilk", {255, 248, 220, 255}},
    {"cornsilk1", {255, 248, 220, 255}},
    {"goldenrod", {218, 165, 32, 255}},
    {"goldenrod1", {255, 193, 37, 255}},
    {"goldenrod2", {238, 180, 34, 255}},
    {"goldenrod3", {205, 155, 29, 255}},
    {"goldenrod4", {139, 105, 20, 255}},
    {"DarkGoldenrod", {184, 134, 11, 255}},
    {"DarkGoldenrod1", {255, 185, 15, 255}},
    {"DarkGoldenrod2", {238, 173, 14, 255}},
    {"DarkGoldenrod3", {205, 149, 12, 255}},
    {"DarkGoldenrod4", {139, 101, 8, 255}},
    {"FloralWhite", {255, 250, 240, 255}},
    {"wheat2", {238, 216, 174, 255}},
    {"OldLace", {253, 245, 230, 255}},
    {"wheat", {245, 222, 179, 255}},
    {"wheat1", {255, 231, 186, 255}},
    {"wheat3", {205, 186, 150, 255}},
    {"orange", {255, 165, 0, 255}},
    {"orange1", {255, 165, 0, 255}},
    {"orange2", {238, 154, 0, 255}},
    {"orange3", {205, 133, 0, 255}},
    {"orange4", {139, 90, 0, 255}},
    {"wheat4", {139, 126, 102, 255}},
    {"moccasin", {255, 228, 181, 255}},
    {"PapayaWhip", {255, 239, 213, 255}},
    {"NavajoWhite3", {205, 179, 139, 255}},
    {"BlanchedAlmond", {255, 235, 205, 255}},
    {"NavajoWhite", {255, 222, 173, 255}},
    {"NavajoWhite1", {255, 222, 173, 255}},
    {"NavajoWhite2", {238, 207, 161, 255}},
    {"NavajoWhite4", {139, 121, 94, 255}},
    {"AntiqueWhite4", {139, 131, 120, 255}},
    {"AntiqueWhite", {250, 235, 215, 255}},
    {"tan", {210, 180, 140, 255}},
    {"bisque4", {139, 125, 107, 255}},
    {"burlywood", {222, 184, 135, 255}},
    {"AntiqueWhite2", {238, 223, 204, 255}},
    {"burlywood1", {255, 211, 155, 255}},
    {"burlywood3", {205, 170, 125, 255}},
    {"burlywood2", {238, 197, 145, 255}},
    {"AntiqueWhite1", {255, 239, 219, 255}},
    {"burlywood4", {139, 115, 85, 255}},
    {"AntiqueWhite3", {205, 192, 176, 255}},
    {"DarkOrange", {255, 140, 0, 255}},
    {"bisque2", {238, 213, 183, 255}},
    {"bisque", {255, 228, 196, 255}},
    {"bisque1", {255, 228, 196, 255}},
    {"bisque3", {205, 183, 158, 255}},
    {"DarkOrange1", {255, 127, 0, 255}},
    {"linen", {250, 240, 230, 255}},
    {"DarkOrange2", {238, 118, 0, 255}},
    {"DarkOrange3", {205, 102, 0, 255}},
    {"DarkOrange4", {139, 69, 0, 255}},
    {"peru", {205, 133, 63, 255}},
    {"tan1", {255, 165, 79, 255}},
    {"tan2", {238, 154, 73, 255}},
    {"tan3", {205, 133, 63, 255}},
    {"tan4", {139, 90, 43, 255}},
    {"PeachPuff", {255, 218, 185, 255}},
    {"PeachPuff1", {255, 218, 185, 255}},
    {"PeachPuff4", {139, 119, 101, 255}},
    {"PeachPuff2", {238, 203, 173, 255}},
    {"PeachPuff3", {205, 175, 149, 255}},
    {"SandyBrown", {244, 164, 96, 255}},
    {"seashell4", {139, 134, 130, 255}},
    {"seashell2", {238, 229, 222, 255}},
    {"seashell3", {205, 197, 191, 255}},
    {"chocolate", {210, 105, 30, 255}},
    {"chocolate1", {255, 127, 36, 255}},
    {"chocolate2", {238, 118, 33, 255}},
    {"chocolate3", {205, 102, 29, 255}},
    {"chocolate4", {139, 69, 19, 255}},
    {"SaddleBrown", {139, 69, 19, 255}},
    {"seashell", {255, 245, 238, 255}},
    {"seashell1", {255, 245, 238, 255}},
    {"sienna4", {139, 71, 38, 255}},
    {"sienna", {160, 82, 45, 255}},
    {"sienna1", {255, 130, 71, 255}},
    {"sienna2", {238, 121, 66, 255}},
    {"sienna3", {205, 104, 57, 255}},
    {"LightSalmon3", {205, 129, 98, 255}},
    {"LightSalmon", {255, 160, 122, 255}},
    {"LightSalmon1", {255, 160, 122, 255}},
    {"LightSalmon4", {139, 87, 66, 255}},
    {"LightSalmon2", {238, 149, 114, 255}},
    {"coral", {255, 127, 80, 255}},
    {"OrangeRed", {255, 69, 0, 255}},
    {"OrangeRed1", {255, 69, 0, 255}},
    {"OrangeRed2", {238, 64, 0, 255}},
    {"OrangeRed3", {205, 55, 0, 255}},
    {"OrangeRed4", {139, 37, 0, 255}},
    {"DarkSalmon", {233, 150, 122, 255}},
    {"salmon1", {255, 140, 105, 255}},
    {"salmon2", {238, 130, 98, 255}},
    {"salmon3", {205, 112, 84, 255}},
    {"salmon4", {139, 76, 57, 255}},
    {"coral1", {255, 114, 86, 255}},
    {"coral2", {238, 106, 80, 255}},
    {"coral3", {205, 91, 69, 255}},
    {"coral4", {139, 62, 47, 255}},
    {"tomato4", {139, 54, 38, 255}},
    {"tomato", {255, 99, 71, 255}},
    {"tomato1", {255, 99, 71, 255}},
    {"tomato2", {238, 92, 66, 255}},
    {"tomato3", {205, 79, 57, 255}},
    {"MistyRose4", {139, 125, 123, 255}},
    {"MistyRose2", {238, 213, 210, 255}},
    {"MistyRose", {255, 228, 225, 255}},
    {"MistyRose1", {255, 228, 225, 255}},
    {"salmon", {250, 128, 114, 255}},
    {"MistyRose3", {205, 183, 181, 255}},
    {"white", {255, 255, 255, 255}},
    {"gray100", {255, 255, 255, 255}},
    {"grey100", {255, 255, 255, 255}},
    {"grey100", {255, 255, 255, 255}},
    {"gray99", {252, 252, 252, 255}},
    {"grey99", {252, 252, 252, 255}},
    {"gray98", {250, 250, 250, 255}},
    {"grey98", {250, 250, 250, 255}},
    {"gray97", {247, 247, 247, 255}},
    {"grey97", {247, 247, 247, 255}},
    {"gray96", {245, 245, 245, 255}},
    {"grey96", {245, 245, 245, 255}},
    {"WhiteSmoke", {245, 245, 245, 255}},
    {"gray95", {242, 242, 242, 255}},
    {"grey95", {242, 242, 242, 255}},
    {"gray94", {240, 240, 240, 255}},
    {"grey94", {240, 240, 240, 255}},
    {"gray93", {237, 237, 237, 255}},
    {"grey93", {237, 237, 237, 255}},
    {"gray92", {235, 235, 235, 255}},
    {"grey92", {235, 235, 235, 255}},
    {"gray91", {232, 232, 232, 255}},
    {"grey91", {232, 232, 232, 255}},
    {"gray90", {229, 229, 229, 255}},
    {"grey90", {229, 229, 229, 255}},
    {"gray89", {227, 227, 227, 255}},
    {"grey89", {227, 227, 227, 255}},
    {"gray88", {224, 224, 224, 255}},
    {"grey88", {224, 224, 224, 255}},
    {"gray87", {222, 222, 222, 255}},
    {"grey87", {222, 222, 222, 255}},
    {"gainsboro", {220, 220, 220, 255}},
    {"gray86", {219, 219, 219, 255}},
    {"grey86", {219, 219, 219, 255}},
    {"gray85", {217, 217, 217, 255}},
    {"grey85", {217, 217, 217, 255}},
    {"gray84", {214, 214, 214, 255}},
    {"grey84", {214, 214, 214, 255}},
    {"gray83", {212, 212, 212, 255}},
    {"grey83", {212, 212, 212, 255}},
    {"LightGray", {211, 211, 211, 255}},
    {"LightGrey", {211, 211, 211, 255}},
    {"gray82", {209, 209, 209, 255}},
    {"grey82", {209, 209, 209, 255}},
    {"gray81", {207, 207, 207, 255}},
    {"grey81", {207, 207, 207, 255}},
    {"gray80", {204, 204, 204, 255}},
    {"grey80", {204, 204, 204, 255}},
    {"gray79", {201, 201, 201, 255}},
    {"grey79", {201, 201, 201, 255}},
    {"gray78", {199, 199, 199, 255}},
    {"grey78", {199, 199, 199, 255}},
    {"gray77", {196, 196, 196, 255}},
    {"grey77", {196, 196, 196, 255}},
    {"gray76", {194, 194, 194, 255}},
    {"grey76", {194, 194, 194, 255}},
    {"silver", {192, 192, 192, 255}},
    {"gray75", {191, 191, 191, 255}},
    {"grey75", {191, 191, 191, 255}},
    {"gray74", {189, 189, 189, 255}},
    {"grey74", {189, 189, 189, 255}},
    {"gray73", {186, 186, 186, 255}},
    {"grey73", {186, 186, 186, 255}},
    {"gray72", {184, 184, 184, 255}},
    {"grey72", {184, 184, 184, 255}},
    {"gray71", {181, 181, 181, 255}},
    {"grey71", {181, 181, 181, 255}},
    {"gray70", {179, 179, 179, 255}},
    {"grey70", {179, 179, 179, 255}},
    {"gray69", {176, 176, 176, 255}},
    {"grey69", {176, 176, 176, 255}},
    {"gray68", {173, 173, 173, 255}},
    {"grey68", {173, 173, 173, 255}},
    {"gray67", {171, 171, 171, 255}},
    {"grey67", {171, 171, 171, 255}},
    {"DarkGray", {169, 169, 169, 255}},
    {"DarkGrey", {169, 169, 169, 255}},
    {"gray66", {168, 168, 168, 255}},
    {"grey66", {168, 168, 168, 255}},
    {"gray65", {166, 166, 166, 255}},
    {"grey65", {166, 166, 166, 255}},
    {"gray64", {163, 163, 163, 255}},
    {"grey64", {163, 163, 163, 255}},
    {"gray63", {161, 161, 161, 255}},
    {"grey63", {161, 161, 161, 255}},
    {"gray62", {158, 158, 158, 255}},
    {"grey62", {158, 158, 158, 255}},
    {"gray61", {156, 156, 156, 255}},
    {"grey61", {156, 156, 156, 255}},
    {"gray60", {153, 153, 153, 255}},
    {"grey60", {153, 153, 153, 255}},
    {"gray59", {150, 150, 150, 255}},
    {"grey59", {150, 150, 150, 255}},
    {"gray58", {148, 148, 148, 255}},
    {"grey58", {148, 148, 148, 255}},
    {"gray57", {145, 145, 145, 255}},
    {"grey57", {145, 145, 145, 255}},
    {"gray56", {143, 143, 143, 255}},
    {"grey56", {143, 143, 143, 255}},
    {"gray55", {140, 140, 140, 255}},
    {"grey55", {140, 140, 140, 255}},
    {"gray54", {138, 138, 138, 255}},
    {"grey54", {138, 138, 138, 255}},
    {"gray53", {135, 135, 135, 255}},
    {"grey53", {135, 135, 135, 255}},
    {"gray52", {133, 133, 133, 255}},
    {"grey52", {133, 133, 133, 255}},
    {"gray51", {130, 130, 130, 255}},
    {"grey51", {130, 130, 130, 255}},
    {"fractal", {128, 128, 128, 255}},
    {"gray50", {127, 127, 127, 255}},
    {"grey50", {127, 127, 127, 255}},
    {"gray", {126, 126, 126, 255}},
    {"gray49", {125, 125, 125, 255}},
    {"grey49", {125, 125, 125, 255}},
    {"gray48", {122, 122, 122, 255}},
    {"grey48", {122, 122, 122, 255}},
    {"gray47", {120, 120, 120, 255}},
    {"grey47", {120, 120, 120, 255}},
    {"gray46", {117, 117, 117, 255}},
    {"grey46", {117, 117, 117, 255}},
    {"gray45", {115, 115, 115, 255}},
    {"grey45", {115, 115, 115, 255}},
    {"gray44", {112, 112, 112, 255}},
    {"grey44", {112, 112, 112, 255}},
    {"gray43", {110, 110, 110, 255}},
    {"grey43", {110, 110, 110, 255}},
    {"gray42", {107, 107, 107, 255}},
    {"grey42", {107, 107, 107, 255}},
    {"DimGray", {105, 105, 105, 255}},
    {"DimGrey", {105, 105, 105, 255}},
    {"gray41", {105, 105, 105, 255}},
    {"grey41", {105, 105, 105, 255}},
    {"gray40", {102, 102, 102, 255}},
    {"grey40", {102, 102, 102, 255}},
    {"gray39", { 99, 99, 99, 255}},
    {"grey39", { 99, 99, 99, 255}},
    {"gray38", { 97, 97, 97, 255}},
    {"grey38", { 97, 97, 97, 255}},
    {"gray37", { 94, 94, 94, 255}},
    {"grey37", { 94, 94, 94, 255}},
    {"gray36", { 92, 92, 92, 255}},
    {"grey36", { 92, 92, 92, 255}},
    {"gray35", { 89, 89, 89, 255}},
    {"grey35", { 89, 89, 89, 255}},
    {"gray34", { 87, 87, 87, 255}},
    {"grey34", { 87, 87, 87, 255}},
    {"gray33", { 84, 84, 84, 255}},
    {"grey33", { 84, 84, 84, 255}},
    {"gray32", { 82, 82, 82, 255}},
    {"grey32", { 82, 82, 82, 255}},
    {"gray31", { 79, 79, 79, 255}},
    {"grey31", { 79, 79, 79, 255}},
    {"gray30", { 77, 77, 77, 255}},
    {"grey30", { 77, 77, 77, 255}},
    {"gray29", { 74, 74, 74, 255}},
    {"grey29", { 74, 74, 74, 255}},
    {"gray28", { 71, 71, 71, 255}},
    {"grey28", { 71, 71, 71, 255}},
    {"gray27", { 69, 69, 69, 255}},
    {"grey27", { 69, 69, 69, 255}},
    {"gray26", { 66, 66, 66, 255}},
    {"grey26", { 66, 66, 66, 255}},
    {"gray25", { 64, 64, 64, 255}},
    {"grey25", { 64, 64, 64, 255}},
    {"gray24", { 61, 61, 61, 255}},
    {"grey24", { 61, 61, 61, 255}},
    {"gray23", { 59, 59, 59, 255}},
    {"grey23", { 59, 59, 59, 255}},
    {"gray22", { 56, 56, 56, 255}},
    {"grey22", { 56, 56, 56, 255}},
    {"gray21", { 54, 54, 54, 255}},
    {"grey21", { 54, 54, 54, 255}},
    {"gray20", { 51, 51, 51, 255}},
    {"grey20", { 51, 51, 51, 255}},
    {"gray19", { 48, 48, 48, 255}},
    {"grey19", { 48, 48, 48, 255}},
    {"gray18", { 46, 46, 46, 255}},
    {"grey18", { 46, 46, 46, 255}},
    {"gray17", { 43, 43, 43, 255}},
    {"grey17", { 43, 43, 43, 255}},
    {"gray16", { 41, 41, 41, 255}},
    {"grey16", { 41, 41, 41, 255}},
    {"gray15", { 38, 38, 38, 255}},
    {"grey15", { 38, 38, 38, 255}},
    {"gray14", { 36, 36, 36, 255}},
    {"grey14", { 36, 36, 36, 255}},
    {"gray13", { 33, 33, 33, 255}},
    {"grey13", { 33, 33, 33, 255}},
    {"gray12", { 31, 31, 31, 255}},
    {"grey12", { 31, 31, 31, 255}},
    {"gray11", { 28, 28, 28, 255}},
    {"grey11", { 28, 28, 28, 255}},
    {"gray10", { 26, 26, 26, 255}},
    {"grey10", { 26, 26, 26, 255}},
    {"gray9", { 23, 23, 23, 255}},
    {"grey9", { 23, 23, 23, 255}},
    {"gray8", { 20, 20, 20, 255}},
    {"grey8", { 20, 20, 20, 255}},
    {"gray7", { 18, 18, 18, 255}},
    {"grey7", { 18, 18, 18, 255}},
    {"gray6", { 15, 15, 15, 255}},
    {"grey6", { 15, 15, 15, 255}},
    {"gray5", { 13, 13, 13, 255}},
    {"grey5", { 13, 13, 13, 255}},
    {"gray4", { 10, 10, 10, 255}},
    {"grey4", { 10, 10, 10, 255}},
    {"gray3", { 8, 8, 8, 255}},
    {"grey3", { 8, 8, 8, 255}},
    {"gray2", { 5, 5, 5, 255}},
    {"grey2", { 5, 5, 5, 255}},
    {"gray1", { 3, 3, 3, 255}},
    {"grey1", { 3, 3, 3, 255}},
    {"black", { 0, 0, 0, 255}},
    {"gray0", { 0, 0, 0, 255}},
    {"grey0", { 0, 0, 0, 255}},
    {"opaque", { 0, 0, 0, 255}},
    {"none", { 0, 0, 0, 0}},
    {"transparent", {0, 0, 0, 0}},
    {0, {0, 0, 0, 0}} /* marks end of list */
  };

static void skipseparators
  (
    const char ** src,
    const char * srcend
  )
  {
    unsigned char ch;
    for (;;)
      {
        if (*src == srcend)
            break;
       ch = **src;
       if (ch != ' ' && ch != '\n' && ch != '\t')
           break;
       ++*src;
      } /*for*/
  } /*skipseparators*/

static bool parse_color_component
  (
    const char ** src,
    const char * srcend,
    int maxvalue,
    int * value
  )
  /* parses a colour component which can be a number [0 .. maxvalue]
    or a percentage of maxvalue. */
  {
    bool ok;
    float fvalue;
    int factor;
    bool decptseen, percentseen;
    unsigned char ch;
    decptseen = false;
    percentseen = false;
    fvalue = 0;
    do /*once*/
      {
        ok = false; /* in case no valid characters seen */
        for (;;)
          {
            if (*src == srcend)
                break;
            ch = **src;
            ++*src;
            ok = true; /* if not already set */
            if (ch >= '0' && ch <= '9')
              {
                const int val = ch - '0';
                if (decptseen)
                  {
                    fvalue += (float)val / factor;
                    factor *= 10;
                  }
                else
                  {
                    fvalue = fvalue * 10 + val;
                  } /*if*/
              }
            else if (ch == '.')
              {
                if (decptseen)
                  {
                    ok = false;
                    break;
                  } /*if*/
                decptseen = true;
                factor = 10;
              }
            else if (ch == '%')
              {
                percentseen = true;
                break;
              }
            else
              {
                --*src;
                break;
              } /*if*/
          } /*for*/
        if (!ok)
            break;
        if (fvalue > (percentseen ? 100 : maxvalue))
          {
            ok = false;
            break;
          } /*if*/
        *value = percentseen ? fvalue * maxvalue / 100 : fvalue;
      }
    while (false);
    return ok;
  } /*parse_color_component*/

colorspec parse_color
  (
    const char * colorstr,
    const char * what /* additional explanatory text for error message */
  )
  /* parses colorstr and returns the resulting colour. Will abort the process
    on any errors. */
  {
    colorspec result;
    const char * src;
    const char * const srcend = colorstr + strlen(colorstr);
    const char * errmsg = 0;
    char c;
    int colorindex;
    bool hex, nonhex, paren;
    do /*once*/
      {
        hex = false;
        nonhex = false;
        paren = false;
        src = colorstr;
        for (;;)
          {
            if (src == srcend)
                break;
            c = *src;
            if (c == '#')
              {
                if (src != colorstr) /* must occur at start */
                  {
                    errmsg = "illegal \"#\"";
                    break;
                  } /*if*/
                hex = true;
              }
            else if (c == '(')
              {
                paren = true;
                break;
              }
            else if (c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c >= 'A' && c <= 'Z')
              {
                if (c > 'f' && c <= 'z' || c > 'F' && c <= 'Z')
                  {
                    nonhex = true;
                  } /*if*/
              }
            else
              {
                errmsg = "non-alphanumeric character";
                break;
              } /*if*/
            ++src;
          } /*for*/
        if (errmsg != 0)
            break;
        if (hex && (nonhex || paren))
          {
            errmsg = "doesn't make sense";
            break;
          } /*if*/
        if (paren)
          {
            enum
              {
                maxspacename = 5, /* max length of all the valid colour space names */
              };
            char spacename[maxspacename + 1];
            int nrcomponents, componentindex, component[4];
            bool gotcolors, gothue, gotlightness, gotalpha;
            if (src - colorstr > maxspacename)
              {
                errmsg = "illegal color-space name";
                break;
              } /*if*/
            memcpy(spacename, colorstr, src - colorstr);
            spacename[src - colorstr] = 0;
            if (strcasecmp(spacename, "rgb") == 0)
              {
                nrcomponents = 3;
                gotcolors = true;
                gothue = false;
                gotalpha = false;
              }
            else if (strcasecmp(spacename, "rgba") == 0)
              {
                nrcomponents = 4;
                gotcolors = true;
                gothue = false;
                gotalpha = true;
              }
            else if (strcasecmp(spacename, "gray") == 0)
              {
                nrcomponents = 1;
                gotcolors = false;
                gothue = false;
                gotalpha = false;
              }
            else if (strcasecmp(spacename, "graya") == 0)
              {
                nrcomponents = 2;
                gotcolors = false;
                gothue = false;
                gotalpha = true;
              }
            else if (strcasecmp(spacename, "hsb") == 0 || strcasecmp(spacename, "hsv") == 0)
              {
                nrcomponents = 3;
                gotcolors = true;
                gothue = true;
                gotlightness = false;
                gotalpha = false;
              }
            else if (strcasecmp(spacename, "hsba") == 0 || strcasecmp(spacename, "hsva") == 0)
              {
                nrcomponents = 4;
                gotcolors = true;
                gothue = true;
                gotlightness = false;
                gotalpha = true;
              }
            else if (strcasecmp(spacename, "hsl") == 0)
              {
                nrcomponents = 3;
                gotcolors = true;
                gothue = true;
                gotlightness = true;
                gotalpha = false;
              }
            else if (strcasecmp(spacename, "hsla") == 0)
              {
                nrcomponents = 4;
                gotcolors = true;
                gothue = true;
                gotlightness = true;
                gotalpha = true;
              }
            else
              {
                errmsg = "can't recognize color-space name";
                break;
              } /*if*/
            for (componentindex = 0;;)
              {
                if (componentindex == 0 ? *src != '(' : *src != ',')
                  {
                    errmsg = "illegal character in color spec";
                    break;
                  } /*if*/
                ++src; /* skip parenthesis/comma */
                skipseparators(&src, srcend);
                if (*src == ')')
                  {
                    errmsg = "too few color components";
                    break;
                  } /*if*/
                if
                  (
                    !parse_color_component
                      (
                        /*src =*/ &src,
                        /*srcend =*/ srcend,
                        /*maxvalue =*/ gothue && componentindex == 0 ? 360 : 255,
                        /*value =*/ component + componentindex
                      )
                  )
                  {
                    errmsg = "illegal number for color component";
                    break;
                  } /*if*/
                ++componentindex;
                if (componentindex == nrcomponents)
                    break;
              } /*for*/
            if (errmsg != 0)
                break;
            skipseparators(&src, srcend);
            if (*src++ != ')')
              {
                errmsg = "missing close parenthesis";
                break;
              } /*if*/
            if (src != srcend)
              {
                errmsg = "junk after close parenthesis";
                break;
              } /*if*/
            if (gothue)
              {
                component[0] %= 360;
              /* convert to RGB using formulas from Wikipedia */
                  {
                    const int hue = component[0];
                    const int chroma =
                            (gotlightness ?
                                    2
                                *
                                    (component[2] < 128 ? component[2] : 255 - component[2])
                            :
                                component[2]
                            )
                        *
                            component[1]
                        /
                            255;
                    const int second = chroma * (60 - abs(hue % 120 - 60)) / 60;
                    const int brighten =
                        gotlightness ?
                            component[2] - chroma / 2
                        :
                            component[2] - chroma;
                    int * primary, * secondary, * opposite;
                    if (hue < 60)
                      {
                        primary = component + 0;
                        secondary = component + 1;
                        opposite = component + 2;
                      }
                    else if (hue >= 60 && hue < 120)
                      {
                        primary = component + 1;
                        secondary = component + 0;
                        opposite = component + 2;
                      }
                    else if (hue >= 120 && hue < 180)
                      {
                        primary = component + 1;
                        secondary = component + 2;
                        opposite = component + 0;
                      }
                    else if (hue >= 180 && hue < 240)
                      {
                        primary = component + 2;
                        secondary = component + 1;
                        opposite = component + 0;
                      }
                    else if (hue >= 240 && hue < 300)
                      {
                        primary = component + 2;
                        secondary = component + 0;
                        opposite = component + 1;
                      }
                    else /* hue >= 300 && hue < 360 */
                      {
                        primary = component + 0;
                        secondary = component + 2;
                        opposite = component + 1;
                      } /*if*/
                    *primary = chroma + brighten;
                    *secondary = second + brighten;
                    *opposite = brighten;
                  }
              }
            else if (!gotcolors)
              {
                if (gotalpha)
                  {
                    component[3] = component[1];
                  } /*if*/
                component[2] = component[1] = component[0];
              } /*if*/
            if (!gotalpha)
              {
                component[3] = 255;
              } /*if*/
            result.r = component[0];
            result.g = component[1];
            result.b = component[2];
            result.a = component[3];
            break;
          } /*if*/
        if (nonhex)
          {
            bool found;
            for (colorindex = 0;;)
              {
                const named_color * const entry = predefined_color_names + colorindex;
                if (entry->name == 0)
                  {
                    found = false;
                    break;
                  } /*if*/
                if (strcasecmp(entry->name, colorstr) == 0)
                  {
                    result = entry->color;
                    found = true;
                    break;
                  } /*if*/
                ++colorindex;
              } /*for*/
            if (found)
                break;
            if (nonhex)
              {
                errmsg = "unrecognized color name";
                break;
              } /*if*/
          /* allow plain hex string with no preceding "#" for backward compatibility */
          } /*if*/
      /* must be hex */
          {
            int componentlength, componentscale, componentindex;
            src = colorstr + (colorstr[0] == '#' ? 1 : 0);
            if (srcend - src > 12 || (srcend - src) % 3 != 0)
              {
                errmsg = "bad hex color string";
                break;
              } /*if*/
            componentlength = (srcend - src) / 3;
            componentscale = (1 << 4 * componentlength) - 1;
            for (componentindex = 0; componentindex < 3; ++componentindex)
              {
                int component, j;
                unsigned char * dest;
                component = 0;
                for (j = 0; j < componentlength; ++j)
                  {
                    const unsigned char ch = *src++;
                    component =
                            component * 16
                        +
                            (ch >= 'a' && ch <= 'f' ?
                                ch - 'a' + 10
                            : ch >= 'A' && ch <= 'F' ?
                                ch - 'A' + 10
                            : /* ch >= '0' && ch <= '9' ? */
                                ch - '0'
                            );
                  } /*for*/
                switch (componentindex)
                  {
                case 0:
                    dest = &result.r;
                break;
                case 1:
                    dest = &result.g;
                break;
                case 2:
                    dest = &result.b;
                break;
                  } /*switch*/
                *dest = component * 255 / componentscale;
              } /*for*/
            result.a = 255;
          }
      }
    while (false);
    if (errmsg != 0)
      {
        fprintf(stderr, "ERR:  couldn't parse %s colorspec \"%s\" -- %s\n", what, colorstr, errmsg);
        exit(1);
      } /*if*/
    return result;
  } /*parse_color*/
