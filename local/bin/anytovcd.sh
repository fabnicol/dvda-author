#!/bin/sh
#
#  anytovcd.sh
#
#  Copyright (C) 2004,2005 Nicolas Boos <nicolaxx@free.fr>
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

AUD_TRACK="1"
VOLUMES="1"
PREFIX_OUT="out"
VCD_TYPE="dvd"
DEC_TOOL="ffmpeg"
ENC_TOOL="mpeg2enc"
MATRICES="tmpgenc"
QUALITY="best"
CQMFILE="matrices.txt"
STATFILE="statfile.log"

BFR="bfr"
FFMPEG="ffmpeg"
MPEG2DEC="mpeg2dec"
MPEG2ENC="mpeg2enc"
MPLEX="mplex"
PGMTOY4M="pgmtoy4m"
SOX="sox"
TCCAT="tccat"
TCDEMUX="tcdemux"
TCPROBE="tcprobe"
TCREQUANT="tcrequant"
Y4MDENOISE="y4mdenoise"
Y4MSCALER="y4mscaler"
Y4MSPATIALFILTER="y4mspatialfilter"
Y4MUNSHARP="y4munsharp"
YUVDENOISE="yuvdenoise"
YUVFPS="yuvfps"
YUVMEDIANFILTER="yuvmedianfilter"

SCRIPT_NAME="anytovcd.sh"
SCRIPT_VERSION="13"


# custom quant. matrices
INTRA_KVCD="8,9,12,22,26,27,29,34,9,10,14,26,27,29,34,37,12,14,18,27,29,34,37,38,22,26,27,31,36,37,38,40,26,27,29,36,39,38,40,48,27,29,34,37,38,40,48,58,29,34,37,38,40,48,58,69,34,37,38,40,48,58,69,79"
INTER_KVCD="16,18,20,22,24,26,28,30,18,20,22,24,26,28,30,32,20,22,24,26,28,30,32,34,22,24,26,30,32,32,34,36,24,26,28,32,34,34,36,38,26,28,30,32,34,36,38,40,28,30,32,34,36,38,42,42,30,32,34,36,38,40,42,44"

INTRA_MVCD="8,10,16,20,25,28,33,44,10,10,14,26,27,29,34,47,16,14,18,27,29,34,37,48,20,26,27,31,36,37,38,50,25,27,29,32,37,40,48,58,28,33,36,42,45,50,58,68,33,37,39,44,48,56,66,79,37,39,45,48,56,66,79,83"
INTER_MVCD="16,18,20,22,23,27,32,40,18,20,22,24,26,32,36,42,20,22,24,26,33,38,42,44,22,24,26,34,38,42,44,46,23,26,33,38,42,44,46,48,27,32,38,42,44,46,48,50,32,36,42,44,46,48,50,52,40,42,44,46,48,50,52,58"

INTRA_TMPGENC="8,16,19,22,26,27,29,34,16,16,22,24,27,29,34,37,19,22,26,27,29,34,34,38,22,22,26,27,29,34,37,40,22,26,27,29,32,35,40,48,26,27,29,32,35,40,40,58,26,27,29,34,38,46,56,69,27,29,35,38,46,56,69,83"
INTER_TMPGENC="16,17,18,19,20,21,22,23,17,18,19,20,21,22,23,24,18,19,20,21,22,23,24,25,19,20,21,22,23,24,26,27,20,21,22,23,25,26,27,28,21,22,23,24,26,27,28,30,22,23,24,26,27,28,30,31,23,24,25,27,28,30,31,33"

matrix_copy ()
{
    # TODO: find less ugly func.
    echo "`echo "$1" | \
    awk -F, '{for (i=1; i<=NF; i++) {n=n$i","; z+=1; if (i==64) {z=0; sub(",$","",n)}; if (z==8) {z=0; n=n"\n"}}; print n}'`"
}

range_check ()
{
    # if input value < min. value
    if test "$1" -lt "$2"; then
        
	# = min. value
	echo "$2"

    # if input value > max. value
    elif test "$1" -gt "$3"; then
        
	# = max. value
	echo "$3"
    
    else
        
	echo "$1"
    
    fi
}

show_help ()
{
    echo "-a audio_track  audio track number (default = \"1\")"
    echo "-A N:D          force input pixel aspect ratio"
    echo "-b              blind mode (no video)"
    echo "-c              enable audio pass-through"
    echo "-d tool_name    video decoder tool"
    echo "                  avail. : ffmpeg (default) or mpeg2dec"
    echo "-D              deinterlace video (ffmpeg decoder only)"
    echo "-e tool_name    video encoder tool"
    echo "                  avail. : mpeg2enc (default) or ffmpeg (experimental)"
    echo "-i filename     input file"
    echo "-I flag_name    force input ilace flag"
    echo "                  avail. : none, top_first and bottom_first"
    echo "-J flag_name    force output ilace flag (if possible)"
    echo "                  avail. : none, top_first and bottom_first"
    echo "-f filter1[:filter1_level][,filter2:[filter2_level]]...  video filter(s)"
    echo "                  avail. filters :"
    echo "                  - hqdenoise (y4mdenoise)"
    echo "                  - mean      (yuvmedianfilter -f)"
    echo "                  - median    (yuvmedianfilter)"
    echo "                  - spatial   (y4mspatialfilter)"
    echo "                  - temporal  (yuvdenoise)"
    echo "                  - unsharp   (y4munsharp)"
    echo "                  avail. levels :"
    echo "                  - light     (for low noise source)"
    echo "                  - medium    (for middle noise)"
    echo "                  - heavy     (for high noise)"
    echo "-m              mute mode (no audio)        "
    echo "-n norm         output norm in case of input with non-standard framerate "
    echo "                  avail. norms : pal (default), ntsc or ntsc_film"
    echo "-o filename     output files prefix (default = \"out\")"
    echo "-p preset_name  output type (default = dvd)"
    echo "                  avail. : cvd, cvd_wide, dvd, dvd_wide, mvcd, svcd and vcd"
    echo "-q level        quality (default = best)"
    echo "                  avail. : best, good, fair"
    echo "-r              enable two passes encoding mode"
    echo "-R N:D          force input framerate"
    echo "-T length       force input length (minutes)"
    echo "-v              show script version"
    echo "-V              number of volumes (default = \"1\")"
}

show_error ()
{
    echo "**ERROR: [${SCRIPT_NAME}] $1"
}

show_info ()
{
    echo "   INFO: [${SCRIPT_NAME}] $1"
}

show_warn ()
{
    echo "++ WARN: [${SCRIPT_NAME}] $1"
}

test_bin ()
{
    if ! which ${1} >/dev/null; then

        show_error "${1} binary not present."
        exit 2

    fi
}

for BIN in ${FFMPEG} ${PGMTOY4M}; do

    test_bin ${BIN}

done

while getopts a:A:bcd:De:f:hi:I:J:mn:o:p:q:rR:T:vV: OPT
do
    case ${OPT} in
    a)    AUD_TRACK="${OPTARG}";;
    A)    VID_SAR_SRC="${OPTARG}";;
    b)    BLIND_MODE="1";;
    c)    ACOPY_MODE="1";;
    d)    DEC_TOOL="${OPTARG}";;
    D)    DEINT_MODE="1";;
    e)    ENC_TOOL="${OPTARG}";;
    h)    show_help;
          exit 0;;
    i)    VIDEO_SRC="${OPTARG}"; AUDIO_SRC="${VIDEO_SRC}";;
    I)    INTERLACING="${OPTARG}";;
    J)    INTERLACING_OUT="${OPTARG}";;
    f)    FILTERS_CHAIN="${OPTARG}";;
    m)    MUTE_MODE="1";;
    n)    OUT_NORM="${OPTARG}";;
    o)    PREFIX_OUT="${OPTARG}";;
    p)    VCD_TYPE="${OPTARG}";;
    q)    QUALITY="${OPTARG}";;
#   r)    ENC_TYPE="abr";;
    r)    REQUANT_MODE="1";;
    R)    VID_FPS_SRC="${OPTARG}";;
    T)    DURATION="${OPTARG}";;
    v)    echo "${SCRIPT_NAME} version ${SCRIPT_VERSION}"; exit 0;;
    V)    VOLUMES="${OPTARG}";;
    \?)   show_error "Unknown or uncomplete command line option.";
          show_error "Use -h to get help.";
          exit 2;;
    esac
done

if test "${VIDEO_SRC}" = "" || ! test -r "${VIDEO_SRC}"; then

    show_error "input file not specified or not present."
    show_error "Use -h to get help."
    exit 2

fi

FFMPEG_VERSION="`${FFMPEG} 2>&1 | awk '$4 == "build" {print $5}' | sed s/,// | head -1`"

AUD_TRACK="`range_check ${AUD_TRACK} 1 99`"
FFMPEG_AUD_TRACK="`${FFMPEG} -i \"${AUDIO_SRC}\" 2>&1 | awk '/Audio:/ {sub("^#","",$2); print $2}' | awk -F[ '{print $1}' | head -${AUD_TRACK} | tail -1`"

AUD_BITRATE_SRC="`${FFMPEG} -i "${AUDIO_SRC}" 2>&1 | awk '/Audio:/ {print $8}' | head -${AUD_TRACK} | tail -1`"
AUD_FMT_SRC="`${FFMPEG} -i "${AUDIO_SRC}" 2>&1 | awk '/Audio:/ {print $4}' | sed s/,// | head -${AUD_TRACK} | tail -1`"
AUD_FREQ_SRC="`${FFMPEG} -i "${AUDIO_SRC}" 2>&1 | awk '/Audio:/ {print $5}' | sed s/,// | head -${AUD_TRACK} | tail -1`"

Y4M_HEADER="`${FFMPEG} -i "${VIDEO_SRC}" -f yuv4mpegpipe -pix_fmt yuv420p -t 1 -y /dev/stdout 2>/dev/null | head -1`"

VID_FMT_SRC="`${FFMPEG} -i "${VIDEO_SRC}" 2>&1 | awk '/Video:/ {print $4}' | sed s/,// | head -1`"
VID_WIDTH_SRC="`echo ${Y4M_HEADER} | awk '{print $2}' | sed s/W//`"
VID_HEIGHT_SRC="`echo ${Y4M_HEADER} | awk '{print $3}' | sed s/H//`"
VID_SIZE_SRC="${VID_WIDTH_SRC}x${VID_HEIGHT_SRC}"

FF_DEC="${FFMPEG} -i \"${VIDEO_SRC}\""
FF_DEC_AFLAGS="-map ${FFMPEG_AUD_TRACK} -i \"${AUDIO_SRC}\""

# video deinterlacing
if test "${DEC_TOOL}" = "ffmpeg" && test "${DEINT_MODE}" = "1"; then

    INTERLACING="none"

    FF_DEC="${FF_DEC} -deinterlace"

fi

# input interlacing
if test "${INTERLACING}" = "none"; then

    VID_ILACE_SRC="p"

elif test "${INTERLACING}" = "top_first"; then

    VID_ILACE_SRC="t"

elif test "${INTERLACING}" = "bottom_first"; then

    VID_ILACE_SRC="b"

else

    VID_ILACE_SRC="`echo ${Y4M_HEADER} | awk '{print $5}' | cut -c 2-`"

fi

# output interlacing
if test "${INTERLACING_OUT}" = "none"; then

    VID_ILACE_OUT="p"

elif test "${INTERLACING_OUT}" = "top_first"; then

    VID_ILACE_OUT="t"

elif test "${INTERLACING_OUT}" = "bottom_first"; then

    VID_ILACE_OUT="b"

else

    VID_ILACE_OUT="${VID_ILACE_SRC}"

fi

# input framerate
if test "${VID_FPS_SRC}" = ""; then

    VID_FPS_SRC="`echo ${Y4M_HEADER} | awk '{print $4}' | cut -c 2-`"

fi

# input pixel/sample aspect ratio
if test "${VID_SAR_SRC}" = ""; then

    VID_SAR_SRC="`echo ${Y4M_HEADER} | awk '{print $6}' | cut -c 2-`"

fi

VID_SAR_N_SRC="`echo ${VID_SAR_SRC} | cut -d: -f1`"
VID_SAR_D_SRC="`echo ${VID_SAR_SRC} | cut -d: -f2`"

if test "${VID_SAR_N_SRC}" = "0" || test "${VID_SAR_D_SRC}" = "0"; then

    show_warn "Unknown pixel aspect ratio, defaulting to 1:1"
    VID_SAR_SRC="1:1"

fi

FF_ENC="${FFMPEG} -v 0 -f yuv4mpegpipe -i /dev/stdin -bf 2"
MPEG2ENC="${MPEG2ENC} -v 0 -M 0 -R 2 -s -2 1 -E -5"
MPLEX_FLAGS="-v 1"
PGMTOY4M="${PGMTOY4M} -r ${VID_FPS_SRC} -i ${VID_ILACE_SRC} -a ${VID_SAR_SRC}"
Y4MSCALER_FLAGS="-v 0 -S option=cubicCR -O infer=exact"

# ffmpeg decoder version
if test "${FFMPEG_VERSION}" -ge "4731"; then

    FF_DEC="${FF_DEC} -f image2pipe -vcodec pgmyuv"

else

    FF_DEC="${FF_DEC} -f imagepipe -img pgmyuv"

fi

# quality presets
if test "${QUALITY}" = "best"; then

    QUANT="4"

elif test "${QUALITY}" = "good"; then

    QUANT="5"

elif test "${QUALITY}" = "fair"; then

    QUANT="6"

else

    show_error "the specified quality preset is inexistant."
    show_error "Use -h to get help."
    exit 2

fi

# profile(s)
if test "${VCD_TYPE}" = "cvd"; then

    FF_ENC="${FF_ENC} -vcodec mpeg2video -f rawvideo -bufsize 224"
    MPEG2ENC="${MPEG2ENC} -f 8"
    
    VID_FMT_OUT="m2v"
    VID_MINRATE_OUT="0"
    VID_MAXRATE_OUT="7500"

    VID_SAR_525_OUT="20:11"
    VID_SAR_625_OUT="59:27"
    VID_SIZE_525_OUT="352x480"
    VID_SIZE_625_OUT="352x576"
    
    AUD_FMT_OUT="ac3"
    AUD_BITRATE_OUT="192"
    AUD_FREQ_OUT="48000"
    AUD_CHANNELS_OUT="2"
    
    # default volume size in mbytes (m = 1000)
    VOL_SIZE="4700"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 8"
    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -O preset=cvd"

elif test "${VCD_TYPE}" = "cvd_wide"; then

    FF_ENC="${FF_ENC} -vcodec mpeg2video -f rawvideo -bufsize 224"
    MPEG2ENC="${MPEG2ENC} -f 8"
    
    VID_FMT_OUT="m2v"
    VID_MINRATE_OUT="0"
    VID_MAXRATE_OUT="7500"

    VID_SAR_525_OUT="80:33"
    VID_SAR_625_OUT="236:81"
    VID_SIZE_525_OUT="352x480"
    VID_SIZE_625_OUT="352x576"
    
    AUD_FMT_OUT="ac3"
    AUD_BITRATE_OUT="192"
    AUD_FREQ_OUT="48000"
    AUD_CHANNELS_OUT="2"
    
    VOL_SIZE="4700"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 8"

elif test "${VCD_TYPE}" = "dvd"; then

    FF_ENC="${FF_ENC} -target dvd -f rawvideo -dc 10"
    MPEG2ENC="${MPEG2ENC} -f 8 -D 10"

    VID_FMT_OUT="m2v"
    VID_MINRATE_OUT="0"
    VID_MAXRATE_OUT="8000"
    
    VID_SAR_525_OUT="10:11"
    VID_SAR_625_OUT="59:54"
    VID_SIZE_525_OUT="720x480"
    VID_SIZE_625_OUT="720x576"
    
    AUD_FMT_OUT="ac3"
    AUD_BITRATE_OUT="192"
    AUD_FREQ_OUT="48000"
    AUD_CHANNELS_OUT="2"
    
    VOL_SIZE="4700"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 8"
    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -O preset=dvd"

elif test "${VCD_TYPE}" = "dvd_wide"; then

    FF_ENC="${FF_ENC} -target dvd -f rawvideo -dc 10"
    MPEG2ENC="${MPEG2ENC} -f 8 -D 10"

    VID_FMT_OUT="m2v"
    VID_MINRATE_OUT="0"
    VID_MAXRATE_OUT="8000"
    
    VID_SAR_525_OUT="40:33"
    VID_SAR_625_OUT="118:81"
    VID_SIZE_525_OUT="720x480"
    VID_SIZE_625_OUT="720x576"
    
    AUD_FMT_OUT="ac3"
    AUD_BITRATE_OUT="192"
    AUD_FREQ_OUT="48000"
    AUD_CHANNELS_OUT="2"
    
    VOL_SIZE="4700"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 8"
    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -O preset=dvd_wide"

elif test "${VCD_TYPE}" = "mvcd"; then

    FF_ENC="${FF_ENC} -vcodec mpeg1video -f rawvideo -bufsize 40 -me_range 64"
    MPEG2ENC="${MPEG2ENC} -f 2 -g 12 -G 24 -B 256 -S 797"

    MATRICES="mvcd"
    VID_FMT_OUT="m1v"
    VID_ILACE_OUT="p"
    VID_MINRATE_OUT="0"
    VID_MAXRATE_OUT="1152"
    
    VID_SAR_525_OUT="10:11"
    VID_SAR_625_OUT="59:54"
    VID_SIZE_525_OUT="352x240"
    VID_SIZE_625_OUT="352x288"
    
    AUD_FMT_OUT="mp2"
    AUD_BITRATE_OUT="128"
    AUD_FREQ_OUT="44100"
    AUD_CHANNELS_OUT="2"
    
    # 360000 sectors * 2324 bytes per sector / 1000 / 1000
    VOL_SIZE="836"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 2 -r 1400 -V"
    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -O preset=vcd"

elif test "${VCD_TYPE}" = "svcd"; then

    FF_ENC="${FF_ENC} -target svcd -f rawvideo"
    MPEG2ENC="${MPEG2ENC} -f 4 -B 256 -S 797"

    VID_FMT_OUT="m2v"
    VID_MINRATE_OUT="0"
    VID_MAXRATE_OUT="2500"
    
    VID_SAR_525_OUT="15:11"
    VID_SAR_625_OUT="59:36"
    VID_SIZE_525_OUT="480x480"
    VID_SIZE_625_OUT="480x576"
    
    AUD_FMT_OUT="mp2"
    AUD_BITRATE_OUT="224"
    AUD_FREQ_OUT="44100"
    AUD_CHANNELS_OUT="2"
    
    VOL_SIZE="836"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 4"
    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -O preset=svcd"

elif test "${VCD_TYPE}" = "vcd"; then

    ENC_TYPE="cbr"

    FF_ENC="${FF_ENC} -target vcd -f rawvideo -me_range 64"
    MPEG2ENC="${MPEG2ENC} -f 1 -B 256 -S 797"

    VID_FMT_OUT="m1v"
    VID_ILACE_OUT="p"
    
    VID_SAR_525_OUT="10:11"
    VID_SAR_625_OUT="59:54"
    VID_SIZE_525_OUT="352x240"
    VID_SIZE_625_OUT="352x288"
    
    AUD_FMT_OUT="mp2"
    AUD_BITRATE_OUT="224"
    AUD_FREQ_OUT="44100"
    AUD_CHANNELS_OUT="2"
    
    VOL_SIZE="836"

    MPLEX_FLAGS="${MPLEX_FLAGS} -f 1"
    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -O preset=vcd"

else

    show_error "the specified output type/profile is inexistant."
    show_error "Use -h to get help."
    exit 2

fi

# audio pass-through
if test "${ACOPY_MODE}" = "1"; then

    AUD_BITRATE_OUT="${AUD_BITRATE_SRC}"
    AUD_FMT_OUT="${AUD_FMT_SRC}"
    AUD_FREQ_OUT="${AUD_FREQ_SRC}"

fi

# output filenames
VIDEO_OUT="${PREFIX_OUT}.${VID_FMT_OUT}"
AUDIO_OUT="${PREFIX_OUT}.${AUD_FMT_OUT}"
MPEG_OUT="${PREFIX_OUT}-%d.mpg"

# output interlacing
if test "${VID_ILACE_OUT}" = "b"; then

    FF_ENC="${FF_ENC} -ildct -ilme -top 0"
    MPEG2ENC="${MPEG2ENC} -I 1 -z b"

elif test "${VID_ILACE_OUT}" = "t"; then

    FF_ENC="${FF_ENC} -ildct -ilme -top 1"
    MPEG2ENC="${MPEG2ENC} -I 1 -z t"

else

    MPEG2ENC="${MPEG2ENC} -I 0"

fi

# output framerate
if test "${VID_FPS_SRC}" = "30000:1001" || test "${VID_FPS_SRC}" = "24000:1001" || test "${VID_FPS_SRC}" = "25:1"; then

    VID_FPS_OUT="${VID_FPS_SRC}"

elif test "${OUT_NORM}" = "ntsc"; then

    VID_FPS_OUT="30000:1001"

elif test "${OUT_NORM}" = "ntsc_film"; then

    VID_FPS_OUT="24000:1001"

else

    VID_FPS_OUT="25:1"

fi

# output sar and framesize for pal/ntsc
if test "${VID_FPS_OUT}" = "25:1"; then

    VID_SAR_OUT="${VID_SAR_625_OUT}"
    VID_SIZE_OUT="${VID_SIZE_625_OUT}"

else

    VID_SAR_OUT="${VID_SAR_525_OUT}"
    VID_SIZE_OUT="${VID_SIZE_525_OUT}"

fi

# cvd_wide "preset" for y4mscaler
if test "${VCD_TYPE}" = "cvd_wide"; then

    Y4MSCALER="${Y4MSCALER} -O sar=${VID_SAR_OUT} -O size=${VID_SIZE_OUT}"

fi

# pipe buffer
if which ${BFR} >/dev/null; then

    PIPE_BUFFER="${BFR} -b 16m |"

fi

# video decoder
if test "${DEC_TOOL}" = "mpeg2dec"; then

    test_bin ${MPEG2DEC}
    DECODER="${MPEG2DEC} -s -o pgmpipe \"${VIDEO_SRC}\" | ${PGMTOY4M} |"

elif test "${DEC_TOOL}" = "ffmpeg"; then

    test_bin ${FFMPEG}
    DECODER="${FF_DEC} -y /dev/stdout | ${PGMTOY4M} |"

else

    show_error "the specified video decoder tool is not used by this script."
    show_error "Use -h to get help."
    exit 2

fi

# video framerate correction
if test "${VID_FPS_SRC}" = "${VID_FPS_OUT}"; then

    FRC=""

else

    test_bin ${YUVFPS}
    FRC="${YUVFPS} -r ${VID_FPS_OUT} |"

fi

# video "deinterlacer"
if test "${VID_ILACE_SRC}" != "p" && test "${VID_ILACE_OUT}" = "p"; then

    Y4MSCALER_FLAGS="${Y4MSCALER_FLAGS} -I ilace=top_only"

fi

# video scaler
if test "${VID_SAR_SRC}" = "${VID_SAR_OUT}" && test "${VID_SIZE_SRC}" = "${VID_SIZE_OUT}" && test "${VID_ILACE_SRC}" = "${VID_ILACE_OUT}"; then

    SCALER=""

else

    test_bin ${Y4MSCALER}
    SCALER="${Y4MSCALER} ${Y4MSCALER_FLAGS} |"

fi

# video filters chain
for FILTER in `echo ${FILTERS_CHAIN} | sed s/,/"\n"/g`; do

    FILTER_TYPE="`echo ${FILTER} | awk -F: '{print $1}'`"
    FILTER_LEVEL="`echo ${FILTER} | awk -F: '{print $2}'`"
    
    if test "${FILTER_LEVEL}" = ""; then

        FILTER_LEVEL="light"

    fi

    MEANFILTER_FLAGS="-f"
    Y4MDENOISE_FLAGS="-v 0"
    
    # filter level
    if test "${FILTER_LEVEL}" = "light"; then

        YUVMEDIANFILTER_FLAGS="-t 0"
        YUVDENOISE_FLAGS="-l 1 -t 4 -S 0"
        YUVDENOISE2_FLAGS="-y 4 -u 8 -v 8"
        YUVDENOISE3_FLAGS=""
        Y4MSPATIALFILTER_FLAGS="-x 0 -y 0"
        MEANFILTER_FLAGS="${MEANFILTER_FLAGS} -t 0"
        Y4MDENOISE_FLAGS="${Y4MDENOISE_FLAGS} -t 1"
        Y4MUNSHARP_FLAGS="-L 1.5,0.2,0"

    elif test "${FILTER_LEVEL}" = "medium"; then

        YUVMEDIANFILTER_FLAGS="-T 0"
        YUVDENOISE_FLAGS="-l 2 -t 6 -S 0"
        YUVDENOISE2_FLAGS="-y 8 -u 16 -v 16"
        YUVDENOISE3_FLAGS=""
        Y4MSPATIALFILTER_FLAGS="-X 4,0.64 -Y 4,0.8"
        MEANFILTER_FLAGS="${MEANFILTER_FLAGS} -r 1 -T 0 -w 12"
        Y4MDENOISE_FLAGS="${Y4MDENOISE_FLAGS} -t 2"
        Y4MUNSHARP_FLAGS="-L 2.0,0.3,0"

    elif test "${FILTER_LEVEL}" = "heavy"; then

        YUVMEDIANFILTER_FLAGS=""
        YUVDENOISE_FLAGS="-l 3 -t 8 -S 0"
        YUVDENOISE2_FLAGS="-y 16 -u 32 -v 32"
        YUVDENOISE3_FLAGS=""
        Y4MSPATIALFILTER_FLAGS="-X 4,0.5 -Y 4,0.5 -x 2,0.5 -y 2,0.5"
        MEANFILTER_FLAGS="${MEANFILTER_FLAGS} -r 1 -R 1"
        Y4MDENOISE_FLAGS="${Y4MDENOISE_FLAGS} -t 3"
        Y4MUNSHARP_FLAGS="-L 5.0,0.5,0"

    else

        show_error "the specified \"${FILTER_LEVEL}\" filter level is inexistant."
        exit 2

    fi

    # filter type
    if test "${FILTER_TYPE}" = "none"; then

        FILTER=""

    elif test "${FILTER_TYPE}" = "hqdenoise"; then

        test_bin ${Y4MDENOISE}
        FILTER="${Y4MDENOISE} ${Y4MDENOISE_FLAGS} |"

    elif test "${FILTER_TYPE}" = "mean"; then
    
        test_bin ${YUVMEDIANFILTER}
        FILTER="${YUVMEDIANFILTER} ${MEANFILTER_FLAGS} |"

    elif test "${FILTER_TYPE}" = "median"; then

        test_bin ${YUVMEDIANFILTER}
        FILTER="${YUVMEDIANFILTER} ${YUVMEDIANFILTER_FLAGS} |"

    elif test "${FILTER_TYPE}" = "spatial"; then

        test_bin ${Y4MSPATIALFILTER}
        FILTER="${Y4MSPATIALFILTER} ${Y4MSPATIALFILTER_FLAGS} |"

    elif test "${FILTER_TYPE}" = "temporal_old"; then

        test_bin ${YUVDENOISE}
        FILTER="${YUVDENOISE} ${YUVDENOISE_FLAGS} |"

    elif test "${FILTER_TYPE}" = "temporal_rc1"; then

        test_bin ${YUVDENOISE}
        FILTER="${YUVDENOISE} ${YUVDENOISE2_FLAGS} |"

    elif test "${FILTER_TYPE}" = "temporal"; then
        
        test_bin ${YUVDENOISE}
        FILTER="${YUVDENOISE} ${YUVDENOISE3_FLAGS} |"

    elif test "${FILTER_TYPE}" = "unsharp"; then

        test_bin ${Y4MUNSHARP}
        FILTER="${Y4MUNSHARP} ${Y4MUNSHARP_FLAGS} |"

    else

        show_error "the specified \"${FILTER_TYPE}\" filter type is not used by this script."
        exit 2

    fi

    FILTERS="${FILTERS} ${FILTER}"

done

# 3:2 pulldown
if test "${VID_FPS_OUT}" = "24000:1001" && test "${VID_FMT_OUT}" = "m2v"; then

    FF_ENC="${FF_ENC}"
    MPEG2ENC="${MPEG2ENC} -p"

fi

# quantisation matrices
if test "${MATRICES}" = "kvcd"; then

    FF_ENC="${FF_ENC} -intra_matrix ${INTRA_KVCD} -inter_matrix ${INTER_KVCD}"
    MPEG2ENC="${MPEG2ENC} -K kvcd"

elif test "${MATRICES}" = "mvcd"; then

    echo "`matrix_copy ${INTRA_MVCD}`" > ${CQMFILE}
    echo "`matrix_copy ${INTER_MVCD}`" >> ${CQMFILE}

    FF_ENC="${FF_ENC} -intra_matrix ${INTRA_MVCD} -inter_matrix ${INTER_MVCD}"
    MPEG2ENC="${MPEG2ENC} -K file=${CQMFILE}"

elif test "${MATRICES}" = "tmpgenc"; then

    FF_ENC="${FF_ENC} -intra_matrix ${INTRA_TMPGENC} -inter_matrix ${INTER_TMPGENC}"
    MPEG2ENC="${MPEG2ENC} -K tmpgenc"

fi

# video encoding mode
if test "${ENC_TYPE}" = "cbr"; then

    # when using -q with mpeg2enc, variable bitrate is selected.
    # it's not ok for vcds...
    MPEG2ENC="${MPEG2ENC}"

elif test "${ENC_TYPE}" = "abr"; then

    if test "${DURATION}" != ""; then
    
        # input total time (seconds)
        VID_TIME_SRC="`echo "${DURATION}" | \
        awk '{print $1 * 60}'`"

    else
    
        # input length (frames)
        if echo "${VID_FMT_SRC}" | grep -e mpeg1 -e mpeg2 >/dev/null 2>&1; then
	
            test_bin ${TCCAT} && test_bin ${TCDEMUX}
            VID_LENGTH_SRC="`${TCCAT} -i "${VIDEO_SRC}" | ${TCDEMUX} -W 2>&1 | awk '/unit/ {print sum+=$2}' | tail -1`"

        else

            test_bin ${TCPROBE}
            VID_LENGTH_SRC="`${TCPROBE} -i "${VIDEO_SRC}" 2>/dev/null | awk '/length:/ {print $2}'`"

        fi

        # input total time (seconds) = nb of frames / fps
        VID_TIME_SRC="`echo "${VID_LENGTH_SRC}:${VID_FPS_SRC}" | \
        awk -F: '{print $1 / $2 * $3 + 1}'`"

    fi
    
    test_bin bc
    
    # available kbits (k = 1000) per second = global bitrate
    BPS="`echo "${VOLUMES} * ${VOL_SIZE} * 1000 * 8 / ${VID_TIME_SRC}" | bc`"
    
    # muxing-overhead: 3% (0.97 * ...)
    VID_BITRATE_OUT="`echo "0.97 * ${BPS} - ${AUD_BITRATE_OUT}" | bc | \
    awk -F. '{print $1}'`"
    
    VID_BITRATE_OUT="`range_check ${VID_BITRATE_OUT} ${VID_MINRATE_OUT} ${VID_MAXRATE_OUT}`"
    
    FF_ENC="${FF_ENC} -minrate ${VID_MINRATE_OUT} -maxrate ${VID_MAXRATE_OUT} -b ${VID_BITRATE_OUT} -passlogfile ${STATFILE}"
    
    FF_STATS="${FF_ENC} -pass 1"
    MPEG2ENC_STATS="${MPEG2ENC} -b ${VID_BITRATE_OUT} -q 1 -v 1 2>${STATFILE}"
    
    FF_ENC="${FF_ENC} -pass 2"
    MPEG2ENC="${MPEG2ENC}"

else

    FF_ENC="${FF_ENC} -maxrate ${VID_MAXRATE_OUT} -qscale ${QUANT}"
    MPEG2ENC="${MPEG2ENC} -b ${VID_MAXRATE_OUT} -q ${QUANT}"

fi

# video encoder
if test "${ENC_TOOL}" = "ffmpeg"; then

    test_bin ${FFMPEG}
    ANALYSER="${FF_STATS} -y \"${VIDEO_OUT}\""
    ENCODER="${FF_ENC} -y \"${VIDEO_OUT}\""

elif test "${ENC_TOOL}" = "mpeg2enc"; then

    test_bin ${MPEG2ENC}
    ANALYSER="${MPEG2ENC_STATS} -o \"${VIDEO_OUT}\""
    ENCODER="${MPEG2ENC} -o \"${VIDEO_OUT}\""

else

    show_error "the specified video encoder tool is not used by this script."
    show_error "Use -h to get help."
    exit 2

fi

# audio resampler
if test "${AUD_FREQ_SRC}" = "${AUD_FREQ_OUT}"; then

    AUDIO_RESAMPLER=""

else

    test_bin ${SOX}
    AUDIO_RESAMPLER="${SOX} -t au /dev/stdin -r ${AUD_FREQ_OUT} -t au /dev/stdout polyphase |"

fi

# audio decoder/encoder
if test "${ACOPY_MODE}" = "1"; then

    AUDIO_DECODER="${FFMPEG} ${FF_DEC_AFLAGS}"
    AUDIO_ENCODER="-acodec copy -y \"${AUDIO_OUT}\""

elif test "${AUD_FREQ_SRC}" = "${AUD_FREQ_OUT}"; then

    AUDIO_DECODER="${FFMPEG} ${FF_DEC_AFLAGS}"
    AUDIO_ENCODER="-ab ${AUD_BITRATE_OUT} -ac ${AUD_CHANNELS_OUT} -y \"${AUDIO_OUT}\""

else

    AUDIO_DECODER="${FFMPEG} ${FF_DEC_AFLAGS} -v 0 -f au -y /dev/stdout |"
    AUDIO_ENCODER="${FFMPEG} -f au -i /dev/stdin -ab ${AUD_BITRATE_OUT} -ac ${AUD_CHANNELS_OUT} -y \"${AUDIO_OUT}\""

fi

# output some info.
show_info "===== SOURCE STREAM ====================="
show_info "           frame size: ${VID_SIZE_SRC}"
show_info "           frame rate: ${VID_FPS_SRC}"
show_info "          interlacing: ${VID_ILACE_SRC}"
show_info "   pixel aspect ratio: ${VID_SAR_SRC}"
show_info "===== OUTPUT STREAM ====================="
show_info "           frame size: ${VID_SIZE_OUT}"
show_info "           frame rate: ${VID_FPS_OUT}"
show_info "          interlacing: ${VID_ILACE_OUT}"
show_info "   pixel aspect ratio: ${VID_SAR_OUT}"

# video "analyse"
if ! test "${BLIND_MODE}" = "1" && test "${ENC_TYPE}" = "abr"; then

    eval "${DECODER} ${FILTERS} ${FRC} ${SCALER} ${PIPE_BUFFER} ${ANALYSER}"
    
    if test "${ENC_TOOL}" = "mpeg2enc"; then

        # calculate average quant value from pseudo statfile (mpeg2enc output)
        QUANT="`awk -F= '/quant=/ {print $2}' "${STATFILE}" | \
        awk '{iquant=$1; \
        oquant=iquant; \
        if (iquant>=  10) {oquant=9}; \
        if (iquant>=  12) {oquant=10}; \
        if (iquant>=  14) {oquant=11}; \
        if (iquant>=  16) {oquant=12}; \
        if (iquant>=  18) {oquant=13}; \
        if (iquant>=  20) {oquant=14}; \
        if (iquant>=  22) {oquant=15}; \
        if (iquant>=  24) {oquant=16}; \
        if (iquant>=  28) {oquant=17}; \
        if (iquant>=  32) {oquant=18}; \
        if (iquant>=  36) {oquant=19}; \
        if (iquant>=  40) {oquant=20}; \
        if (iquant>=  44) {oquant=21}; \
        if (iquant>=  48) {oquant=22}; \
        if (iquant>=  52) {oquant=23}; \
        if (iquant>=  56) {oquant=24}; \
        if (iquant>=  64) {oquant=25}; \
        if (iquant>=  72) {oquant=26}; \
        if (iquant>=  80) {oquant=27}; \
        if (iquant>=  88) {oquant=28}; \
        if (iquant>=  96) {oquant=29}; \
        if (iquant>= 104) {oquant=30}; \
        if (iquant>= 112) {oquant=31}; \
        nquant+=1; \
        sum_quant+=oquant; \
        print sum_quant/nquant}' | \
        tail -1 | awk -F. '{print $1}'`"
        QUANT="`range_check $QUANT 3 31`"
        ENCODER="${MPEG2ENC} -b ${VID_MAXRATE_OUT} -q ${QUANT} -o ${VIDEO_OUT}"

    fi

fi

# video (de)coding
if ! test "${BLIND_MODE}" = "1"; then

    eval "${DECODER} ${FILTERS} ${FRC} ${SCALER} ${PIPE_BUFFER} ${ENCODER}"

fi

# audio (de)coding
if ! test "${MUTE_MODE}" = "1"; then

    eval "${AUDIO_DECODER} ${AUDIO_RESAMPLER} ${AUDIO_ENCODER}"

fi

# multiplexing
if ! test "${BLIND_MODE}" = "1" && ! test "${MUTE_MODE}" = "1"; then

    if test "${REQUANT_MODE}" = "1"; then

        VID_SRC_SIZE="`ls -l ${VIDEO_OUT} | awk '{print $5}'`"
        AUD_SRC_SIZE="`ls -l ${AUDIO_OUT} | awk '{print $5}'`"
        
        REQUANT_FACT="`echo "scale=3; ${VID_SRC_SIZE} / (0.96 * ${VOL_SIZE} * 1000 * 1000 - ${AUD_SRC_SIZE})" | bc -l`"

        # sed is here to replace a "requant_fact * 1000"
        if test `echo ${REQUANT_FACT} | sed s/"\."//` -gt 1000; then

            VIDEO_SRC="${VIDEO_OUT}"
            VIDEO_OUT="${PREFIX_OUT}.req"

            show_info "Requantisation factor: ${REQUANT_FACT}"
            
            test_bin ${TCREQUANT}
            ${TCREQUANT} -i ${VIDEO_SRC} -o ${VIDEO_OUT} -f ${REQUANT_FACT}

        else

            show_warn "Requantisation not done. Requantisation factor lesser than 1.0."

        fi

    fi

    test_bin ${MPLEX}
    ${MPLEX} ${MPLEX_FLAGS} -o "${MPEG_OUT}" "${VIDEO_OUT}" "${AUDIO_OUT}"

fi

rm -f ${CQMFILE} ${STATFILE}

exit 0
