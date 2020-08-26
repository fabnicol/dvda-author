/*
 * $Id: y4minterlace.c,v 1.4 2007/04/01 18:06:06 sms00 Exp $
 *
 * A simple program to generate interlaced output from progressive input by
 * using one field from each of two frames and reducing the rate by a factor
 * of two.   
 *
 * Why would one want to do this?   In the US one of the HDTV broadcast formats
 * is 1280x720p at the NTSC field rate of 60000/1001 frames/sec.  It was desired
 * to recode a captured file to a format suitable for placement on a DVD.  Of
 * course 1280x720 is too large and more importantly 59.94 frames/sec is far 
 * too high to put on a DVD (DVDs use a subset of MP@ML, not MP@HL  MPEG-2).
 *
 * The decoding (using mpeg2dec) and rescaling (using y4mscaler and taking into
 * account that HD uses 1:1 pixels of course) of the data was easily done but 
 * what to do about the frame rate?   Not only is 60000/1001 too high for a 
 * DVD but even if the target was not DVD the higher frame rates (-F 6 and 
 * above) belong to a higher MPEG-2 profile/level than mpeg2enc implements.
 * Ah ha, create interlaced frames comprised of lines from each of two 
 * progressive frames and then change the rate from 60000/1001 to 30000/1001.
 *
 * Options are provided to force the frame rate (-r) and to specify which
 * field dominance is desired (-i).  These options should rarely, if ever,
 * be needed.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yuv4mpeg.h"
#include "mjpeg_logging.h"

static void usage(char *);

int main(int argc, char **argv)
	{
	int	chroma_id, fdin, fdout, c, verbose = 1, err, uvlen, ylen;
	char	ilace = Y4M_ILACE_TOP_FIRST;
	y4m_stream_info_t ostream, istream;
	y4m_frame_info_t iframe, oframe;
	y4m_ratio_t rate_ratio = y4m_fps_UNKNOWN;
	u_char *f1_top[3], *f1_bot[3], *f2_top[3], *f2_bot[3];
	u_char **top_field, **bot_field;

	fdin = fileno(stdin);
	fdout = fileno(stdout);

	y4m_accept_extensions(1);

	mjpeg_default_handler_verbosity(verbose);

	opterr = 0;
	while	((c = getopt(argc, argv, "hr:i:v:")) != EOF)
		{
		switch	(c)
			{
			case	'i':
				if	(strcmp("b", optarg) == 0)
					ilace = Y4M_ILACE_BOTTOM_FIRST;
				else if	(strcmp("t", optarg) == 0)
					ilace = Y4M_ILACE_TOP_FIRST;
				else
					mjpeg_error_exit1("-i arg must be t or b");
				break;
			case	'v':
				verbose = atoi(optarg);
				if	(verbose < 0 || verbose >= 3)
					mjpeg_error_exit1("Verbosity [0..2]");
				break;
			case	'r':
				err = y4m_parse_ratio(&rate_ratio, optarg);
				if	(err != Y4M_OK)
					mjpeg_error_exit1("invalid/unparseable -r arg");
				break;
			case	'?':
			case	'h':
			default:
				usage(argv[0]);
			}
		}

	if	(isatty(fdout))
		mjpeg_error_exit1("stdout must not be a terminal");

	mjpeg_default_handler_verbosity(verbose);

	y4m_init_stream_info(&istream);
	y4m_init_frame_info(&iframe);
	y4m_init_frame_info(&oframe);

	err = y4m_read_stream_header(fdin, &istream);
	if	(err != Y4M_OK)
                mjpeg_error_exit1("Input stream error: %s\n", y4m_strerr(err));

	if	(y4m_si_get_interlace(&istream) != Y4M_ILACE_NONE)
		mjpeg_error_exit1("Input stream is NOT progressive!");

	if	(y4m_si_get_plane_count(&istream) != 3)
		mjpeg_error_exit1("Only the 3 plane formats supported.");

	chroma_id = y4m_si_get_chroma(&istream);

	if	(y4m_chroma_ss_y_ratio(chroma_id).d != 1)
		mjpeg_warn("Vertically subsampled chroma (%s) should be upsampled but will proceed.", y4m_chroma_keyword(chroma_id));

/*
 * NOTE:  each buffer to be allocated will only hold 1 field or 1/2 a frame.
 *        thus the size is divided by 2 below
*/
	ylen  = y4m_si_get_plane_length(&istream, 0);	/* luma size */
	uvlen = y4m_si_get_plane_length(&istream, 1);	/* chroma plane size */

/* First frame's two field buffers */
	f1_top[0] = malloc(ylen / 2);
	f1_top[1] = malloc(uvlen / 2);
	f1_top[2] = malloc(uvlen / 2);
	f1_bot[0] = malloc(ylen / 2);
	f1_bot[1] = malloc(uvlen / 2);
	f1_bot[2] = malloc(uvlen / 2);

/* Second frame's two field buffers */
	f2_top[0] = malloc(ylen / 2);
	f2_top[1] = malloc(uvlen / 2);
	f2_top[2] = malloc(uvlen / 2);
	f2_bot[0] = malloc(ylen / 2);
	f2_bot[1] = malloc(uvlen / 2);
	f2_bot[2] = malloc(uvlen / 2);

	y4m_init_stream_info(&ostream);
	y4m_copy_stream_info(&ostream, &istream);
	if	(Y4M_RATIO_EQL(rate_ratio, y4m_fps_UNKNOWN))
		{
		rate_ratio = y4m_si_get_framerate(&istream);
		rate_ratio.d *= 2;
		y4m_ratio_reduce(&rate_ratio);
		}
	y4m_si_set_interlace(&ostream, ilace);
	y4m_si_set_framerate(&ostream, rate_ratio);

	y4m_log_stream_info(mjpeg_loglev_t("info"), "ostream", &ostream);

	y4m_write_stream_header(fdout, &ostream);
	while	((err = y4m_read_fields(fdin, &istream, &iframe, f1_top, f1_bot)) == Y4M_OK)
		{
		y4m_read_fields(fdin, &istream, &iframe, f2_top, f2_bot);
		if	(ilace == Y4M_ILACE_TOP_FIRST)
			{
			top_field = f1_top;
			bot_field = f2_bot;
			}
		else
			{
			top_field = f2_top;
			bot_field = f1_bot;
			}
		y4m_write_fields(fdout,&ostream, &oframe, top_field, bot_field);
		}
	y4m_fini_frame_info(&iframe);
	y4m_fini_frame_info(&oframe);
	y4m_fini_stream_info(&istream);
	y4m_fini_stream_info(&ostream);
	exit(0);
	}

static void usage(char *progname)
	{

	fprintf(stderr, "%s usage: [-v n] [-i t|b] [-r rate]\n", progname);
	fprintf(stderr, "%s\t-i defaults to (t) top field first\n", progname);
	fprintf(stderr, "%s\t-r overrides using (input rate)/2\n",progname);
	exit(0);
	}
