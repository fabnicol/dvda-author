#include "mlp.h"

static  const uint8_t default_cga[6] = {0,  1,  7,  3,   9,   12};  //default channel assignment
static uint32_t cga2wav_channels[21] = {0x4, 0x3, 0x103, 0x33, 0xB, 0x10B, 0x3B, 0x7, 0x107, 0x37, 0xF, 0x10F, 0x3F, 0x107, 0x37, 0xF, 0x10F, 0x3F, 0x3B, 0x37, 0x3B };

int decode_mlp_file(fileinfo_t* info, globalData* globals)
{
    // initialize all muxers, demuxers and protocols for libavformat
    // (does nothing if called twice during the course of one program execution)

  av_register_all();

  const char* path = info->filename;

    // get format from audio file
  AVFormatContext* format = avformat_alloc_context();

  if (avformat_open_input(&format, path, NULL, NULL) != 0)
  {
        fprintf(stderr, ERR "Could not open file '%s'\n", path);
        EXIT_ON_RUNTIME_ERROR
  }

  if (avformat_find_stream_info(format, NULL) < 0)
  {
        fprintf(stderr, ERR "Could not retrieve stream info from file '%s'\n", path);
        EXIT_ON_RUNTIME_ERROR
  }
   errno = 0;  // hushes up an ioctl report
 // Find the index of the first audio stream

  int stream_index = -1;
  for (int i = 0; i < format->nb_streams; ++i)
  {
      if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
      {
          stream_index = i;
          break;
      }
  }

  if (stream_index == -1)
  {
        fprintf(stderr, ERR "Could not retrieve audio stream from file '%s'\n", path);
        EXIT_ON_RUNTIME_ERROR
  }

  AVStream* stream = format->streams[stream_index];

  // find & open codec

  AVCodecContext* codec = stream->codec;

  codec->request_sample_fmt = av_get_alt_sample_fmt(codec->sample_fmt, 0);

  if (avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), NULL) < 0)
    {
        fprintf(stderr, ERR "Failed to open decoder for stream #%u in file '%s'\n", stream_index, path);
        EXIT_ON_RUNTIME_ERROR
    }

    // prepare to read data
  AVPacket packet;

  av_init_packet(&packet);

  AVFrame* frame = av_frame_alloc();

  if (!frame)
    {
        fprintf(stderr, ERR "Error allocating the frame\n");
        EXIT_ON_RUNTIME_ERROR
    }

  FILE* fp = NULL;
  WaveData   info2;
  WaveHeader header;

 int32_t cumbytes_written = 0;

 if (globals->decode)
 {
   int32_t bytes_written = 0;
   errno = 0;
   while (av_read_frame(format, &packet) >= 0)
    {
        // decode one frame
        int gotFrame;
        int ret;
        if ((ret = avcodec_decode_audio4(codec, frame, &gotFrame, &packet)) < 0)
        {
            fprintf(stderr, ERR "Error decoding audio frame (%s)\n", av_err2str(ret));
            errno = 0;
            if (fp) fclose(fp);
            if (errno)
                fprintf(stderr, ERR "Could not close file %s\n", info->out_filename);
            else
                fprintf(stderr, MSG_TAG "Extracted %ld bytes from file %s to file %s\n", cumbytes_written, path, info->out_filename);
            break;
        }

        if (gotFrame)
        {

            if (globals->decode && bytes_written == 0)
            {
              if (info->out_filename && globals->fixwav_prepend)
              {
                    fprintf(stderr, INF "Decoding file %s  to raw data in path: %s\n", info->filename, info->out_filename);
                    int debug = globals->debugging;

                    // Hush it up as there will be spurious error mmsg

                    globals->debugging = false;

                    // initializ

                    memset(&info2, 0, sizeof (WaveData));
                    memset(&header, 0, sizeof (WaveHeader));

                    // WaveData info: file and header prepending characteristics

                    info2.prepend = true;
                    info2.in_place = true;
                    info2.cautious = false;
                    info2.automatic = true;
                    info2.interactive = false;
                    info2.prunedbytes = 0;
                    info2.padbytes = 0;
                    info2.virtual = false;
                    info2.outfile.filename = info->out_filename;
                    info2.outfile.fp = fp;
                    info2.infile = info2.outfile;
                    info2.infile.isopen = false;

                    // WaveHeader info : audio and header characteristics


                    header.header_size_out = 80;
                    header.channels = frame->channels;
                    header.is_extensible = header.channels > 2;
                    header.nBlockAlign = frame->channels * codec->bits_per_raw_sample / 8 ;
                    header.wBitsPerSample = codec->bits_per_raw_sample;
                    header.dwChannelMask = (frame->channel_layout < 21 && frame->channel_layout > 0) ? cga2wav_channels[frame->channel_layout] : 0;
                    header.dwSamplesPerSec = frame->sample_rate;

                    // Prepend header to empty file. Will close files.

                    fixwav(&info2, &header, globals);

                    // Restore verbosity levels

                    globals->debugging = debug;


              }

              fp = fopen(info->out_filename, "ab+");
              if (fp == NULL)
               {
                   fprintf(stderr, ERR "Could not open destination file %s\n", info->out_filename);
                   info->out_filename = NULL;
                   globals->decode = false;
               }

            }

            if (globals->decode && fp)
            {
                size_t unpadded_linesize = 0;

                int sampleSize = av_get_bytes_per_sample(codec->sample_fmt);

                if (sampleSize == 2)
                {
                    unpadded_linesize = frame->channels *  sampleSize * frame->nb_samples;
                    bytes_written = fwrite(frame->extended_data[0], 1, unpadded_linesize, fp);
                }
                else
                if (sampleSize == 4)    // 32 bits -> 24 bits (codec->bits_per_raw_sample)
                {

                  uint32_t bytes_written_sample = 0;
                   for(int s = 0; s < frame->nb_samples; ++s)
                   {
                    for(int c = 0; c < codec->channels; ++c)
                     {
                        uint32_t val = ((int32_t*) frame->extended_data[0])[s * codec->channels + c];
                        val  >>= 8; // sampleSize * 8 - codec->bits_per_raw_sample

                       bytes_written_sample += fwrite(&val, 1, 3, fp);
                     }
                   }

                   bytes_written = bytes_written_sample;
                }
                else
                {
                        fprintf(stderr, "Invalid sample size %d.\n", sampleSize);
                        fclose(fp);
                        break;
                }

            }

            cumbytes_written += bytes_written;

            if (globals->maxverbose)
            {
                    fprintf(stderr, "Bytes_written: %d Nb samples: %d FR_PTS: %ld PKT_POS: %ld PKT_DURATION: %ld FR_PKT_SIZE; %ld\n",
                                cumbytes_written,
                                frame->nb_samples,
                                frame->pts,
                                frame->pkt_pos,
                                frame->pkt_duration,
                                frame->pkt_size);
            }

        }
        else
        {
           continue;
        }
    }

    if (globals->decode && info->out_filename && globals->fixwav_prepend)
    {
//         WAV output is now OK except for the wav file size-based header data.
//         ckSize, data_ckSize and nBlockAlign must be readjusted by computing
//         the exact audio content bytesize. Also we no longer prepend the header
//         but overwrite the existing one

        fp = fopen(info->out_filename, "rb+");
        if (fp == NULL)
         {
             fprintf(stderr, ERR "Could not open destination file %s\n", info->out_filename);
             info->out_filename = NULL;
             globals->decode = false;
         }
         else
         {
            info2.infile.fp = fp;
            info2.infile.isopen = true;

            // No longer prepend to empty file but overwrite in place

            info2.prepend = false;
            info2.in_place = true;

            errno = 0;

            fixwav(&info2, &header, globals);

            if (errno)
                fprintf(stderr, ERR "Error detected in header correction  for %s\n", info->out_filename);
            else
                fprintf(stderr, MSG_TAG "Extracted %ld bytes to file %s\n", cumbytes_written, info->out_filename);
         }
    }
 }
 else
 {
    uint32_t rank = 0;
    uint32_t totnbsamples = 0;
    unsigned long PKT_POS_SECT = 0;
    unsigned long HEADER_OFFSET = 0;
    unsigned long SECT_RANK = 0;
    unsigned long SECT_RANK_OLD = 0;

    if (info->file_size && info->lpcm_payload)
    {
        uint32_t size = info->file_size / info->lpcm_payload + 4;
        info->mlp_layout = (struct MLP_LAYOUT *) calloc(size, sizeof (struct MLP_LAYOUT));
    }
    else
    {
        goto clean_up; // do sth more verbose
    }

    while (av_read_frame(format, &packet) >= 0)
    {
      // decode one frame
      int gotFrame;
      if (avcodec_decode_audio4(codec, frame, &gotFrame, &packet) < 0)
      {
           break;
      }

      if (gotFrame)
      {
              if (HEADER_OFFSET == 0) HEADER_OFFSET = 64;

              PKT_POS_SECT = frame->pkt_pos + HEADER_OFFSET;

              SECT_RANK_OLD = SECT_RANK;
              SECT_RANK = (PKT_POS_SECT - 1)/ 2048;

              bool new_sector = (SECT_RANK != SECT_RANK_OLD);

              if (new_sector || rank == 0)
              {
                    if (new_sector) HEADER_OFFSET += 43;
                    info->mlp_layout[rank].pkt_pos = frame->pkt_pos;
                    info->mlp_layout[rank].nb_samples = totnbsamples;
                    info->mlp_layout[rank].rank = SECT_RANK;

                    ++rank;
              }

              totnbsamples += frame->nb_samples;

              if (globals->maxverbose)
              {
                  fprintf(stderr, "Sect: %lu samples_written: %d Nb samples: %d FR_PTS: %ld PKT_POS: %ld PKT_DURATION: %ld FR_PKT_SIZE; %ld\n",
                              SECT_RANK,
                              totnbsamples,
                              frame->nb_samples,
                              frame->pts,
                              frame->pkt_pos,
                              frame->pkt_duration,
                              frame->pkt_size);
              }
      }
      else
      {
          continue;
      }

    }

    info->mlp_layout[rank].pkt_pos = frame->pkt_pos;
    info->mlp_layout[rank].nb_samples = totnbsamples;
    info->mlp_layout[rank].rank = SECT_RANK;
    info->mlp_layout_size = rank + 1;

    if (globals->maxverbose)
    {
        fprintf(stderr, "** \n Layout size: %d **\n", info->mlp_layout_size);
    }
  }

clean_up:

  av_frame_free(&frame);
  avcodec_close(codec);
  avformat_free_context(format);

  return errno;
}



/* check that a given sample format is supported by the encoder */

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

#if 0


#define AV_CH_FRONT_LEFT             0x00000001
#define AV_CH_FRONT_RIGHT            0x00000002
#define AV_CH_FRONT_CENTER           0x00000004
#define AV_CH_LOW_FREQUENCY          0x00000008
#define AV_CH_BACK_LEFT              0x00000010
#define AV_CH_BACK_RIGHT             0x00000020
#define AV_CH_FRONT_LEFT_OF_CENTER   0x00000040
#define AV_CH_FRONT_RIGHT_OF_CENTER  0x00000080
#define AV_CH_BACK_CENTER            0x00000100
#define AV_CH_SIDE_LEFT              0x00000200
#define AV_CH_SIDE_RIGHT             0x00000400
#define AV_CH_TOP_CENTER             0x00000800
#define AV_CH_TOP_FRONT_LEFT         0x00001000
#define AV_CH_TOP_FRONT_CENTER       0x00002000
#define AV_CH_TOP_FRONT_RIGHT        0x00004000
#define AV_CH_TOP_BACK_LEFT          0x00008000
#define AV_CH_TOP_BACK_CENTER        0x00010000
#define AV_CH_TOP_BACK_RIGHT         0x00020000
#define AV_CH_STEREO_LEFT            0x20000000  ///< Stereo downmix.
#define AV_CH_STEREO_RIGHT           0x40000000  ///< See AV_CH_STEREO_LEFT.
#define AV_CH_WIDE_LEFT              0x0000000080000000ULL
#define AV_CH_WIDE_RIGHT             0x0000000100000000ULL
#define AV_CH_SURROUND_DIRECT_LEFT   0x0000000200000000ULL
#define AV_CH_SURROUND_DIRECT_RIGHT  0x0000000400000000ULL
#define AV_CH_LOW_FREQUENCY_2        0x0000000800000000ULL
#define AV_CH_TOP_SIDE_LEFT          0x0000001000000000ULL
#define AV_CH_TOP_SIDE_RIGHT         0x0000002000000000ULL
#define AV_CH_BOTTOM_FRONT_CENTER    0x0000004000000000ULL
#define AV_CH_BOTTOM_FRONT_LEFT      0x0000008000000000ULL
#define AV_CH_BOTTOM_FRONT_RIGHT     0x0000010000000000ULL
#define AV_CH_LAYOUT_MONO              (AV_CH_FRONT_CENTER)
#define AV_CH_LAYOUT_STEREO            (AV_CH_FRONT_LEFT|AV_CH_FRONT_RIGHT)
#define AV_CH_LAYOUT_2POINT1           (AV_CH_LAYOUT_STEREO|AV_CH_LOW_FREQUENCY)
#define AV_CH_LAYOUT_2_1               (AV_CH_LAYOUT_STEREO|AV_CH_BACK_CENTER)
#define AV_CH_LAYOUT_SURROUND          (AV_CH_LAYOUT_STEREO|AV_CH_FRONT_CENTER)
#define AV_CH_LAYOUT_3POINT1           (AV_CH_LAYOUT_SURROUND|AV_CH_LOW_FREQUENCY)
#define AV_CH_LAYOUT_4POINT0           (AV_CH_LAYOUT_SURROUND|AV_CH_BACK_CENTER)
#define AV_CH_LAYOUT_4POINT1           (AV_CH_LAYOUT_4POINT0|AV_CH_LOW_FREQUENCY)
#define AV_CH_LAYOUT_2_2               (AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT)
#define AV_CH_LAYOUT_QUAD              (AV_CH_LAYOUT_STEREO|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV_CH_LAYOUT_5POINT0           (AV_CH_LAYOUT_SURROUND|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT)
#define AV_CH_LAYOUT_5POINT1           (AV_CH_LAYOUT_5POINT0|AV_CH_LOW_FREQUENCY)
#define AV_CH_LAYOUT_5POINT0_BACK      (AV_CH_LAYOUT_SURROUND|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV_CH_LAYOUT_5POINT1_BACK      (AV_CH_LAYOUT_5POINT0_BACK|AV_CH_LOW_FREQUENCY)


for MLP these are the layouts allowed by mlp.c:58

    AV_CH_LAYOUT_MONO            0x00000004  SPEAKER_FRONT_CENTER
    AV_CH_LAYOUT_STEREO          0x00000003  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT
    AV_CH_LAYOUT_2_1             0x00000103  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER
    AV_CH_LAYOUT_QUAD            0x00000033  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
    AV_CH_LAYOUT_2POINT1         0x0000000B  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY
    AV_CH_LAYOUT_SURROUND        0x00000007  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
    AV_CH_LAYOUT_4POINT0         0x00000107  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER
    AV_CH_LAYOUT_5POINT0_BACK    0x00000037  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
    AV_CH_LAYOUT_3POINT1         0x0000000F  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY
    AV_CH_LAYOUT_4POINT1         0x0000010F  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY
    AV_CH_LAYOUT_5POINT1_BACK    0x0000003F  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY

which can be rewrittent without deps fo ffmpeg and keeping to MS defines as follows:

# define SPEAKER_FRONT_LEFT             0x1
# define SPEAKER_FRONT_RIGHT            0x2
# define SPEAKER_FRONT_CENTER           0x4
# define SPEAKER_LOW_FREQUENCY          0x8
# define SPEAKER_BACK_LEFT              0x10
# define SPEAKER_BACK_RIGHT             0x20
# define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
# define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
# define SPEAKER_BACK_CENTER            0x100
# define SPEAKER_SIDE_LEFT              0x200
# define SPEAKER_SIDE_RIGHT             0x400
# define SPEAKER_TOP_CENTER             0x800
# define SPEAKER_TOP_FRONT_LEFT         0x1000
# define SPEAKER_TOP_FRONT_CENTER       0x2000
# define SPEAKER_TOP_FRONT_RIGHT        0x4000
# define SPEAKER_TOP_BACK_LEFT          0x8000
# define SPEAKER_TOP_BACK_CENTER        0x10000
# define SPEAKER_TOP_BACK_RIGHT         0x20000
# define SPEAKER_RESERVED               0x80000000

#endif // 0

static int  channels2layout[6] = { SPEAKER_FRONT_CENTER,
                                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
                                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
                                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
                                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
                                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY
                                 };

inline static int select_channel_layout(fileinfo_t* info, globalData* globals)
{
    switch(info->dw_channel_mask)
    {
         // do nothing
         case  SPEAKER_FRONT_CENTER:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY:
         case  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY:
               return (info->dw_channel_mask);
         default:
               if (info->channels > 0)
                  return channels2layout[info->channels - 1];  // reassign
               else
               {
                  foutput(ERR "File %s has neither channels or channel layout.", info->filename);
                  EXITING
               }
    }

  return 0;
}

static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output, globalData* globals)
{
    int ret;

    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("Error sending the frame to the encoder\n");
    }

    /* read all the available output packets (in general there may be any
     * number of them */

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE("Error encoding audio frame\n");
        }

        fwrite(pkt->data, 1, pkt->size, output);
        av_packet_unref(pkt);
    }
}


inline static uint64_t encode_fmt_s32(AVCodecContext* c, AVFrame* frame, AVPacket *pkt, FILE* in_fp, FILE* out_fp, globalData* globals)
{
        int load = c->channels * 4 * c->frame_size;

        uint16_t samples[load];
        memset(samples, 0, load);

        int res = -1, bytes_read_per_sample = -1, bytes_read_per_frame = -1;
        static uint64_t bytes_read;

        for (int s = 0; res != 0 && s < c->frame_size; ++s)
        {
           for (int ch = 0; res != 0 && ch < c->channels; ++ch)
           {

             res = fread(&samples[(s * c->channels + ch)* 4 + 1], 1, 3, in_fp);

             bytes_read_per_sample += res;
           }

           bytes_read_per_frame += bytes_read_per_sample;

           if (globals->maxverbose)
            foutput(MSG_TAG "Bytes read per sample: %d Bytes read per frame: %d\n",
                                                  bytes_read_per_sample,
                                                  bytes_read_per_frame);
        }

        memcpy((uint16_t*) frame->data[0], samples, load);

        if (bytes_read_per_frame > 0)
        {
           encode(c, frame, pkt, out_fp, globals);
           bytes_read += bytes_read_per_frame;
        }

        return bytes_read;
}

inline static uint64_t encode_fmt_s16(AVCodecContext* c, AVFrame* frame, AVPacket *pkt, FILE* in_fp, FILE* out_fp, globalData* globals)
{
        int load = c->channels * 2 * c->frame_size;

        uint16_t * samples = (uint16_t*) frame->data[0];
        static uint64_t bytes_read;
        uint16_t bytes_read_per_frame;

        bytes_read_per_frame = fread(samples, 1, load, in_fp);

        if (bytes_read_per_frame)
        {
            encode(c, frame, pkt, out_fp, globals);

           if (globals->maxverbose)
            foutput(MSG_TAG "Bytes read per frame: %d\n", bytes_read_per_frame);
        }

        bytes_read += bytes_read_per_frame;

        return bytes_read;
}

int encode_mlp_file(fileinfo_t* info, globalData* globals)
{
    // initialize all muxers, demuxers and protocols for libavformat
    // (does nothing if called twice during the course of one program execution)

    if (info->bitspersample == 24)
    {
        foutput(WAR "Currently 24-bit MLP support is still under development. Balking at concerting file %s\n", info->filename);
        return 0;
    }

    if (globals->debugging)
        foutput(INF "Encoding %s to MLP\n", info->filename);

    av_register_all();

    const AVCodec *codec;
    AVCodecContext *c= NULL;
    AVFrame *frame;
    AVPacket *pkt;
    int ret;
    uint16_t *samples;
    int32_t bytes_written = 0;

    errno = 0;

    codec = avcodec_find_encoder(AV_CODEC_ID_MLP);
    if (! codec)
     {
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "MLP codec not found\n");
     }

    c = avcodec_alloc_context3(codec);
    if (!c)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not allocate audio codec context\n");
    }

    /* put sample parameters */

    c->sample_fmt = info->bitspersample == 16 ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_S32;

    if (!check_sample_fmt(codec, c->sample_fmt))
    {
        foutput(ERR "Encoder does not support sample format %s",
                  av_get_sample_fmt_name(c->sample_fmt));
        EXITING
    }

    c->sample_rate    = info->samplerate;
    c->channel_layout = select_channel_layout(info, globals);
    c->channels       = info->channels;
    c->bits_per_coded_sample = info->bitspersample;

    /* open it */

    c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL; // allow experimental codecs

    if (avcodec_open2(c, codec, NULL) < 0)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not open codec\n")
    }

    FILE* in_fp = fopen(strdup(info->filename), "rb");

    path_t* fn = parse_filepath(info->filename, globals);

    char* out_filename = (char*) calloc(strlen(globals->settings.tempdir) + 1 + strlen(fn->rawfilename) + 12+ 1, sizeof(char));

    sprintf(out_filename, "%s%s%s%s", globals->settings.tempdir, SEPARATOR, fn->rawfilename, "_enc_wav.mlp");

    free_filepath(fn);

    FILE*  out_fp   = fopen(strdup(out_filename), "wb");

    if (! in_fp)
    {
        foutput("Could not read %s\n", info->filename);
        EXITING
    }

    if (! out_fp)
    {
        foutput("Could not write to %s\n", out_filename);
        EXITING
    }

    // packet for holding encoded output

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "could not allocate the packet\n");
        exit(1);
    }

    // frame containing input raw audio

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    frame->nb_samples     = c->frame_size;
    frame->format         = c->sample_fmt;
    frame->channel_layout = c->channel_layout;

    // allocate the data buffers

    int res = av_frame_get_buffer(frame, 0);
    if (res < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(2);
    }

    uint64_t bytes_read;

    while(! feof(in_fp))
    {
        res = av_frame_make_writable(frame);
        if (res < 0)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE("FFMPEG Frame not writable in encoding loop")
        }

        if (c->sample_fmt == AV_SAMPLE_FMT_S16)

            bytes_read = encode_fmt_s16(c, frame, pkt, in_fp, out_fp, globals);
        else
            bytes_read = encode_fmt_s32(c, frame, pkt, in_fp, out_fp, globals);
    }

    if (bytes_read == info->file_size)
        if (globals->debugging) foutput(MSG_TAG "File %s was entirely read (%llu B)", info->filename, info->file_size);
    else
        foutput(MSG_TAG "File %s was not entirely read (%llu B / %llu B)", info->filename, bytes_read, info->file_size);

    // flush the encoder

    encode(c, NULL, pkt, out_fp, globals);

    fclose(out_fp);
    fclose(in_fp);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);

    info->mlp_filename = out_filename;

    if (errno == 0 && globals->debugging)
    {
        foutput(MSG_TAG "MLP encoding performed:\n     %s --> %s\n", info->filename, out_filename);
    }

  return errno;
}


void transport_to_mlp(fileinfo_t* info, globalData* globals)
{

     info->filename = strdup(info->mlp_filename);
     if (info->filename == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE("Could not copy MLP filename.")

     int res = audiofile_getinfo(info, globals);

     if (res == AFMT_MLP)
     {
          if (globals->debugging)
            foutput(MSG_TAG "Found MLP format for %s\n", info->filename);

         info->type = AFMT_MLP;
     }
     else
     {
         foutput(MSG_TAG "Unfortunately did not find MLP format for file %s\n", info->filename);
         EXITING
     }
}

