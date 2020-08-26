/*
 * $Id: y4mstabilizer.c,v 1.12 2009/09/19 19:01:43 sms00 Exp $
 *
 * written by J. Macropol <jm@wx.gd-ais.com>
 *	Framework and shifting code adapted from y4mshift by Steve Schultz.
 *	Motion detection was adapted from yuvdenoise.
 *
 * Program to stabilize a video stream.
 * Works only non-interlaced yuv4mpeg streams for now.
 *
 * Usage: y4mstabilizer [-v] [-a <alpha>] [-r <srchRadius>]
 *
 *	-v		Verbose.  Add additional -v to increase verbosity/debug
 *	-a <alpha>	The alpha value is a "viscosity" measure (see below).
 *	-r <srchRadius>	How far to look for movement.
 *	-s <stride>	How far apart the motion search points are.
 *	-n		Do not supersample the chroma to get 1-pixel shifts.
 *	-i		Try alternate interlaced mode (does not work yet)
 *
 * Legal <alpha> values are beween 0 and 1.
 * Useful <alpha> values are beween 0.7 (or so) and .95.
 * Higher values resist panning more at the expense of greater
 * shifting.  <alpha> defaults to 0.95, a fairly high value, on the theory
 * that you probably have some significant vibrations that need smoothing.
 * Separate alpha values for the X- and Y-axes can be specified by separating
 * them with a colon.  For example,
 * 	-a 0.7:0.9
 * would set the X-axis alpha to 0.7, and the Y-axis alpha to 0.9.  Thus,
 * the stabilizer would be much more responsive to side-to-side movement,
 * while resisting up-and-down motion.
 *
 * The <srchRadius> defaults to 15.  Smaller values speed things up,
 * but won't be able to cope with large/fast movements as well.
 *
 * The <stride> defaults to 48 pixels.  Giving a larger number here will
 * speed the process tremendously, but makes it easier for the motion search
 * to be fooled by local movement.  Use a smaller number if the search seems
 * to be following local movements.
 *
 * No file arguments are needed since this is a filter only program.
 *
 * TODO:
 * 	Get alternate interlace method working.
 * 	Get chroma super/subsampleing working better.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "yuv4mpeg.h"
#include "subsample.h"

struct
    {
    int		verbose;	/* Talkative flag */
    int		nosuper;	/* Flag not to supersample chroma on shift */
    int		rad, diam;	/* Search radius and diameter */
    int		stride;		/* Stride between motion points */
    float	alphax;		/* X Viscosity */
    float	alphay;		/* Y Viscosity */
    float	gsX, gsY;	/* Accumulated shift */
    int		ss_h, ss_v;	/* UV decimation factors */
    } Stab;
#define SS_H Stab.ss_h
#define SS_V Stab.ss_v

typedef struct { int x, y; } vec;

static void usage(void);
static void alloc_yuv(u_char**, int, int);
static void subsample(uint8_t*, uint8_t*, int, int);
static void gmotion(u_char**, u_char**, int, int, int, vec*);
static void motion(u_char*, u_char*, int, int, int, int, vec*);
static void motion0(u_char*, u_char*, int, int, int, vec*);
static uint32_t calc_SAD_noaccel(uint8_t*, uint8_t*, int, int);
static uint32_t calc_SAD_half_noaccel(uint8_t*, uint8_t*, uint8_t*, int, int);
static void calcshift(vec*, vec*);
static int xround(float, int);
static void doshift(u_char**, u_char**, int, int, int, vec*);
static void hshift(u_char*, u_char*, int, int, int, int, int);
static void vertical_shift(u_char*, int, int, int, int, int);

int
main (int argc, char **argv)
    {
    int i, c, width, height, frames, err;
    vec g, shift;
    int interlace, iflag = 0, chroma_ss;
    u_char *yuv0[10], *yuv1[10], *yuv2[10], *line1;
    y4m_stream_info_t istream, ostream;
    y4m_frame_info_t iframe;
    int fdin = fileno(stdin);

    Stab.rad = 15;		/* Default search radius */
    Stab.stride = 48;		/* Default stride between motion points */
    Stab.alphax = Stab.alphay = 0.95; /* Default viscosity */

    y4m_accept_extensions(1);

    opterr = 0;
    while ((c = getopt(argc, argv, "va:r:bis:n")) != EOF)
	switch  (c)
	    {
	  case  'n':
	    Stab.nosuper = 1;
	    break;
	  case  'i':
	    iflag |= 0200;
	    break;
	  case  'v':
	    Stab.verbose++;
	    break;
	  case 's':
	    Stab.stride = atoi(optarg);
	    break;
	  case 'a':
	    if (strchr(optarg, ':'))
		{
		if (sscanf(optarg, "%g:%g", &Stab.alphax, &Stab.alphay) != 2)
		    usage();
		}
	    else
		{
		if (sscanf(optarg, "%g", &Stab.alphax) != 1)
		    usage();
		Stab.alphay = Stab.alphax;
		}
	    break;
	  case 'r':
	    Stab.rad = atoi(optarg);
	    break;
	  case    '?':
	  case    'h':
	  default:
	    usage();
	    }

    /* Initialize your input stream */
    y4m_init_stream_info(&istream);
    y4m_init_frame_info(&iframe);
    err = y4m_read_stream_header(fdin, &istream);
    if (err != Y4M_OK)
	mjpeg_error_exit1("Input stream error: %s\n", y4m_strerr(err));
    if (y4m_si_get_plane_count(&istream) != 3)
	mjpeg_error_exit1("Only 3 plane formats supported");
    switch (interlace = y4m_si_get_interlace(&istream))
	{
      case Y4M_ILACE_NONE:
	break;
      case Y4M_ILACE_TOP_FIRST:
      case Y4M_ILACE_BOTTOM_FIRST:
	interlace |= iflag;
	break;
      case Y4M_ILACE_MIXED:
	mjpeg_error_exit1("No mixed-interlaced streams!\n");
      default:
	mjpeg_error_exit1("Unknown interlace!\n");
	}
    chroma_ss = y4m_si_get_chroma(&istream);
    SS_H = y4m_chroma_ss_x_ratio(chroma_ss).d;
    SS_V = y4m_chroma_ss_y_ratio(chroma_ss).d;
    switch (chroma_ss)
	{
      case Y4M_CHROMA_420JPEG:
      case Y4M_CHROMA_420MPEG2:
      case Y4M_CHROMA_444:
	break;
      case Y4M_CHROMA_MONO:
	 mjpeg_error_exit1("MONO (1 plane) chroma not supported!\n");
      case Y4M_CHROMA_444ALPHA:
	 mjpeg_error_exit1("444ALPHA (4 plane) chroma not supported!\n");
      default:
	if (!Stab.nosuper)
	    mjpeg_info("Cannot supersample %s chroma",
		y4m_chroma_description(chroma_ss));
	Stab.nosuper = 1;
	break;
	}
    width = y4m_si_get_width(&istream);
    height = y4m_si_get_height(&istream);
    if (Stab.verbose)
	y4m_log_stream_info(mjpeg_loglev_t("info"), "", &istream);

    /* Initialize output stream */
    y4m_init_stream_info(&ostream);
    y4m_copy_stream_info(&ostream, &istream);
    y4m_write_stream_header(fileno(stdout), &ostream);

    /* Allocate our frame arrays */
    alloc_yuv(yuv0, height, width);
    alloc_yuv(yuv1, height, width);
    alloc_yuv(yuv2, height, width);

    /* Set up the search diameter. */
    Stab.diam = Stab.rad + Stab.rad + 1;

    /* Fetch 1st frame - nothing to compare, so just copy it out.
     * (Note that this is not strictly true if we have interlace
     * and use the -i modified mode where we treat the fields separately.
     * But I am *SURE* nobody will notice...  err...) */
    frames = 1;
    if (y4m_read_frame(fdin,&istream,&iframe,yuv0) != Y4M_OK)
	goto endit;
    subsample(yuv0[0], yuv0[3], width, height);
    subsample(yuv0[3], yuv0[4], width/2, height/2);
    y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv0);
    for (; y4m_read_frame(fdin,&istream,&iframe,yuv1) == Y4M_OK; frames++)
	{
	if ((Stab.verbose > 1) || (Stab.verbose && ((frames % 100) == 0)))
	    mjpeg_info("Frame %d", frames);
	subsample(yuv1[0], yuv1[3], width, height);
	subsample(yuv1[3], yuv1[4], width/2, height/2);
	switch (interlace)
	    {
	  /* Easy - non-interlaced */
	  case Y4M_ILACE_NONE:
	    /* Find out how much this frame has changed from the previous */
	    gmotion(yuv0, yuv1, width, width, height, &g);
	    /* Figure out how much to shift this frame to compensate */
	    calcshift(&g, &shift);
	    /* If nothing to shift, just dump this frame and continue */
	    if ((shift.x == 0) && (shift.y == 0))
		y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv1);
	    /* Else shift frame & write it out */
	    else
		{
		doshift(yuv1, yuv2, height, width, width, &shift);
		y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv2);
		}
	    break;
	  /* Default interlaced method.
	   * Treat fields as one wide field & shift both the same.  */
	  case Y4M_ILACE_TOP_FIRST    | 0:
	  case Y4M_ILACE_BOTTOM_FIRST | 0:
	    /* Find out how much this frame has changed from the previous */
	    gmotion(yuv0, yuv1, width*2, width*2, height/2, &g);
	    /* Figure out how much to shift this frame to compensate */
	    calcshift(&g, &shift);
	    /* If nothing to shift, just dump this frame and continue */
	    if ((shift.x == 0) && (shift.y == 0))
		y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv1);
	    /* Shift the fields separately & write the frame */
	    else
		{
		doshift(yuv1,   yuv2,   height/2, width,width*2,&shift);
		doshift(yuv1+5, yuv2+5, height/2, width,width*2,&shift);
		y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv2);
		}
	    break;
	    /* Alternate interlaced method:
	     * Treat fields as separate frames, one half pixel apart vertically. */
	  case Y4M_ILACE_TOP_FIRST    | 0200:
	    /* Last bottom half -> Top half */
	    gmotion(yuv0+5, yuv1, width, width*2, height/2, &g);
	    g.y += 0.5;
	    calcshift(&g, &shift);
	    doshift(yuv1, yuv2, height/2, width, width*2, &shift);
	    /* Top half -> Bottom half */
	    gmotion(yuv1, yuv1+5, width, width*2, height/2, &g);
	    g.y -= 0.5;
	    calcshift(&g, &shift);
	    doshift(yuv1+5, yuv2+5, height/2, width, width*2, &shift);
	    y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv2);
	    break;
	  case Y4M_ILACE_BOTTOM_FIRST | 0200:
	    /* Last top half -> Bottom half */
	    gmotion(yuv0, yuv1+5, width, width*2, height/2, &g);
	    g.y -= 0.5;
	    calcshift(&g, &shift);
	    doshift(yuv1+5, yuv2+5, height/2, width, width*2, &shift);
	    /* Bottom half -> Top half */
	    gmotion(yuv1+5, yuv1, width, width*2, height/2, &g);
	    g.y += 0.5;
	    calcshift(&g, &shift);
	    doshift(yuv1, yuv2, height/2, width, width*2, &shift);
	    y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv2);
	    break;
	    }
	/* swap yuv0 and yuv1, so yuv1 becomes the old reference frame
	 * for the motion search. */
	for (i = 0; i < 10; i++)
	    { line1 = yuv0[i]; yuv0[i] = yuv1[i]; yuv1[i] = line1; }
	}
    /* All done - close out the streams and exit */
endit:
    y4m_fini_frame_info(&iframe);
    y4m_fini_stream_info(&istream);
    y4m_fini_stream_info(&ostream);
    exit(0);
    }

static void
usage (void)
        {
	fputs(
"Program to stabilize a video stream.\n"
"Works only non-interlaced yuv4mpeg streams for now.\n"
"\n"
"Usage: y4mstabilizer [-v] [-a <alpha>] [-r <srchRadius>]\n"
"\n"
"	-v		Verbose.  Repeat -v to increase verbosity/debug info\n"
"	-a <alpha>	A \"viscosity\" measure (see below).\n"
"	-r <srchRadius>	How far to look for movement.\n"
"	-s <stride>	How far apart the motion search points are.\n"
"	-n		Do not supersample the chroma to get 1-pixel shifts.\n"
"	-i		Try alternate interlaced mode (does not work yet)\n"
"\n"
"Legal <alpha> values are beween 0 and 1.\n"
"Useful <alpha> values are beween 0.7 (or so) and .95.\n"
"Higher values resist panning more at the expense of greater\n"
"shifting.  <alpha> defaults to 0.95, a fairly high value, on the theory\n"
"that you probably have some significant vibrations that need smoothing.\n"
"Separate alpha values for the X- and Y-axes can be specified by separating\n"
"them with a colon.  For example,\n"
"	-a 0.7:0.9\n"
"would set the X-axis alpha to 0.7, and the Y-axis alpha to 0.9.  Thus,\n"
"the stabilizer would be much more responsive to side-to-side movement,\n"
"while resisting up-and-down motion.\n"
"\n"
"The <srchRadius> defaults to 15.  Smaller values speed things up,\n"
"but won't be able to cope with large/fast movements as well.\n"
"\n"
"The <stride> defaults to 48 pixels.  Giving a larger number here will\n"
"speed the process tremendously, but makes it easier for the motion search\n"
"to be fooled by local movement.  Use a smaller number if the search seems\n"
"to be following local movements.\n"
"\n"
"No file arguments are needed since this is a filter only program.\n"
"\n"
"This program presently works best when given 444, deinterlaced input.\n"
"Very good results can be obtained with the following pipeline:\n"
"    ... | yuvdeinterlace | \\\n"
"	   y4mscaler -v 0 -O sar=src -O chromass=444 | \\\n"
"	   y4mstabilizer | \\\n"
"	   y4mscaler -v 0 -O sar=src -O chromass=420_MPEG2 | ...\n"
, stderr);
exit(1);
}

static void
alloc_yuv (u_char **yuv, int h, int w)
{
int len = h * w * 2;	/* Double the amount - overkill but it's easier than figuring out how much (off by one?) is really needed */
int uvlen = Stab.nosuper ? (len / (SS_H * SS_V)) : len;
yuv[0] = malloc(len);
if (yuv[0] == NULL)
mjpeg_error_exit1(" malloc(%d) failed\n", len);
yuv[1] = malloc(uvlen);
if (yuv[1] == NULL)
mjpeg_error_exit1(" malloc(%d) failed\n", uvlen);
yuv[2] = malloc(uvlen);
if (yuv[2] == NULL)
mjpeg_error_exit1(" malloc(%d) failed\n", uvlen);
yuv[3] = malloc(len/4);
if (yuv[3] == NULL)
mjpeg_error_exit1(" malloc(%d) failed\n", len/4);
yuv[4] = malloc(len/16);
if (yuv[4] == NULL)
mjpeg_error_exit1(" malloc(%d) failed\n", len/16);
yuv[5] = yuv[0] + w;
yuv[6] = yuv[1] + w/SS_H;
yuv[7] = yuv[2] + w/SS_H;
yuv[8] = yuv[3] + w/2;
yuv[9] = yuv[3] + w/4;
}

/*****************************************************************************
* generate a lowpassfiltered and subsampled copy                            *
* of the source image (src) at the destination                              *
* image location.                                                           *
* Lowpass-filtering is important, as this subsampled                        *
* image is used for motion estimation and the result                        *
* is more reliable when filtered.                                           *
* only subsample actual data, but keeping full buffer size for simplicity   *
*****************************************************************************/
static void
subsample (uint8_t *src, uint8_t *dst, int w, int h)
{
int c, x, w2 = w / 2;
uint8_t *s1 = src;
uint8_t *s2 = src + w;
for (h /= 2; h >= 0; h--)
{
for (x = 0; x < w2; x++)
{
c = *s1++ + *s2++;
c += *s1++ + *s2++;
*dst++ = c >> 2;
}
s1 += w;
s2 += w;
}
}

/*
* Determine global motion.
* Note that only the Y-plane is used.
* The global motion is taken as the median of the individual motions.
*
* Note that w (frame width) should equal ws (frame width stride) unless
* we are treating an interlaced frame as two subframes, in which case
* ws should be twice w.
*/
static void
gmotion (u_char **y0, u_char **y1, int w, int ws, int h, vec *dij)
{
int i, j, di, dj;
int we = w - (Stab.rad+8);
int he = h - (Stab.rad+8);
int xs[Stab.diam+Stab.diam], ys[Stab.diam+Stab.diam], t = 0;
vec ij;
bzero(xs, sizeof xs);
bzero(ys, sizeof ys);
/* Determine local motions for all blocks */
for (i = Stab.rad; i < we; i += Stab.stride)
for (j = Stab.rad; j < he; j += Stab.stride)
{
ij.x = i/4; ij.y = j/4;
motion(y0[4], y1[4], ws/4, Stab.rad/4, i/4,  j/4,  &ij);
ij.x += ij.x; ij.y += ij.y;
motion(y0[3], y1[3], ws/2, 3,          i/2, j/2, &ij);
ij.x += ij.x; ij.y += ij.y;
motion(y0[0], y1[0], ws,   3,          i,   j,   &ij);
motion0(y0[0],y1[0], ws,               i,   j,   &ij);
di = ij.x - (i+i); dj = ij.y - (j+j);
/*if ((abs(di) <= Stab.rad) && (abs(dj) <= Stab.rad))*/
{
xs[di+Stab.diam]++;
ys[dj+Stab.diam]++;
t++;
}
}
/* Determine median motions */
t /= 2;
for (di = i = 0; di < Stab.diam+Stab.diam; i += xs[di++])
if (i >= t)
break;
dij->x = di - Stab.diam;
for (dj = j = 0; dj < Stab.diam+Stab.diam; j += ys[dj++])
if (j >= t)
break;
dij->y = dj - Stab.diam;
}

/*********************************************************************
*                                                                   *
* Estimate Motion Vectors                                           *
*                                                                   *
*********************************************************************/
static void 
motion (u_char *y0, u_char *y1, int w, int r, int ri, int rj, vec *dij)
{
uint32_t best_SAD=INT_MAX, SAD=INT_MAX; 
int i = dij->x, j = dij->y;
int ii, jj;
y0 += (rj * w) + ri;
for (ii = -r; ii <= r; ii++)
for (jj = -r; jj <= r; jj++)
{
SAD = calc_SAD_noaccel(y0, y1 + (j+jj) * w + i+ii, w, best_SAD);
SAD += ii*ii + jj*jj; /* favour center matches... */
if (SAD <= best_SAD)
{
best_SAD = SAD;
dij->x = i + ii;
dij->y = j + jj;
}
}
}

/*********************************************************************
*                                                                   *
* Estimate Motion Vectors in not subsampled frames                  *
*                                                                   *
*********************************************************************/
static void 
motion0 (u_char *y0, u_char *y1, int w, int ri, int rj, vec *dij)
{
uint32_t SAD, best_SAD = INT_MAX;
int adjw;
int i = dij->x, j = dij->y;
int ii, jj;
u_char *y1r = y1 + j*w + i;

y0 += rj*w + ri;
y1 += (j-1)*w + i - 1;
adjw = w - 3;
for (jj = -1; jj <= 1; jj++, y1 += adjw)
for (ii = -1; ii <= 1; ii++, y1++)
{
SAD = calc_SAD_half_noaccel(y0, y1r, y1, w, best_SAD);
if (SAD < best_SAD)
{
best_SAD = SAD;
dij->x = ii+i+i-1;
dij->y = jj+j+j-1;
}
}
}

/*********************************************************************
*                                                                   *
* SAD-function for Y without MMX/MMXE                               *
*                                                                   *
*********************************************************************/
static uint32_t
calc_SAD_noaccel (uint8_t *frm, uint8_t *ref, int w, int limit)
{
uint32_t d = 0;
uint32_t adj = w - 8;
#define LINE \
d += abs(*frm++ - *ref++); d += abs(*frm++ - *ref++); \
d += abs(*frm++ - *ref++); d += abs(*frm++ - *ref++); \
d += abs(*frm++ - *ref++); d += abs(*frm++ - *ref++); \
d += abs(*frm++ - *ref++); d += abs(*frm++ - *ref++); \
if (d > limit) \
return INT_MAX; \
frm += adj; ref += adj
LINE; LINE; LINE; LINE;
LINE; LINE; LINE; LINE;
#undef LINE
return d;
}

/*********************************************************************
*                                                                   *
* halfpel SAD-function for Y without MMX/MMXE                       *
*                                                                   *
*********************************************************************/
static uint32_t
calc_SAD_half_noaccel(uint8_t*ref, uint8_t*frm1, uint8_t*frm2, int w, int limit)
{
uint32_t d = 0;
uint32_t adj = w - 8;
#define LINE \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
d += abs(((*frm1++ + *frm2++) >> 1) - *ref++); \
if (d > limit) \
return INT_MAX; \
frm1 += adj; frm2 += adj; ref += adj
LINE; LINE; LINE; LINE;
LINE; LINE; LINE; LINE;
#undef LINE
return d;
}

static void
calcshift (vec *gp, vec *shftp)
{
int ss_h = Stab.nosuper ? SS_H : 1;
int ss_v = Stab.nosuper ? SS_V : 1;
/* gmotion() returns motion in half-pixels */
/* Factor in <alpha> the "viscosity"... */
Stab.gsX = (Stab.gsX * Stab.alphax) + gp->x/2.0;
Stab.gsY = (Stab.gsY * Stab.alphay) + gp->y/2.0;
/* Now that we know the movement, shift to counteract it */
shftp->x = -xround(Stab.gsX, ss_h);
shftp->y = -xround(Stab.gsY, ss_v);
if (Stab.verbose > 1)
mjpeg_info("global motion xy*2=<%d,%d>"
" Accumulated xy=<%g,%g> shift xy=%d,%d>\n",
gp->x, gp->y, Stab.gsX, Stab.gsY, shftp->x, shftp->y);
}

/* Round the given float value to the nearest multiple of <r>. */
static int
xround (float v, int r)
{
if (v < 0)
return (-xround(-v, r));
return (((int)((v + r/2.0) / r)) * r);
}

/*
* Shift a frame.
* The frame is always copied to the destination.
* 
* Note that w (frame width) should equal ws (frame width stride) unless
* we are treating an interlaced frame as two subframes, in which case
* ws should be twice w.
*/
static void
doshift (u_char**yuv1, u_char**yuv2, int h, int w, int ws, vec *shift)
{
int dosuper = (shift->x % SS_H) | (shift->y % SS_V);
int ss_h = dosuper ? 1 : SS_H;
int ss_v = dosuper ? 1 : SS_V;
/* If we have to supersample the chroma, then do it now, before shifting */
if (dosuper)
chroma_supersample(Y4M_CHROMA_420JPEG, yuv1, w, h);
/* Do the horizontal shifting first.  The frame is shifted into
* the yuv2 frame. Even if there is no horizontal shifting to do,
* we copy the frame because the vertical shift is desctructive,
* and we want to preserve the yuv1 frame to compare against the
* next one. */
hshift(yuv1[0],yuv2[0],h,     w,     shift->x,      16, ws);
hshift(yuv1[1],yuv2[1],h/ss_v,w/ss_h,shift->x/ss_h,128, ws/ss_h);
hshift(yuv1[2],yuv2[2],h/ss_v,w/ss_h,shift->x/ss_h,128, ws/ss_h);
/* Vertical shift, then write the frame */
if (shift->y)
{
vertical_shift(yuv2[0], shift->y,      w,      ws,      h,      16);
vertical_shift(yuv2[1], shift->y/ss_v, w/ss_h, ws/ss_h, h/ss_v, 128);
vertical_shift(yuv2[2], shift->y/ss_v, w/ss_h, ws/ss_h, h/ss_v, 128);
}
/* Undo the supersampling */
if (dosuper)
chroma_subsample(Y4M_CHROMA_420JPEG, yuv1, w, h);
}

/*
* Shift one plane of the frame by the given horizontal shift amount.
* The shifted frame is left in the <vdst> buffer.
*/
static void
hshift (u_char *vsrc, u_char *vdst, int h, int w, int shift, int blk,int ws)
{
int i;
for (i = 0; i < h; i++)
{
if (shift > 0)
{
memmove(vdst + shift, vsrc, w - shift);
memset(vdst, blk, shift); /* black */
}
else if (shift < 0)
{
memmove(vdst, vsrc - shift, w + shift);
memset(vdst+w+shift, blk, -shift);
}
else
memmove(vdst, vsrc, w);
vsrc += ws;
vdst += ws;
}
}

/*
* Shift the frame vertically.  The frame data is modified in place.
*/
static void
vertical_shift(u_char *y, int vshift, int w, int ws, int h, int blk)
{
/* Easy case - we can shift everything at once */
if (w == ws)
{
if (vshift > 0)
{
memmove(y + vshift*ws, y, (h-vshift) * ws);
memset(y, blk, vshift * ws);
}
else
{
memmove(y, y - vshift * ws, (h+vshift) * ws);
memset(y + (h + vshift) * ws, blk, -vshift * w);
}
}
/* Must go line-by-line */
else
{
u_char *dst, *src;
int i, n;
if (vshift > 0)
{
dst = y + (h - 1) * ws;
src = dst - vshift * ws;
n = h - vshift;
for (i = 0; i < n; i++, (dst -= ws), (src -= ws))
memcpy(dst, src, w);
for ( ; i < h; i++, dst -= ws)
memset(dst, blk, w);
}
else
{
dst = y;
src = y - vshift * ws;
n = h + vshift;
for (i = 0; i < n; i++, (dst += ws), (src += ws))
memcpy(dst, src, w);
for ( ; i < h; i++, dst += ws)
memset(dst, blk, w);
}
}
}
