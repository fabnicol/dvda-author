/*
 * $Id: y4mtoyuv.c,v 1.3 2007/04/01 18:06:06 sms00 Exp $
 *
 * Simple program to convert the YUV4MPEG2 format used by the 
 * mjpeg.sourceforge.net suite of programs into pure EYUV format used
 * by the mpeg4ip project and other programs. 
 *
 * 2001/10/19 - Rewritten to use the y4m_* routines from mjpegtools.
 * 2004/1/1 - Added XYSCSS tag handling to deal with 411, 422, 444 data 
 * 2004/4/5 - Rewritten to use the new YUV4MPEG2 API.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yuv4mpeg.h"
#include "mjpeg_logging.h"

static	void	usage(void);

int main(int argc, char **argv)
	{
	int	c, err;
	int	fd_in = fileno(stdin), fd_out = fileno(stdout);
	u_char	*yuv[3];
	int	plane_length[3];
	y4m_stream_info_t istream;
	y4m_frame_info_t iframe;

	opterr = 0;
	while	((c = getopt(argc, argv, "h")) != EOF)
		{
		switch	(c)
			{
			case	'h':
			case	'?':
			default:
				usage();
			}
		}

	y4m_accept_extensions(1);

	y4m_init_stream_info(&istream);
	y4m_init_frame_info(&iframe);

	err = y4m_read_stream_header(fd_in, &istream);
	if	(err != Y4M_OK)
		mjpeg_error_exit1("Input stream error: %s\n", y4m_strerr(err));

	if	(y4m_si_get_plane_count(&istream) != 3)
		mjpeg_error_exit1("only 3 plane formats supported");

	plane_length[0] = y4m_si_get_plane_length(&istream, 0);
	plane_length[1] = y4m_si_get_plane_length(&istream, 1);
	plane_length[2] = y4m_si_get_plane_length(&istream, 2);

	yuv[0] = malloc(plane_length[0]);
	yuv[1] = malloc(plane_length[1]);
	yuv[2] = malloc(plane_length[2]);

	y4m_log_stream_info(mjpeg_loglev_t("info"), "", &istream);

	while	(y4m_read_frame(fd_in, &istream, &iframe, yuv) == Y4M_OK)
		{
		if	(y4m_write(fd_out, yuv[0], plane_length[0]) != Y4M_OK)
			break;
		if	(y4m_write(fd_out, yuv[1], plane_length[1]) != Y4M_OK)
			break;
		if	(y4m_write(fd_out, yuv[2], plane_length[2]) != Y4M_OK)
			break;
		}
	y4m_fini_frame_info(&iframe);
	y4m_fini_stream_info(&istream);
	exit(0);
	}

static void usage()
	{

	mjpeg_error_exit1("<file.y4m > file.yuv");
	/* NOTREACHED */
	}
