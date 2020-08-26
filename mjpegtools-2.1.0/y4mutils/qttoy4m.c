/*
 * $Id: qttoy4m.c,v 1.11 2006/06/25 02:36:35 sms00 Exp $
 *
 * Extract uncompressed Y'CbCr data from a Quicktime file and generate a
 * YUV4MPEG2 stream.  As many of the attributes (frame rate, sample aspect, etc)
 * as possible are retrieved from the Quicktime file.  However not all QT
 * files have all the attributes or perhaps they are not exactly as desired.
 * Commandline options are provided and if used with override information from
 * the Quicktime file.
 *
 * Optionally, if "-A filename" is used, the specified audio track (-t, default
 * is the first audio track found) will be extracted to a wav file.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <quicktime.h>
#include <lqt.h>
#include <colormodels.h>
#include "yuv4mpeg.h"
#include "mpegconsts.h"
#include "lav_io.h"

#define	nSAMPS 8192

typedef struct WavHeader
{
	uint8_t riff[4];	/* RIFF */
	uint32_t riff_length;
	uint8_t type[4];

	uint8_t format[4];	/* FORMAT */
	uint32_t format_length;

	uint16_t tag;		/* COMMON */
	uint16_t channels;
	uint32_t rate;
	uint32_t bytespersecond;
	uint16_t bytespersample;
	uint16_t bitspersample;

	uint8_t data[4];	/* DATA */
	uint32_t data_length;
} WAVheader;

static void fixwavheader(WAVheader *, int);
static void do_audio(quicktime_t *, FILE *, int);

static void usage(void)
{
	mjpeg_warn("usage: [-r rate] [-a sar] [-i t|b|p] [-t vtrack] [-T atrack] [-A audiofile] input.mov > output.y4m");
	mjpeg_warn("    Decode specified video track to YUV4MPEG2 on stdout (-1 disables video output)");
	mjpeg_warn("    Extract specified audio track to 'audiofile'");
	exit(1);
}

int main(int argc, char *argv[])
{
	quicktime_t *file;
	quicktime_pasp_t pasp;
	int nfields = 0, dominance = 0, interlace = Y4M_UNKNOWN;
	FILE *audio_fp = NULL;
	char	*audiofile = NULL;
	int64_t i, length, width, height;
	int	vtrack = 0, atrack = 0, c, sts, cmodel,	ss_h, ss_v;
	int	y4mchroma = Y4M_UNKNOWN, omodel;
	double	qtrate;
	y4m_stream_info_t ostream;
	y4m_frame_info_t oframe;
	y4m_ratio_t rate = y4m_fps_UNKNOWN, sar = y4m_sar_UNKNOWN;
	uint8_t **qtrows, *yuv[3];

	opterr = 0;

	while	((c = getopt(argc, argv, "T:A:t:i:a:r:h")) != EOF)
		{
		switch	(c)
			{
			case	'T':
				atrack = atoi(optarg);
				break;
			case	'A':
				audiofile = strdup(optarg);
				break;
			case	't':
				vtrack = atoi(optarg);
				break;
			case	'a':
				sts = y4m_parse_ratio(&sar, optarg);
				if	(sts != Y4M_OK)
					mjpeg_error_exit1("Invalid aspect: %s",
						optarg);
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
						usage();
					}
				break;
			case	'r':
				sts = y4m_parse_ratio(&rate, optarg);
				if	(sts != Y4M_OK)
					mjpeg_error_exit1("Invalid rate: %s",
						optarg);
				break;
			case	'?':
			case	'h':
			default:
				usage();
			}
		}

	argc -= optind;
	argv += optind;

	if	(argc < 1)
		usage();

	if	(!(file = quicktime_open(argv[0], 1, 0)))
		mjpeg_error_exit1("quicktime_open(%s,1,0)failed\n", argv[0]);

	if	(audiofile != NULL)
		{
		if	(quicktime_audio_tracks(file) < atrack)
			mjpeg_warn("No audio track %d in file\n", atrack);
		else if	((audio_fp = fopen(audiofile, "w")) == NULL)
			mjpeg_warn("fopen(%s,w) failed\n", audiofile);
		}

	if	(vtrack < 0)
		goto doaudio;

	if	(quicktime_supported_video(file, vtrack) == 0)
		mjpeg_error_exit1("Unsupported video codec");

	if	(quicktime_video_tracks(file) < vtrack)
		mjpeg_error_exit1("No video track %d in file\n", vtrack);

	length = quicktime_video_length(file, vtrack);
	width = quicktime_video_width(file, vtrack);
	height = quicktime_video_height(file, vtrack);

	cmodel = lqt_get_cmodel(file, vtrack);
	omodel = 0;

	if	(lqt_colormodel_is_yuv(cmodel) == 0)
		mjpeg_error_exit1("Color space '%s' not Y'CbCr",
				lqt_colormodel_to_string(cmodel));

	if	(lqt_colormodel_has_alpha(cmodel) == 1)
		mjpeg_error_exit1("Color space '%s' has alpha channel",
				lqt_colormodel_to_string(cmodel));

	y4m_accept_extensions(1);

	y4m_init_stream_info(&ostream);
	y4m_init_frame_info(&oframe);

	y4m_si_set_width(&ostream, width);
	y4m_si_set_height(&ostream, height);

/*
 * Now to map the BC_* values to Y4M_CHROMA_* values.  The 16bit types will
 * hopefully be converted to 8 bit in the decoding.
*/
	switch	(cmodel)
		{
		case	BC_YUV411P:
			y4mchroma = Y4M_CHROMA_411;
			break;
		case	BC_YUV422:
		case	BC_YUV422P16:
		case	BC_YUVJ422P:			/* should never see */
		case	BC_YUV422P:
			/* set color model to 422P to get 8bit planar output */
			lqt_set_cmodel(file, vtrack, BC_YUV422P);
			omodel = BC_YUV422P;
			y4mchroma = Y4M_CHROMA_422;
			break;
		case	BC_YUV420P:
		case	BC_YUVJ420P:
			y4mchroma = Y4M_CHROMA_420JPEG;
#if 0
			if	(lqt_get_chroma_placement(file, vtrack) == 
					LQT_CHROMA_PLACEMENT_DVPAL)
				y4mchroma = Y4M_CHROMA_PALDV;
#endif
			break;
		case	BC_YUV444P16:
		case	BC_YUV444P:
		case	BC_YUVJ444P:			/* should never see */
			/* set color model to 444P to get 8bit planar output */
			lqt_set_cmodel(file, vtrack, BC_YUV444P);
			omodel = BC_YUV444P;
			y4mchroma = Y4M_CHROMA_444;
			break;
		default:
			mjpeg_error_exit1("Unknown/unsupported color model %s",
				lqt_colormodel_to_string(cmodel));
		}

/*
 * Report unsupported conversions.  This should never happen unless a new
 * colormodel is added to the switch statement above.
*/
	if	(omodel && !lqt_colormodel_has_conversion(cmodel, omodel))
		mjpeg_error_exit1("libquicktime can't convert from %s to %s :(",
			lqt_colormodel_to_string(cmodel),
			lqt_colormodel_to_string(omodel));

	lqt_colormodel_get_chroma_sub(cmodel, &ss_h, &ss_v);
	y4m_si_set_chroma(&ostream, y4mchroma);

/*
 * If interlacing was specified on the commandline use it (override/ignore
 * any 'fiel' atom in the file).  Otherwise use the 'fiel' atom if present.
 * If no command line value and no 'fiel' atom then use 'progressive' and 
 * log a warning.
*/
	if	(interlace == Y4M_UNKNOWN)
		{
		lqt_get_fiel(file, vtrack, &nfields, &dominance);
		if	(nfields == 0)
			{
			mjpeg_warn("no 'fiel' atom and no -i option, assuming PROGRESSIVE");
			interlace = Y4M_ILACE_NONE;
			}
		else if	(nfields == 1)
			interlace = Y4M_ILACE_NONE;
		else if	(nfields == 2)
			{
			if	(dominance == 14 || dominance == 6)
				interlace = Y4M_ILACE_BOTTOM_FIRST;
			else if	(dominance == 9 || dominance == 1)
				interlace = Y4M_ILACE_TOP_FIRST;
			else
				mjpeg_error_exit1("Unknown fiel dominance %d",
					dominance);
			}
		else if (nfields > 2)
			mjpeg_error_exit1("Bad nfields '%d' in 'fiel' atom",
				nfields);
		}
	y4m_si_set_interlace(&ostream, interlace);

/*
 * If a command line (-a) value given then use the command line value (ignore/
 * override the 'pasp' atom if present).  If no commandline (-a) option given
 * but there is a 'pasp' atom then use the latter.  If neither 'pasp' or
 * command line option then assume 1:1 pixels with a warning.
*/
	if	(Y4M_RATIO_EQL(sar, y4m_sar_UNKNOWN))
		{
		memset(&pasp, 0, sizeof (pasp));
		lqt_get_pasp(file, vtrack, &pasp);
		if	(pasp.hSpacing && pasp.vSpacing)
			{
			sar.n = pasp.hSpacing;
			sar.d = pasp.vSpacing;
			}
		else
			{
			mjpeg_warn("Missing/malformed 'pasp' atom, -a not specified, 1:1 SAR assumed.");
			sar = y4m_sar_SQUARE;
			}
		}
	y4m_ratio_reduce(&sar);
	y4m_si_set_sampleaspect(&ostream, sar);

	if	(Y4M_RATIO_EQL(rate, y4m_fps_UNKNOWN))
		{
		qtrate = quicktime_frame_rate(file, vtrack);
		if	(qtrate == 0.0)
			mjpeg_error_exit1("frame rate = 0 and no -r given");
		rate = mpeg_conform_framerate(qtrate);
		}
	y4m_ratio_reduce(&rate);
	y4m_si_set_framerate(&ostream, rate);

/*
 * Everything needed for the Y4M header has been obtained - so now write
 * out the YUV4MPEG2 header
*/
	y4m_write_stream_header(fileno(stdout), &ostream);

/*
 * Now allocate the memory for the buffers.
*/
	qtrows = yuv;
	yuv[0] = (uint8_t *) malloc(width * height);
	yuv[1] = (uint8_t *) malloc((width / ss_h) * (height / ss_v));
	yuv[2] = (uint8_t *) malloc((width / ss_h) * (height / ss_v));

	for	(i = 0; i < length; i++)
		{
		lqt_decode_video(file, qtrows, vtrack);
		y4m_write_frame(fileno(stdout), &ostream, &oframe, yuv);
		}

/*
 * Now do the audio if requested and the audio file open succeeded earlier
*/
doaudio:
	if	(audio_fp)
		do_audio(file, audio_fp, atrack);
	quicktime_close(file);
        exit(0);
	}

static void 
do_audio(quicktime_t *qtf, FILE *audio_fp, int atrack)
	{
	long total_samples_decoded = 0;
	long audio_rate, l;
	int  audio_sample_size, channels, samples_decoded, i, bigendian;
        int16_t **qt_audion, *audbuf, *swapbuf = NULL;
	WAVheader wavhead;

	bigendian = lav_detect_endian();

	audio_rate = quicktime_sample_rate(qtf, atrack);
	audio_sample_size = quicktime_audio_bits(qtf, atrack) / 8;
	/* if audio_sample_size != 2 then bail? */

	channels = quicktime_track_channels(qtf, atrack);

	audbuf = (int16_t*)malloc(channels * audio_sample_size * nSAMPS);
	if	(bigendian != 0)
		swapbuf = (int16_t*)malloc(channels*audio_sample_size*nSAMPS);

	qt_audion = malloc(channels * sizeof (int16_t **));
	for	(i = 0; i < channels; i++)
		qt_audion[i] = (int16_t *)malloc(nSAMPS * audio_sample_size);

/* 
 * Need to generate prototype WAV header and write it out - at the end of the
 * input data a rewind will be done and the WAV header rewritten
*/
	memcpy(wavhead.riff, "RIFF", 4);
	wavhead.riff_length = sizeof(wavhead) - sizeof(wavhead.riff) - 
				sizeof(wavhead.riff_length);
	wavhead.data_length = 0;
	memcpy(wavhead.type, "WAVE", 4);
	memcpy(wavhead.format, "fmt ", 4);
	wavhead.format_length = 0x10;
	wavhead.tag = 0x01;	/* PCM */
	wavhead.channels = channels;
	wavhead.rate = audio_rate;
	wavhead.bytespersecond = audio_rate * channels * audio_sample_size ;
	wavhead.bytespersample = audio_sample_size * channels;
	wavhead.bitspersample = audio_sample_size * 8;
	memcpy(wavhead.data, "data", 4);

	fwrite(&wavhead, sizeof (wavhead), 1, audio_fp);
	mjpeg_info("channels: %d audio_rate: %ld audio_sample_size: %d\n", channels, audio_rate, audio_sample_size);

	while	(1)
		{
   		int64_t last_pos, start_pos;
   		int i, j;

		start_pos = lqt_last_audio_position(qtf, atrack);
		lqt_decode_audio_track(qtf, qt_audion, NULL, nSAMPS, atrack);
		last_pos = lqt_last_audio_position(qtf, atrack);
		samples_decoded = last_pos - start_pos;
/* Interleave the channels of audio into the one buffer provided */
		for	(i = 0; i < samples_decoded; i++)
			{
			for	(j = 0; j < channels; j++)
				audbuf[(channels * i) + j] = qt_audion[j][i];
			}
		l = samples_decoded * audio_sample_size * channels;
		wavhead.data_length += l;
		wavhead.riff_length += l;

		if	(bigendian)
			swab(audbuf, swapbuf, l);
		fwrite(bigendian ? swapbuf : audbuf, l, 1, audio_fp);
		total_samples_decoded += samples_decoded;
		if	(samples_decoded < nSAMPS)
			break;
		}
	rewind(audio_fp);
	fixwavheader(&wavhead, bigendian);
	fwrite(&wavhead, sizeof (wavhead), 1, audio_fp);
	mjpeg_info("total samples decoded: %ld\n", total_samples_decoded);
	fclose(audio_fp);
	}

static void fixwavheader(WAVheader *wave, int bigendian)
	{
	uint16_t temp_16;
	uint32_t temp_32;
	WAVheader saved;

	if	(bigendian == 0)
		return;

	memcpy(&saved, wave, sizeof (*wave));

	temp_32 = saved.riff_length;
	wave->riff_length = reorder_32(temp_32, bigendian);

	temp_32 = saved.format_length;
	wave->format_length = reorder_32(temp_32, bigendian);

	temp_16 = saved.tag;
	swab(&temp_16, &wave->tag, 2);

	temp_16 = saved.channels;
	swab(&temp_16, &wave->channels, 2);

	temp_32 = saved.rate;
	wave->rate = reorder_32(temp_32, bigendian);

	temp_32 = saved.bytespersecond;
        wave->bytespersecond = reorder_32(temp_32, bigendian);

	temp_16 = saved.bytespersample;
	swab(&temp_16, &wave->bytespersample, 2);

	temp_16 = saved.bitspersample;
	swab(&temp_16, &wave->bitspersample, 2);

	temp_32 = saved.data_length;
	wave->data_length = reorder_32(temp_32, bigendian);
	}
