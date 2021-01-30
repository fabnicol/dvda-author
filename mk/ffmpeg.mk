include /home/fab/Documents/dvda-author/mk/ffmpeg.global.mk

ffmpeg_MAKESPEC=auto
ffmpeg_CONFIGSPEC=lib
ffmpeg_TARGETLIB=libavcodec.a

/home/fab/Documents/dvda-author/depconf/ffmpeg.depconf: $(ffmpeg_DEPENDENCY)
	$(call depconf,ffmpeg)

