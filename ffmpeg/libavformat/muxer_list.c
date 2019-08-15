static const AVOutputFormat * const muxer_list[] = {
    &ff_mlp_muxer,
    &ff_null_muxer,
    &ff_pcm_s32le_muxer,
    &ff_pcm_s24le_muxer,
    &ff_pcm_s16le_muxer,
    &ff_truehd_muxer,
    &ff_wav_muxer,
    NULL };
