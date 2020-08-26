/*
 * $Id: y4mivtc.c,v 1.2 2006/05/18 16:21:42 sms00 Exp $
 *
 * Simple program to remove the 2:3 pulldown from a YUV4MPEG2 stream.   This
 * was written because yuvkineco only works with TOP field first streams and
 * the 'hack' of using "yuvcorrect  -T TOP_FORWARD" introduces a phase error
 * that prevents yuvkineco from finding/locking-on-to the correct cadence.
 *
 * With a 2:3 pulldown 4 film frames are converted into 5 video frames by
 * repeating fields.  The four film frames are usually called "A", "B", "C" 
 * and "D".  The 2:3 pulldown process emits:
 *
 *   A1/A2 B1/B2 B1/C2 C1/D2 D1/D2
 *
 * Reversing that process means reading the first two frames and sending them
 * to the output unchanged.  The next two frames are "combined" by taking
 * the 'C2' field from frame 3 and using that with 'C1' from frame 4 to create
 * the original C frame.  The last frame is output directly as the 'D' frame.
 *
 * NOTE: The file MUST start on the 5 frame boundary containing the 2:3
 *       sequence!  No attempt is made to determine if the file begins with
 *       the "A" frame or not.  Also editing can destroy the cadence - if you
 *       edit a file with 2:3 pulldown you have to be careful to make your
 *       edits on the 5 frame boundary containing the complete 2:3:2:3 sequence.
 *
 * REMEMBER: Correct 2:3 pulldown removal can only be done if the data starts
 *           on a 2:3:2: boundary AND there are no breaks in cadence.
 *
 *       You have been warned :)
 *
 * 2006/3/13 - Initial release.
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
	int	c, err, ilace;
	int	fd_in = fileno(stdin), fd_out = fileno(stdout);
	y4m_ratio_t rate;
	y4m_stream_info_t si, so;
	y4m_frame_info_t fi;
	uint8_t *top1[3], *bot1[3], *top2[3], *bot2[3];

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
	y4m_init_stream_info(&si);
	y4m_init_stream_info(&so);
	y4m_init_frame_info(&fi);

	err = y4m_read_stream_header(fd_in, &si);
	if	(err != Y4M_OK)
		mjpeg_error_exit1("Input stream error: %s\n", y4m_strerr(err));

	if	(y4m_si_get_plane_count(&si) != 3)
		mjpeg_error_exit1("only 3 plane formats supported");

	rate = y4m_si_get_framerate(&si);
	if	(!Y4M_RATIO_EQL(rate, y4m_fps_NTSC))
		mjpeg_error_exit1("input stream not NTSC 30000:1001");

	ilace = y4m_si_get_interlace(&si);
	if	(ilace != Y4M_ILACE_BOTTOM_FIRST && ilace != Y4M_ILACE_TOP_FIRST)
		mjpeg_error_exit1("input stream not interlaced");

	top1[0] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,0) / 2);
	top1[1] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,1) / 2);
	top1[2] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,2) / 2);

	bot1[0] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,0) / 2);
	bot1[1] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,1) / 2);
	bot1[2] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,2) / 2);

	top2[0] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,0) / 2);
	top2[1] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,1) / 2);
	top2[2] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,2) / 2);

	bot2[0] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,0) / 2);
	bot2[1] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,1) / 2);
	bot2[2] = (uint8_t *) malloc(y4m_si_get_plane_length(&si,2) / 2);

	y4m_copy_stream_info(&so, &si);
	y4m_si_set_framerate(&so, y4m_fps_NTSC_FILM);
	y4m_si_set_interlace(&so, Y4M_ILACE_NONE);

/*
 * At this point the input stream has been verified to be interlaced NTSC,
 * the output stream rate set to NTSC_FILM, interlacing tag changed to 
 * progressive, and the field buffers allocated.
 *
 * Time to write the output stream header and commence processing input.
*/
	y4m_write_stream_header(fd_out, &so);

	while	(1)
		{
		err = y4m_read_fields(fd_in, &si, &fi, top1, bot1);
		if	(err != Y4M_OK)
			goto done;
		y4m_write_fields(fd_out, &so, &fi, top1, bot1);		/* A */

		err = y4m_read_fields(fd_in, &si, &fi, top1, bot1);
		if	(err != Y4M_OK)
			goto done;
		y4m_write_fields(fd_out, &so, &fi, top1, bot1);		/* B */

		err = y4m_read_fields(fd_in, &si, &fi, top1, bot1);
		if	(err != Y4M_OK)
			goto done;
		err = y4m_read_fields(fd_in, &si, &fi, top2, bot2);
		if	(err != Y4M_OK)
			{
/*
 * End of input when reading the 2nd "mixed field" frame (C+D).  The previous
 * frame was the first "mixed field" frame (B+C).  Rather than emit a mixed
 * interlaced frame duplicate a field and output the previous frame.
*/
			if	(ilace == Y4M_ILACE_BOTTOM_FIRST)
				y4m_write_fields(fd_out, &so, &fi, bot1,bot1);
			else
				y4m_write_fields(fd_out, &so, &fi, top1,top1);
			goto done;
			}
/*
 * Now the key part of the processing - effectively discarding the first mixed
 * frame with fields from frames B + C and creating the C frame from the two
 * mixed frames.  For a BOTTOM FIELD FIRST stream use the 'top' field from
 * frame 3 and the 'bottom' fields from frame 4.  With a TOP FIELD FIRST stream
 * it's the other way around - use the 'bottom' field from frame 3 and the
 * 'top' field from frame 4.
*/
		if	(ilace == Y4M_ILACE_BOTTOM_FIRST)
			y4m_write_fields(fd_out, &so, &fi, top1, bot2);	/* C */
		else
			y4m_write_fields(fd_out, &so, &fi, top2, bot1); /* C */
		
		err = y4m_read_fields(fd_in, &si, &fi, top1, bot1);
		y4m_write_fields(fd_out, &so, &fi, top1, bot1);		/* D */
		}
done:	y4m_fini_frame_info(&fi);
	y4m_fini_stream_info(&si);
	y4m_fini_stream_info(&so);
	exit(0);
	}

static void usage()
	{

	mjpeg_error_exit1("<file-30000:1001.y4m > file-24000:1001.y4m");
	/* NOTREACHED */
	}
