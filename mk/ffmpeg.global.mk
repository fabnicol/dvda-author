ffmpeg_LIB=/home/fab/dvda-author/local/lib/libffmpeg.a
ffmpeg_LINK=
MAYBE_ffmpeg=ffmpeg-4.2.4
HAVE_ffmpeg=no
HAVE_EXTERNAL_ffmpeg=@HAVE_EXTERNAL_ffmpeg@
CONFIGURE_ffmpeg_FLAGS=--prefix=/home/fab/dvda-author/local--disable-demuxers --disable-decoders --disable-muxers --disable-parsers --disable-encoders --disable-devices --disable-protocols --enable-protocol=file --enable-protocol=data --disable-bsfs --disable-hwaccels  --disable-filters   --enable-decoder=mlp --enable-encoder=mlp --enable-encoder=pcm_s16le --enable-encoder=pcm_s24le --enable-encoder=pcm_s32le --enable-decoder=pcm_s16le --enable-decoder=pcm_s24le --enable-decoder=pcm_s24le  --enable-parser=mlp --enable-muxer=wav --enable-muxer=null --enable-muxer=truehd --enable-muxer=mlp --enable-demuxer=mlp --enable-muxer=pcm_s16le --enable-muxer=pcm_s24le --enable-muxer=pcm_s32le --enable-demuxer=pcm_s16le --enable-demuxer=pcm_s24le --enable-demuxer=pcm_s32le  --enable-filter=aresample --disable-bzlib --disable-iconv --disable-libxcb --disable-libxcb-shm --disable-libxcb-xfixes --disable-libxcb-shape --disable-sndio --disable-sdl2 --disable-zlib --disable-xlib --disable-libdrm --disable-vaapi --disable-vdpau --disable-videotoolbox  --enable-static --disable-shared   --enable-demuxer=wav --enable-demuxer=truehd  --disable-swscale  --disable-network --disable-postproc --disable-pixelutils --disable-avdevice --disable-alsa --disable-lzma --disable-doc --disable-d3d11va --disable-amd3dnow --disable-amd3dnowext  --disable-dxva2 
ffmpeg_BUILD=yes
WITH_ffmpeg=@WITH_ffmpeg@
ffmpeg_DEPENDENCY=Makefile /home/fab/dvda-author/ffmpeg-4.2.4
