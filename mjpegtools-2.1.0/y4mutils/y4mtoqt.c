/*
 * $Id: y4mtoqt.c,v 1.13 2007/05/10 03:07:36 sms00 Exp $
 *
 * Utility to place 4:2:2 or 4:4:4 YUV4MPEG2 data in a Quicktime wrapper.   An
 * audio track can also be added by specifying '-a wavfile' (16bit pcm only).
 * The interlacing, frame rate, frame size and field order are extracted 
 * from the YUV4MPEG2 header.  This is the reverse of 'qttoy4m' which dumps
 * planar data from a Quicktime file (and optionally a specified audio track 
 * to a wav file).
 *
 * Usage: y4mtoqt [-X] [-a wavfile] -o outputfile < 422yuv4mpeg2stream
 *
 *   -X enables 10bit packed output, 2vuy becomes v210 and v308 becomes v410.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <quicktime.h>
#include <lqt.h>
#include <colormodels.h>
#include "yuv4mpeg.h"
#include "avilib.h"

static	void	usage(void);
static void do_audio(quicktime_t *, uint8_t *, int, int,  int);

int
main(int argc, char **argv)
	{
	char	*outfilename = NULL;
	char	*audiofilename = NULL;
	uint8_t	*yuv[3], *p;
	uint16_t *p16;
	uint16_t *yuv10[3];
	int	fdin, y_len, u_len, v_len, nfields = 1, dominance = 0, afd = -1;
	int	imodel = 0, allow_wrong_yv12 = 0;
	int	err, i, c, frames, channels = 0, y4mchroma, tenbit = 0;
	char	*qtchroma = NULL;
	quicktime_t *qtf;
	quicktime_pasp_t pasp;
#if 0
	quicktime_colr_t colr;
#endif
	quicktime_clap_t clap;
	y4m_stream_info_t istream;
	y4m_frame_info_t iframe;
	y4m_ratio_t rate, sar;
	struct wave_header wavhdr;
	y4m_accept_extensions(1);
	
	fdin = fileno(stdin);

	opterr = 0;
	while	((c = getopt(argc, argv, "ko:a:X")) != -1)
		{
		switch	(c)
			{
			case	'k':
				allow_wrong_yv12 = 1;
				break;
			case	'X':
				tenbit = 1;
				break;
			case	'o':
				outfilename = optarg;
				break;
			case	'a':
				audiofilename = optarg;
				break;
			case	'?':
			default:
				usage();
			}
		}
	argc -= optind;
	argv += optind;
	if	(argc)
		usage();

	if	(!outfilename)
		usage();

	if	(audiofilename)
		{
		afd = open(audiofilename, O_RDONLY);
		if	(afd < 0)
			mjpeg_error_exit1("Can not open audio file '%s'", 
						audiofilename);
		if	(AVI_read_wave_header(afd, &wavhdr) == -1)
			mjpeg_error_exit1("'%s' is not a valid WAV file",
						audiofilename);
		channels = wavhdr.common.wChannels;
		}

	y4m_init_stream_info(&istream);
	y4m_init_frame_info(&iframe);

	err = y4m_read_stream_header(fdin, &istream);
	if	(err != Y4M_OK)
		mjpeg_error_exit1("Input header error: %s", y4m_strerr(err));

	if	(y4m_si_get_plane_count(&istream) != 3)
		mjpeg_error_exit1("Only 3 plane formats supported");

	rate = y4m_si_get_framerate(&istream);

	switch	(y4mchroma = y4m_si_get_chroma(&istream))
		{
		case	Y4M_CHROMA_420MPEG2:
		case	Y4M_CHROMA_420JPEG:
			/*
			 * Quicktime doesn't appear to have a way to reliably
			 * tell the two non-PALDV variants apart so treat them
			 * both the same (like most other software in the world)
			*/
			qtchroma = QUICKTIME_YUV420;	/* yv12 */
			imodel = BC_YUV420P;
			break;
		case	Y4M_CHROMA_422:
			if	(tenbit)
				qtchroma = QUICKTIME_V210;
			else
				{
				qtchroma = QUICKTIME_2VUY;
				imodel = BC_YUV422P;    /* Input is planar */
				}
			break;
		case	Y4M_CHROMA_444:
			if	(tenbit)
				qtchroma = QUICKTIME_V410;
			else
				{
				qtchroma = QUICKTIME_V308;
				imodel = BC_YUV444P;  /* Need this?? */
				}
			break;
		default:
			mjpeg_error_exit1("unsupported chroma sampling: %s",
				y4m_chroma_keyword(y4mchroma));
			break;
		}

	y_len = y4m_si_get_plane_length(&istream, 0);
	u_len = y4m_si_get_plane_length(&istream, 1);
	v_len = y4m_si_get_plane_length(&istream, 2);
	yuv[0] = malloc(y_len);
	yuv[1] = malloc(u_len);
	yuv[2] = malloc(v_len);
	if	(tenbit)
		{
		yuv10[0] = malloc(y_len * sizeof(uint16_t));
		yuv10[1] = malloc(u_len * sizeof(uint16_t));
		yuv10[2] = malloc(v_len * sizeof(uint16_t));
		}

	qtf = quicktime_open(outfilename, 0, 1);
	if	(!qtf)
		mjpeg_error_exit1("quicktime_open(%s,0,1) failed", outfilename);

	quicktime_set_video(qtf, 1,
				y4m_si_get_width(&istream),
				y4m_si_get_height(&istream),
				(double) rate.n / rate.d,
				qtchroma);

	if	(imodel != 0)
		lqt_set_cmodel(qtf, 0, imodel);

	if	(audiofilename)
		quicktime_set_audio(qtf, channels,
					wavhdr.common.dwSamplesPerSec,
					wavhdr.common.wBitsPerSample,
					QUICKTIME_TWOS);
/*
 * http://developer.apple.com/quicktime/icefloe/dispatch019.html#fiel
 *
 * "dominance" is what Apple calls "detail".  From what I can figure out
 * the "bottom field" first corresponds to a "detail" setting of 14 and
 * "top field first" is a "detail" setting of 9.
*/
	switch	(y4m_si_get_interlace(&istream))
		{
		case	Y4M_ILACE_BOTTOM_FIRST:
			dominance = 14;	/* Weird but that's what Apple says */
			nfields = 2;
			break;
		case	Y4M_ILACE_TOP_FIRST:
			dominance = 9;
			nfields = 2;
			break;
		case	Y4M_UNKNOWN:
		case	Y4M_ILACE_NONE:
			dominance = 0;
			nfields = 1;
			break;
		case	Y4M_ILACE_MIXED:
			mjpeg_error_exit1("Mixed field dominance unsupported");
			break;
		default:
			mjpeg_error_exit1("UNKNOWN field dominance %d",
				y4m_si_get_interlace(&istream));
			break;
		}

	if	(lqt_set_fiel(qtf, 0, nfields, dominance) == 0)
		mjpeg_error_exit1("lqt_set_fiel(qtf, 0, %d, %d) failed",
				nfields, dominance);

	sar = y4m_si_get_sampleaspect(&istream);
	if	(Y4M_RATIO_EQL(sar, y4m_sar_UNKNOWN))
		pasp.hSpacing = pasp.vSpacing = 1;
	else
		{
		pasp.hSpacing = sar.n;
		pasp.vSpacing = sar.d;
		}
	if	(lqt_set_pasp(qtf, 0, &pasp) == 0)
		mjpeg_error_exit1("lqt_set_pasp(qtf, 0, %d/%d) failed",
			pasp.hSpacing, pasp.vSpacing);

/*
 * Don't do this for now - it can crash FinalCutPro if the colr atom is
 * not exactly correct.
*/
#if	0
	colr.colorParamType = 'nclc';
	colr.primaries = 2;
	colr.transferFunction = 2;
	colr.matrix = 2;
	if	(lqt_set_colr(qtf, 0, &colr) == 0)
		mjpeg_error_exit1("lqt_set_colr(qtf, 0,...) failed");
#endif
	clap.cleanApertureWidthN = y4m_si_get_width (&istream);;
	clap.cleanApertureWidthD = 1;
	clap.cleanApertureHeightN = y4m_si_get_height (&istream);
	clap.cleanApertureHeightD = 1;
	clap.horizOffN = 0;
	clap.horizOffD = 1;
	clap.vertOffN = 0;
	clap.vertOffD = 1;
	if	(lqt_set_clap(qtf, 0, &clap) == 0)
		mjpeg_error_exit1("lqt_set_clap(qtf, 0, ...) failed");

	for	(;y4m_read_frame(fdin,&istream,&iframe,yuv) == Y4M_OK; frames++)
		{
		if	(tenbit)
			{
			p = yuv[0];
			p16 = yuv10[0];
			for	(i = 0; i < y_len; i++)
				*p16++ = *p++ << 8;
			p = yuv[1];
			p16 = yuv10[1];
			for	(i = 0; i < u_len; i++)
				*p16++ = *p++ << 8;
			p = yuv[2];
			p16 = yuv10[2];
			for	(i = 0; i < v_len; i++)
				*p16++ = *p++ << 8;
			}
/*
 * What libquicktime calls 'yv12' (QUICKTIME_YUV420) is actually 'iyuv' 
 * (also known as 'i420').  In order to make the data match the fourcc/label
 * the U and V planes need to be swapped.  After all, if the file is labeled 
 * 'yv12' then the data should be in 'yv12' order!
 *
 * Breakage, for compatiblity with quicktime4linux, can be forced by using
 * '-k'.  This allows storing 'iyuv' ('i420') data inside a file that is 
 * labeled as 'yv12' :(
 *
 * It should be noted that very very little outside of V4L knows anything
 * about uncompressed 4:2:0 - the 4:2:0 color space is used but only with
 * compressed formats it seems.
*/

		if	(strcmp(qtchroma, QUICKTIME_YUV420) == 0)
			{
			if	(allow_wrong_yv12 == 0)
				{
				p = yuv[1];
				yuv[1] = yuv[2];
				yuv[2] = p;
				}
			}

		err = quicktime_encode_video(qtf, tenbit ? (uint8_t **)yuv10: yuv, 0);
		if	(err != 0)
			mjpeg_error_exit1("quicktime_encode_video failed.");
		}

	if	(audiofilename)
		{
		uint8_t *buffer;
		int bufsize, n, bps;

		mjpeg_info("channels %d SamplesPerSec %d bits_sample %d",
			channels, wavhdr.common.dwSamplesPerSec,
				  wavhdr.common.wBitsPerSample);

		bps = (wavhdr.common.wBitsPerSample + 7)/8;
		bufsize = 8192 * channels * bps;
		buffer = malloc(bufsize);
		while	((n = AVI_read_wave_pcm_data(afd, buffer, bufsize)) > 0)
			do_audio(qtf, buffer, channels, bps, n / (channels * bps));
		}
	quicktime_close(qtf);
	exit(0);
	}

static void 
do_audio(quicktime_t *qtf, uint8_t *buff, int channels, int bps, int samps)
	{
	int	res;
	int	i, j;
	int16_t *qt_audio = (int16_t *)buff, **qt_audion;

	qt_audion = malloc(channels * sizeof (int16_t **));
	for	(i = 0; i < channels; i++)
		qt_audion[i] = (int16_t *)malloc(samps * bps);

	/* Deinterleave the audio into separate channel buffers */
	for	(i = 0; i < samps; i++)
		{
		for	(j = 0; j < channels; j++)
			qt_audion[j][i] = qt_audio[(channels*i) + j];
		}
	res = lqt_encode_audio_track(qtf, qt_audion, NULL, samps, 0);
	for	(j = 0; j < channels; j++)
		free(qt_audion[j]);
	free(qt_audion);
	}

static void
usage()
	{
	mjpeg_warn("usage: [-k] [-X] [-a inputwavfile] -o outfile");
	mjpeg_warn("       -X = use v210 (default 2vuy) for 4:2:2, v410 (default v308) for 4:4:4");
	mjpeg_warn("       -k = do not perform lqt workaround (U and V plane swap) (default 0 - i.e. DO the workaround)");
	exit(1);
	}
