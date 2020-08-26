static const AVInputFormat * const demuxer_list[] = {
    &ff_mlp_demuxer,
    &ff_pcm_s32le_demuxer,
    &ff_pcm_s24le_demuxer,
    &ff_pcm_s16le_demuxer,
    &ff_truehd_demuxer,
    &ff_wav_demuxer,
    NULL };
