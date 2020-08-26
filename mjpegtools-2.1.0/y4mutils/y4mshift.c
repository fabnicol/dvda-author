
/*
 * $Id: y4mshift.c,v 1.4 2009/09/19 18:57:00 sms00 Exp $
 *
 * written by Steven M. Schultz <sms@2BSD.COM>
 *
 * 2003/5/8 - added the ability to place a black border around the frame.
 *
 * Simple program to shift the data an even number of pixels.   The shift count
 * is positive for shifting to the right and negative for a left shift.   
 *
 * Usage: y4mshift -n N [ -b xoffset,yoffset,xsize,ysize ]
 *
 * No file arguments are needed since this is a filter only program.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yuv4mpeg.h"

#define HALFSHIFT (shiftnum / SS_H)

	void	black_border(u_char **, char *, int, int, int, int);
	void	vertical_shift(u_char **, int, int, int, int, int, int);
static  void    usage(char *);

int main(int argc, char **argv)
        {
        int     i, c, width, height, frames, err, chroma_ss, ilace_factor;
        int     shiftnum = 0, shiftY = 0, rightshiftY, rightshiftUV;
        int     vshift = 0, vshiftY = 0, monochrome = 0;
        int     verbose = 0, fdin;
        int     SS_H = 2, SS_V = 2;
        u_char  *yuv[3], *line;
	char	*borderarg = NULL;
        y4m_stream_info_t istream, ostream;
        y4m_frame_info_t iframe;

        fdin = fileno(stdin);

	y4m_accept_extensions(1);

        opterr = 0;
        while   ((c = getopt(argc, argv, "hvn:N:b:My:Y:")) != EOF)
                {
                switch  (c)
                        {
			case	'M':
				monochrome = 1;
				break;
			case	'b':
				borderarg = strdup(optarg);
				break;
                        case    'n':
                                shiftnum = atoi(optarg);
                                break;
                        case    'y':
                                shiftY = atoi(optarg);
                                break;
                        case    'Y':
                                vshiftY = atoi(optarg);
                                break;
			case	'N':
				vshift = atoi(optarg);
				break;
                        case    'v':
                                verbose++;
                                break;
                        case    '?':
                        case    'h':
                        default:
                                usage(argv[0]);
                        }
                }

	if      ( shiftnum + shiftY > 0 )
		rightshiftY = 1;
	else
		rightshiftY = 0;
	shiftY = abs(shiftnum + shiftY);  /* now shiftY is total Y shift */

        if      (shiftnum > 0)
                rightshiftUV = 1;
        else
                {
                rightshiftUV = 0;
                shiftnum = abs(shiftnum);
                }

        y4m_init_stream_info(&istream);
        y4m_init_frame_info(&iframe);

        err = y4m_read_stream_header(fdin, &istream);
        if      (err != Y4M_OK)
                mjpeg_error_exit1("Input stream error: %s\n", y4m_strerr(err));

	if	(y4m_si_get_plane_count(&istream) != 3)
		mjpeg_error_exit1("Only 3 plane formats supported");

	chroma_ss = y4m_si_get_chroma(&istream);
	SS_H = y4m_chroma_ss_x_ratio(chroma_ss).d;
	SS_V = y4m_chroma_ss_y_ratio(chroma_ss).d;

	if	(y4m_si_get_interlace(&istream) == Y4M_ILACE_NONE)
		ilace_factor = 1;
	else
		ilace_factor = 2;

        if      ((shiftnum % SS_H) != 0)
                usage(argv[0]);

        if      ((vshift % (ilace_factor * SS_V)) != 0)
                usage(argv[0]);

        width = y4m_si_get_width(&istream);
        height = y4m_si_get_height(&istream);

        if      (shiftnum > width / 2)
                mjpeg_error_exit1("nonsense to shift %d out of %d",
                        shiftnum, width);
        if      (shiftY > width / 2)
                mjpeg_error_exit1("nonsense to shift %d out of %d\n",
                        shiftY, width);

        y4m_init_stream_info(&ostream);
        y4m_copy_stream_info(&ostream, &istream);
        y4m_write_stream_header(fileno(stdout), &ostream);

        yuv[0] = malloc(y4m_si_get_plane_length(&istream, 0));
        if      (yuv[0] == NULL)
                mjpeg_error_exit1("malloc() failed for plane 0");
        yuv[1] = malloc(y4m_si_get_plane_length(&istream, 1));
        if      (yuv[1] == NULL)
                mjpeg_error_exit1("malloc() failed for plane 1");
        yuv[2] = malloc(y4m_si_get_plane_length(&istream, 2));
        if      (yuv[2] == NULL)
                mjpeg_error_exit1("malloc() failed for plane 2");

        frames = 0;
        for     (;y4m_read_frame(fdin,&istream,&iframe,yuv) == Y4M_OK; frames++)
                {
                if      (shiftnum == 0 && shiftY == 0)
                        goto outputframe;
                for     (i = 0; i < height; i++)
                        {
/*
 * Y
*/
                        line = &yuv[0][i * width];
                        if      (rightshiftY)
                                {
                                bcopy(line, line + shiftY, width - shiftY);
                                memset(line, 16, shiftY); /* black */
                                }
                        else 
                                {
                                bcopy(line + shiftY, line, width - shiftY);
                                memset(line + width - shiftY, 16, shiftY);
                                }
                        }
/*
 * U
*/
                for     (i = 0; i < height / SS_V; i++)
                        {
                        line = &yuv[1][i * (width / SS_H)];
                        if      (rightshiftUV)
                                {
                                bcopy(line, line+HALFSHIFT, (width-shiftnum)/SS_H);
                                memset(line, 128, HALFSHIFT); /* black */
                                }
                        else
                                {
                                bcopy(line+HALFSHIFT, line, (width-shiftnum)/SS_H);
                                memset(line+(width-shiftnum)/SS_H, 128, HALFSHIFT);
                                }
                        }
/*
 * V
*/
                for     (i = 0; i < height / SS_V; i++)
                        {
                        line = &yuv[2][i  * (width / SS_H)];
                        if      (rightshiftUV)
                                {
                                bcopy(line, line+HALFSHIFT, (width-shiftnum)/SS_H);
                                memset(line, 128, HALFSHIFT); /* black */
                                }
                        else
                                {
                                bcopy(line+HALFSHIFT, line, (width-shiftnum)/SS_H);
                                memset(line+(width-shiftnum)/SS_H, 128, HALFSHIFT);
                                }
                        }
outputframe:
		if	(vshift)
			vertical_shift(yuv, vshift, vshiftY, width, height, SS_H, SS_V);
		if	(borderarg)
			black_border(yuv, borderarg, width, height, SS_H, SS_V);
		if	(monochrome)
			{
			memset(&yuv[1][0], 128, (width / SS_H) * (height / SS_V));
			memset(&yuv[2][0], 128, (width / SS_H) * (height / SS_V));
			}
                y4m_write_frame(fileno(stdout), &ostream, &iframe, yuv);
                }
        y4m_fini_frame_info(&iframe);
        y4m_fini_stream_info(&istream);
        y4m_fini_stream_info(&ostream);

        exit(0);
        }

/*
 * -b Xoff,Yoff,Xsize,YSize
*/

void black_border (u_char *yuv[], char *borderstring, int W, int H, int SS_H, int SS_V)
	{
	static	int parsed = -1;
	static	int BX0, BX1;	/* Left, Right border columns */
	static	int BY0, BY1;	/* Top, Bottom border rows */
	int	i1, i2, i, dy, W2, H2;
  
	if	(parsed == -1)
		{
		parsed = 0;
		i = sscanf(borderstring, "%d,%d,%d,%d", &BX0, &BY0, &i1, &i2);
		if	(i != 4 || (BX0 % SS_H) || (BY0 % (2*SS_V)) || i1 < 0 || i2 < 0 ||
			 (BX0 + i1 > W) || (BY0 + i2 > H))
			{
			mjpeg_warn(" border args invalid - ignored");
			return;
			}
		BX1 = BX0 + i1;
		BY1 = BY0 + i2;
		parsed = 1;
		}
/*
 * If a borderstring was seen but declared invalid then it
 * is being ignored so just return.
*/
	if	(parsed == 0)
		return;

	W2 = W / SS_H;
	H2 = H / SS_V;

/*
 * Yoff Lines at the top.   If the vertical offset is 0 then no top border
 * is being requested and there is nothing to do.
*/
	if	(BY0 != 0)
		{
		memset(yuv[0], 16,  W  * BY0);
		memset(yuv[1], 128, W2 * BY0/SS_V);
		memset(yuv[2], 128, W2 * BY0/SS_V);
		}
/*
 * Height - (Ysize + Yoff) lines at bottom.   If the bottom coincides with
 * the frame size then there is nothing to do.
*/
	if	(H != BY1)
		{
		memset(&yuv[0][BY1 * W], 16, W  * (H - BY1));
		memset(&yuv[1][(BY1 / SS_V) * W2], 128, W2 * (H - BY1)/SS_V);
		memset(&yuv[2][(BY1 / SS_V) * W2], 128, W2 * (H - BY1)/SS_V);
		}
/*
 * Now the partial lines in the middle.   Go from rows BY0 thru BY1 because
 * the whole rows (0 thru BY0) and (BY1 thru H) have been done above.
*/
	for	(dy = BY0; dy < BY1; dy++)
		{
/*
 * First the columns on the left (x = 0 thru BX0).   If the X offset is 0
 * then there is nothing to do.
*/
		if	(BX0 != 0)
			{
			memset(&yuv[0][dy * W], 16, BX0);
			memset(&yuv[1][dy / SS_V * W2], 128, BX0 / SS_H);
			memset(&yuv[2][dy / SS_V * W2], 128, BX0 / SS_H);
			}
/*
 * Then the columns on the right (x = BX1 thru W).   If the right border
 * start coincides with the frame size then there is nothing to do.
*/
		if	(W != BX1)
			{
			memset(&yuv[0][(dy * W) + BX1], 16, W - BX1);
			memset(&yuv[1][(dy/SS_V * W2) + BX1/SS_H], 128, (W - BX1)/SS_H);
			memset(&yuv[2][(dy/SS_V * W2) + BX1/SS_H], 128, (W - BX1)/SS_H);
			}
		}

	}

void vertical_shift(u_char **yuv, int vshift, int vshiftY, int width, int height, int SS_H, int SS_V)
	{
	int	downshiftY, downshiftUV, w2 = width / SS_H, v2;

	if      ( vshift + vshiftY > 0 )
		downshiftY = 1;
	else
		downshiftY = 0;
	vshiftY = abs(vshift + vshiftY);  /* now shiftY is total Y shift */

	if	(vshift > 0)
		downshiftUV = 1;
	else
		{
		downshiftUV = 0;
		vshift = abs(vshift);
		}
	v2 = vshift / SS_V;

	if	(downshiftY)
		{
		memmove(&yuv[0][vshiftY * width], &yuv[0][0],
			(height - vshiftY) * width);
		memset(&yuv[0][0], 16, vshiftY * width);
		}
	else
		{
		memmove(&yuv[0][0], &yuv[0][vshiftY * width],
			(height - vshiftY) * width);
		memset(&yuv[0][(height - vshiftY) * width], 16, vshiftY * width);
		}

	if      (downshiftUV)
	        {
		memmove( &yuv[1][v2 * w2], &yuv[1][0],
			(height - vshift) / SS_V * w2);
		memmove( &yuv[2][v2 * w2], &yuv[2][0],
			(height - vshift) / SS_V * w2);

		memset(&yuv[1][0], 128, v2 * w2);
		memset(&yuv[2][0], 128, v2 * w2);
		}
	else
		{
		memmove(&yuv[1][0], &yuv[1][v2 * w2],
			(height - vshift) / SS_V * w2);
		memmove(&yuv[2][0], &yuv[2][v2 * w2],
			(height - vshift) / SS_V * w2);

		memset(&yuv[1][((height - vshift) / SS_V) * w2], 128, v2 * w2);
		memset(&yuv[2][((height - vshift) / SS_V) * w2], 128, v2 * w2);
		}
	}

static void usage(char *pgm)
        {

        fprintf(stderr, "%s: usage: [-v] [-h] [-M] [-b xoff,yoff,xsize,ysize] [-y num] [-Y num] [-N num] -n N\n", pgm);
	fprintf(stderr, "%s:\t-M = monochrome output\n", pgm);
        fprintf(stderr, "%s:\t-n N = horizontal shift count - must be multiple of 2 for 4:2:0, multiple of 4 for 4:1:1!\n", pgm);
        fprintf(stderr, "%s:\t-y num = Y-only horizontal shift count - need not be even\n", pgm);
        fprintf(stderr, "%s:\t\tpositive count shifts right\n",pgm);
        fprintf(stderr, "%s:\t\t0 passes the data thru unchanged\n",pgm);
        fprintf(stderr, "%s:\t\tnegative count shifts left\n", pgm);
	fprintf(stderr, "%s:\t-N num = vertical shift count - must be multiple of 4 for 4:2:0, multiple of 2 for 4:1:1!\n", pgm);
	fprintf(stderr, "%s:\t-Y num = Y-only vertical shift count - multiple of 2 for interlaced material\n", pgm);
	fprintf(stderr, "%s:\t\tnegative count shifts up\n", pgm);
	fprintf(stderr, "%s:\t\t0 does no vertical shift (is ignored)\n", pgm);
	fprintf(stderr, "%s:\t\tpositive count shifts down\n", pgm);
	fprintf(stderr, "%s:\t-b creates black border\n", pgm);
	fprintf(stderr, "%s:\tShifting is done before border creation\n", pgm);
        fprintf(stderr, "%s:\t-v print input stream info\n", pgm);
        fprintf(stderr, "%s:\t-h print this usage summary\n", pgm);
        exit(1);
        }
