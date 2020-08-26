/*
 * $Id: y4mblack.c,v 1.4 2009/09/19 18:57:00 sms00 Exp $
 *
 * Used to generate YUV4MPEG2 frames with the specific interlace, 
 * dimensions, pixel aspect  and Y/U/V values.  By default the frames 
 * generated are 16/128/128 or pure black.
 *
 * This is strictly an  output program, stdin is ignored.
*/

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "yuv4mpeg.h"

static	void	usage(char *);
static	void	chroma_usage(char *);

int
main(int argc, char **argv)
	{
	int	sts, c, width = 640, height = 480, noheader = 0;
	int	Y = 16, U = 128, V = 128, chroma_mode = Y4M_CHROMA_420MPEG2;
	int	numframes = 1, force = 0;
	y4m_ratio_t	rate_ratio = y4m_fps_NTSC;
	y4m_ratio_t	aspect_ratio = y4m_sar_SQUARE;
	int	plane_length[3];
	u_char	*yuv[3];
	y4m_stream_info_t ostream;
	y4m_frame_info_t oframe;
	char	interlace = Y4M_ILACE_NONE;

	opterr = 0;
	y4m_accept_extensions(1);

	while	((c = getopt(argc, argv, "Hfx:w:h:r:i:a:Y:U:V:n:")) != EOF)
		{
		switch	(c)
			{
			case	'H':
				noheader = 1;
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
			case	'Y':
				Y = atoi(optarg);
				break;
			case	'U':
				U = atoi(optarg);
				break;
			case	'V':
				V = atoi(optarg);
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
			case	'x':
				chroma_mode = y4m_chroma_parse_keyword(optarg);
				if	(chroma_mode == Y4M_UNKNOWN)
					{
					if	(strcmp(optarg, "help") != 0)
						mjpeg_error("Invalid -x arg '%s'", optarg);
					chroma_usage(argv[0]);
					}

				break;
			case	'f':
				force = 1;
				break;
			case	'n':
				numframes = atoi(optarg);
				break;
			case	'?':
			default:
				usage(argv[0]);
			}
		}

	if	(width <= 0)
		mjpeg_error_exit1("Invalid Width: %d", width);

	if	(height <= 0)
		mjpeg_error_exit1("Invalid Height: %d", height);

	if	(!force && (Y < 16 || Y > 235))
		mjpeg_error_exit1("16 < Y < 235");

	if	(!force && (U < 16 || U > 240))
		mjpeg_error_exit1("16 < U < 240");

	if	(!force && (V < 16 || V > 240))
		mjpeg_error_exit1("16 < V < 240");

	y4m_init_stream_info(&ostream);
	y4m_init_frame_info(&oframe);
	y4m_si_set_width(&ostream, width);
	y4m_si_set_height(&ostream, height);
	y4m_si_set_interlace(&ostream, interlace);
	y4m_si_set_framerate(&ostream, rate_ratio);
	y4m_si_set_sampleaspect(&ostream, aspect_ratio);
	y4m_si_set_chroma(&ostream, chroma_mode);

	if	(y4m_si_get_plane_count(&ostream) != 3)
		mjpeg_error_exit1("Only the 3 plane formats supported");

	plane_length[0] = y4m_si_get_plane_length(&ostream, 0);
	plane_length[1] = y4m_si_get_plane_length(&ostream, 1);
	plane_length[2] = y4m_si_get_plane_length(&ostream, 2);

	yuv[0] = malloc(plane_length[0]);
	yuv[1] = malloc(plane_length[1]);
	yuv[2] = malloc(plane_length[2]);

/*
 * Now fill the array once with black but use the provided Y, U and V values
*/
	memset(yuv[0], Y, plane_length[0]);
	memset(yuv[1], U, plane_length[1]);
	memset(yuv[2], V, plane_length[2]);

	if	(noheader == 0)
		y4m_write_stream_header(fileno(stdout), &ostream);
	while	(numframes--)
		y4m_write_frame(fileno(stdout), &ostream, &oframe, yuv);

	free(yuv[0]);
	free(yuv[1]);
	free(yuv[2]);
	y4m_fini_stream_info(&ostream);
	y4m_fini_frame_info(&oframe);
	exit(0);
	}

void usage(char *pgm)
	{

	fprintf(stderr, "%s usage: [-H] [-f] [-n numframes] [-w width] [-h height] [-Y val] [-U val] [-V val] [-a pixel aspect] [-i p|t|b] [-x chroma] [-r rate]\n", pgm);
	fprintf(stderr, "\n  Omit the YUV4MPEG2 header [-H]");
	fprintf(stderr, "\n  Specify chroma sampling [-x string] (420mpeg2)");
	fprintf(stderr, "\n      -x help for list");
	fprintf(stderr, "\n      NOTE: Only 3 plane formats supported");
	fprintf(stderr, "\n  Allow out of range Y/U/V values [-f]");
	fprintf(stderr, "\n  Numframes [-n num] (1)");
	fprintf(stderr, "\n  Width [-w width] (640)");
	fprintf(stderr, "\n  Height [-h height] (480)");
	fprintf(stderr, "\n  Interlace codes [-i X] p (none/progressive) t (top first) b (bottom first) (p)");
	fprintf(stderr, "\n  Rate (as ratio) [-r N:M] (30000:1001)");
	fprintf(stderr, "\n  Pixel aspect (as ratio) [-a N:M] (1:1)");
	fprintf(stderr, "\n  Y: [-Y val] (16)");
	fprintf(stderr, "\n  U: [-U val] (128)");
	fprintf(stderr, "\n  V: [-V val] (128)");
	fprintf(stderr, "\n");
	exit(1);
	}

void chroma_usage(char *pgm)
	{
	int mode = 0;
	const char *keyword;

	fprintf(stderr, "%s -x usage: Only the 3 plane formats are actually supported\n",
		pgm);
	for	(mode = 0; (keyword = y4m_chroma_keyword(mode)) != NULL; mode++)
		fprintf(stderr, "\t%s - %s\n", keyword, y4m_chroma_description(mode));
	exit(1);
	}
