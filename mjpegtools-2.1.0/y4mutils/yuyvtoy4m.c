/*
 * $Id: yuyvtoy4m.c,v 1.3 2009/09/19 18:57:00 sms00 Exp $
 *
 * A simple progam that can be used to convert 'yuyv' (also known as yuv2)
 * 4:2:2 packed into 4:2:2 planar format.  Additional packings may be added
 * in the future BUT some of those are 'full range' rather than 'video range'!
 * Apple calls this 'yuvs' since the bytes within each 16bit word are swapped.
 *
 * The other moderately common 4:2:2 packing is UYVY - a simple byte flip within
 * each 16bit word.  Since there's no way for the program to autodetect which
 * packing is used the '-k' option is provided for the user.  If wild/crazy colors
 * are seen the chances are good that -k is needed ;)
 *
 * Used to convert YUY2/YUYV/YUVS to YUV4MPEG2 which is the format used by
 * mpeg2enc from the mjpeg-tools suite.
 *
 * This is strictly a filter, the use is in a pipeline such as the following
 * which catenates the raw 4:2:2 packed (yuyv) images from a ieee1394 digital 
 * camera into a stream, repacks to 4:2:2 planar, and converts to 4:2:0 before
 * encoding.  Obviously other steps, such as altering the frame rate, etc will
 * be done.  
 *
 * The default sample aspect ratio for is square (1:1) because the anticipated use
 * of this program is with stills from IIDC cameras.
 *
 * cat *.raw | yuyvtoy4m -i p -w 1280 -h 960 -a 1:1 -r 24:1 | \
 *       y4mscaler -O chromass=420_mpeg2 | mpeg2enc ...
 *
 * NOTE: Obviously when catenating multiple files together they ALL must have 
 *       the same attributes (dimensions,  interlacing, etc)!
*/

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "yuv4mpeg.h"

static	void	usage(char *);

int
main(int argc, char **argv)
	{
	int	sts, c, width = 0, height = 0, frame_len, i, yuyv = 1;
	y4m_ratio_t	rate_ratio = y4m_fps_FILM;
	y4m_ratio_t	aspect_ratio = y4m_sar_SQUARE;
	int		interlace = Y4M_ILACE_NONE;
	u_char	*yuv[3], *input_frame, *Y_p, *Cr_p, *Cb_p, *p;
	y4m_stream_info_t ostream;
	y4m_frame_info_t oframe;

	opterr = 0;
	if	(argc == 1)
		usage(argv[0]);

	while	((c = getopt(argc, argv, "w:h:r:i:a:k")) != EOF)
		{
		switch	(c)
			{
			case	'k':
				yuyv = 0;
				break;
			case	'a':
				sts = y4m_parse_ratio(&aspect_ratio, optarg);
				if	(sts != Y4M_OK)
					mjpeg_error_exit1("Invalid aspect: %s",
						optarg);
				break;
			case	'w':
				width = atoi(optarg);
				break;
			case	'h':
				height = atoi(optarg);
				break;
			case	'r':
				sts = y4m_parse_ratio(&rate_ratio, optarg);
				if	(sts != Y4M_OK)
					mjpeg_error_exit1("Invalid rate: %s", optarg);
				break;
			case	'i':
				switch	(optarg[0])
					{
					case	'p':
						interlace = Y4M_ILACE_NONE;
						break;
					case	't':
						interlace = Y4M_ILACE_TOP_FIRST;
						break;
					case	'b':
						interlace = Y4M_ILACE_BOTTOM_FIRST;
						break;
					default:
						usage(argv[0]);
					}
				break;
			case	'?':
			default:
				usage(argv[0]);
			}
		}

	if	(width <= 0)
		mjpeg_error_exit1("Invalid or unspecified Width: %d", width);
	if	(height <= 0)
		mjpeg_error_exit1("Invalid or unspecified Height: %d", height);

	y4m_accept_extensions(1);

	y4m_init_stream_info(&ostream);
	y4m_init_frame_info(&oframe);
	y4m_si_set_width(&ostream, width);
	y4m_si_set_height(&ostream, height);
	y4m_si_set_interlace(&ostream, interlace);
	y4m_si_set_framerate(&ostream, rate_ratio);
	y4m_si_set_sampleaspect(&ostream, aspect_ratio);
	y4m_si_set_chroma(&ostream, Y4M_CHROMA_422);

	yuv[0] = malloc(y4m_si_get_plane_length(&ostream, 0));
	yuv[1] = malloc(y4m_si_get_plane_length(&ostream, 1));
	yuv[2] = malloc(y4m_si_get_plane_length(&ostream, 2));
	if	(yuv[0] == NULL || yuv[1] == NULL || yuv[2] == NULL)
		mjpeg_error_exit1("Could not malloc memory for 4:2:2 planes");

	frame_len = y4m_si_get_framelength(&ostream);
	input_frame = malloc(frame_len);
	if	(input_frame == NULL)
		mjpeg_error_exit1("Could not malloc memory for input frame");

	y4m_write_stream_header(fileno(stdout), &ostream);
	while	(y4m_read(fileno(stdin), input_frame, frame_len) == Y4M_OK)
		{
		Y_p = yuv[0];
		Cb_p = yuv[1];
		Cr_p = yuv[2];
		p = input_frame;
		if	(yuyv)	/* YUYV (YUY2) */
			{
			for	(i = 0; i < frame_len; i += 4)
				{
				*Y_p++ = *p++;
				*Cb_p++ = *p++;
				*Y_p++ = *p++;
				*Cr_p++ = *p++;
				}
			}
		else		/* UYVY */
			{
			for	(i = 0; i < frame_len; i += 4)
				{
				*Cb_p++ = *p++;
				*Y_p++ = *p++;
				*Cr_p++ = *p++;
				*Y_p++ = *p++;
				}
			}
		y4m_write_frame(fileno(stdout), &ostream, &oframe, yuv);
		}
	free(yuv[0]);
	free(yuv[1]);
	free(yuv[2]);
	free(input_frame);
	y4m_fini_stream_info(&ostream);
	y4m_fini_frame_info(&oframe);
	exit(0);
	}

static void usage(char *pgm)
	{

	fprintf(stderr, "%s repacks YUYV (YUY2) and UYVY 4:2:2 streams into 4:2:2 planar YUV4MPEG2 streams\n", pgm);
	fprintf(stderr, "usage: -w width -h height [-k] [-a pixel aspect] [-i p|t|b] -r rate\n");
	fprintf(stderr, "  Interlace codes [-i X]: p (none) t (top first) b (bottom first) (p)\n");
	fprintf(stderr, "  Rate (as ratio) [-r N:M] (24:1)\n");
	fprintf(stderr, "  Pixel aspect (as ratio) [-a N:M] (1:1)\n");
	fprintf(stderr, "  Interpret data as UYVY instead of YUYV [-k] (false)\n");
	fprintf(stderr, "NO DEFAULT for width or height!\n");
	fprintf(stderr, "\n");
	exit(1);
	}
