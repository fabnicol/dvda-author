/*
 * $Id: pgmtoy4m.c,v 1.3 2007/04/01 17:32:17 sms00 Exp $
 *
 * pgmtoy4m converts the PGM output of "mpeg2dec -o pgmpipe" to YUV4MPEG2 on
 * stdout.
 *
 * Note: mpeg2dec uses a rather interesting variation  of the PGM format - the
 * output is not actually traditional "Grey Maps" but rather a catenation of 
 * the Y'CrCb planes.  From the libmpeg2 project's video_out_pgm.c:
 *
 * Layout of the Y, U, and V buffers in our pgm image
 *
 *      YY        YY        YY
 * 420: YY   422: YY   444: YY
 *      UV        UV        UUVV
 *                UV        UUVV
 *
 * The PGM type is P5 ("raw") and the number of rows is really the
 * total of the Y', Cb and Cr heights.   The Cb and Cr data is "joined"
 * together.
 *
 * NOTE: You MAY need to know the field order (top or bottom field first),
 *	sample aspect ratio and frame rate because the PGM format makes
 *	none of that information available!  There are defaults provided
 *	that hopefully will do the right thing in the common cases.
 *
 *	The defaults provided are: 4:2:0, top field first, NTSC rate of 
 *	30000/1001 frames/second, and a sample aspect of 10:11.
 *
 *      If the incoming chroma space is not 4:2:0 then you MUST specify the
 *	"-x chroma_tag" option or chaos and/or a program crash will occur.
*/

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yuv4mpeg.h"

static	void	usage(char *progname);
static	void	chroma_usage(char *progname);
static	int	getint(int);

#define	P5MAGIC	(('P' * 256) + '5')

static int
getmagicnumber(int fd)
	{
	char	ch1 = -1, ch2 = -1;

	read(fd, &ch1, 1);
	read(fd, &ch2, 1);
	return((ch1 * 256) + ch2);
	}

static int
piperead(int fd, u_char *buf, int len)
	{
	int n = 0;
	int r = 0;

	while	(r < len)
		{
		n = read (fd, buf + r, len - r);
		if	(n <= 0)
			return(r);
		r += n;
		}
	return(r);
	}

int
main(int argc, char **argv)
	{
	int	width, height, uvlen, verbose = 1, fdout, fdin, c, i;
	int	columns, rows, maxval, magicn, frameno;
	int	ss_h, ss_v, chroma_height, chroma_width;
	int	chroma_mode = Y4M_UNKNOWN;
	u_char	*yuv[3];
	u_char	junkbuffer[16384];
	y4m_ratio_t	rate_ratio = y4m_fps_UNKNOWN;
	y4m_ratio_t	aspect_ratio = y4m_sar_UNKNOWN;
	char	ilace = Y4M_ILACE_TOP_FIRST;
	y4m_frame_info_t  oframe;
	y4m_stream_info_t ostream;

	fdin = fileno(stdin);
	fdout = fileno(stdout);

	y4m_accept_extensions(1);

	opterr = 0;
	while	((c = getopt(argc, argv, "i:a:r:hv:x:")) != EOF)
		{
		switch	(c)
			{
			case	'i':
				switch	(optarg[0])
					{
					case	'p':
						ilace = Y4M_ILACE_NONE;
						break;
					case	't':
						ilace = Y4M_ILACE_TOP_FIRST;
						break;
					case	'b':
						ilace = Y4M_ILACE_BOTTOM_FIRST;
						break;
					default:
						usage(argv[0]);
					}
				break;
			case	'v':
				verbose = atoi(optarg);
				if	(verbose < 0 || verbose > 3)
					mjpeg_error_exit1("Verbosity [0..2]");
				break;
			case	'a':
				i = y4m_parse_ratio(&aspect_ratio, optarg);
				if	(i != Y4M_OK)
					mjpeg_error_exit1("Invalid aspect: %s",
						optarg);
				break;
			case	'r':
				i = y4m_parse_ratio(&rate_ratio, optarg);
				if	(i != Y4M_OK)
					mjpeg_error_exit1("Invalid rate: %s",
						optarg);
				break;
			case	'x':
				chroma_mode = y4m_chroma_parse_keyword(optarg);
				if	(chroma_mode == Y4M_UNKNOWN)
					{
					if	(strcmp(optarg, "help") == 0)
						chroma_usage(argv[0]);
					mjpeg_error_exit1("bad chroma arg");
					}
				break;
			case	'h':
			case	'?':
			default:
				usage(argv[0]);
				/* NOTREACHED */
			}
		}

	if	(chroma_mode == Y4M_UNKNOWN)
		chroma_mode = Y4M_CHROMA_420MPEG2;

	if	(isatty(fdout))
		mjpeg_error_exit1("stdout must not be a terminal");

	mjpeg_default_handler_verbosity(verbose);

/*
 * Check that the input stream is really a P5 (rawbits PGM) stream, then if it
 * is retrieve the "rows" and "columns" (width and height) and the maxval.
 * The maxval will be 255 if the data was generated from mpeg2dec!
*/
	magicn = getmagicnumber(fdin);
	if	(magicn != P5MAGIC)
		mjpeg_error_exit1("Input not P5 PGM data, got %x", magicn);
	columns = getint(fdin);
	rows = getint(fdin);
	maxval = getint(fdin);
	mjpeg_info("P5 cols: %d rows: %d maxval: %d", columns, rows, maxval);

	if	(maxval != 255)
		mjpeg_error_exit1("Maxval (%d) != 255, not mpeg2dec output?",
			maxval);
/*
 * Put some sanity checks on the sizes - handling up to 4096x4096 should be 
 * enough for now :)
*/
	if	(columns < 0 || columns > 4096 || rows < 0 || rows > 4096)
		mjpeg_error_exit1("columns (%d) or rows(%d) < 0 or > 4096",
			columns, rows);

	y4m_init_frame_info(&oframe);
	y4m_init_stream_info(&ostream);
	y4m_si_set_chroma(&ostream, chroma_mode);

	if	(y4m_si_get_plane_count(&ostream) != 3)
		mjpeg_error_exit1("Only the 3 plane formats supported");

	ss_h = y4m_chroma_ss_x_ratio(chroma_mode).d;
	ss_v = y4m_chroma_ss_y_ratio(chroma_mode).d;

	width = columns;
	height = (rows * ss_v) / (ss_v + 1);
	chroma_width = width / ss_h;
	chroma_height = height / ss_v;
	uvlen = chroma_height * chroma_width;

	yuv[0] = malloc(height * width);
	yuv[1] = malloc(uvlen);
	yuv[2] = malloc(uvlen);
	if	(yuv[0] == NULL || yuv[1] == NULL || yuv[2] == NULL)
		mjpeg_error_exit1("malloc yuv[]");

/*
 * If the (sample) aspect ratio and frame rate were not specified on the
 * command line try to intuit the video norm of NTSC or PAL by looking at the
 * height of the frame.
*/
	if	(Y4M_RATIO_EQL(aspect_ratio, y4m_sar_UNKNOWN))
		{
		if	(height == 480 || height == 240)
			{
			aspect_ratio = y4m_sar_NTSC_CCIR601;
			mjpeg_warn("sample aspect not specified, using NTSC CCIR601 value based on frame height of %d", height);
			}
		else if	(height == 576 || height == 288)
			{
			aspect_ratio = y4m_sar_PAL_CCIR601;
			mjpeg_warn("sample aspect not specified, using PAL CCIR601 value based on frame height of %d", height);
			}
		else
			{
			mjpeg_warn("sample aspect not specified and can not be inferred from frame height of %d (leaving sar as unknown", height);
			}
		}
	if	(Y4M_RATIO_EQL(rate_ratio, y4m_fps_UNKNOWN))
		{
		if	(height == 480 || height == 240)
			{
			rate_ratio = y4m_fps_NTSC;
			mjpeg_warn("frame rate not specified, using NTSC value based on frame height of %d", height);
			}
		else if	(height == 576 || height == 288)
			{
			rate_ratio = y4m_fps_PAL;
			mjpeg_warn("frame rate not specified, using PAL value based on frame height of %d", height);
			}
		else
			{
			mjpeg_warn("frame rate not specified and can not be inferred from frame height of %d (using NTSC value)", height);
			rate_ratio = y4m_fps_NTSC;
			}
		}
	y4m_si_set_interlace(&ostream, ilace);
	y4m_si_set_framerate(&ostream, rate_ratio);
	y4m_si_set_sampleaspect(&ostream, aspect_ratio);
	y4m_si_set_width(&ostream, width);
	y4m_si_set_height(&ostream, height);

	y4m_write_stream_header(fdout, &ostream);

/*
 * Now continue reading P5 frames as long as the magic number appears.  Corrupt
 * (malformed) input or EOF will terminate the loop.   The columns, rows and
 * maxval must be read to get to the raw data.  It's probably not worth doing
 * anything (in fact it's unclear what could be done ;)) with the values 
 * if they're different from the first header which was used to set the
 * output stream parameters.
*/
	frameno = 0;
	while	(1)	
		{
		for	(i = 0; i < height; i++)
			{
			piperead(fdin, yuv[0] + i * width, width);
			piperead(fdin, junkbuffer, 2 * chroma_width - width);
			}
		for	(i = 0; i < chroma_height; i++)
			{
			piperead(fdin, yuv[1] + i * chroma_width, chroma_width);
			piperead(fdin, yuv[2] + i * chroma_width, chroma_width);
			}
		y4m_write_frame(fdout, &ostream, &oframe, yuv);

		magicn = getmagicnumber(fdin);
		if	(magicn != P5MAGIC)
			{
			mjpeg_debug("frame: %d got magic: %x\n",
				frameno, magicn);
			break;
			}
		columns = getint(fdin);
		rows = getint(fdin);
		maxval = getint(fdin);
		frameno++;
		mjpeg_debug("frame: %d P5MAGIC cols: %d rows: %d maxval: %d", frameno, columns, rows, maxval);
		}
	y4m_fini_frame_info(&oframe);
	y4m_fini_stream_info(&ostream);

	return 0;
	}

static void
usage(char *progname)
	{

	fprintf(stderr, "%s usage: [-v n] [-i t|b|p] [-x chroma] [-a sample aspect] [-r rate]\n",
		progname);
	fprintf(stderr, "%s\taspect and rate in ratio form: -a  10:11 and -r 30000:1001 or -r 25:1 for example\n", progname);
	fprintf(stderr, "%s\t chroma default is 420mpeg2, -x help for list\n",
		progname);
	exit(0);
	}

static int
getint(int fd)
	{
	char	ch;
	int	i;

	do
		{
		if	(read(fd, &ch, 1) == -1)
			return(-1);
		if	(ch == '#')
			{
			while	(ch != '\n')
				{
				if	(read(fd, &ch, 1) == -1)
					return(-1);
				}
			}
		} while (isspace(ch));

	if	(!isdigit(ch))
		return(-1);

	i = 0;
	do
		{
		i = i * 10 + (ch - '0');
		if	(read(fd, &ch, 1) == -1)
			break;
		} while (isdigit(ch));
	return(i);
	}

void chroma_usage(char *pname)
	{
	int mode = 0;
	const char *keyword;

	fprintf(stderr, "%s -x usage: Only the 3 plane formats are actually supported\n",
		pname);
	for	(mode = 0; (keyword = y4m_chroma_keyword(mode)) != NULL; mode++)
		fprintf(stderr, "\t%s - %s\n", keyword, y4m_chroma_description(mode));
	exit(1);
	}
