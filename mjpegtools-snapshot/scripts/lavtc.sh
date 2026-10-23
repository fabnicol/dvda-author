#!/bin/bash
#
# lavtc.sh - use transcode to convert MJPEG avi files to another 
#               video/audio format.
#
# copyright 2003 Shawn Sulma <mjpeg@athos.cx>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Description:
#
# This is intended to be a fairly intelligent wrapper for the transcode
# utility when working with mjpeg files and the lav tools.  It assumes
# that you have transcode and the lavtools installed and available in the
# $PATH.
#
# Transcode can be found at:
# http://www.theorie.physik.uni-goettingen.de/~ostreich/transcode/
#
# It is written for bash specifically.  It _should_ be fairly portable with
# only minor changes to other important shells such as zsh and ksh.  It is
# not particularly compatible with the rather minimal sh implementation
# found in some Linux distros.  I don't know much about [t]csh, but doubt
# it's easily portable to that.
#
# I'm sure there are a few naive things left in this script. Constructive
# feedback is welcome.
#
# It doesn't do much checking for existence of codecs, or allow passing of
# arbitrary parameters to transcode (including codec-specific ones). 
# However I think it does manage to cover most of the common ground.  It
# does some simple range checking on many of the parameters.
#
# A useful feature is the ability to specify "--two-pass".  For most DIVX
# codecs (and some others), two pass encoding gives better overall results.
# This script manages the two pass process.  It generates a log file name
# for the first pass based on the output name.  When the first pass is done,
# it generates an md5sum of the log.  This allows the second pass to be
# interrupted.  When the script is run on the same output file again, it
# checks the md5sum of the log.  If it's valid, it assumes that the first
# pass is already successfully done and restarts only the second pass.  Yes,
# this can still break, but it's saved me much effort in converting.
#
# Transcode comes bundled with some plugins that are useful.  In particular,
# ivtc and smartdeinter.  These do inverse telecine and deinterlacing (based
# on SmartDub's deinterlacer which is rather nice).  However, I've found
# that yuvkineco and yuvdenoise sometimes (often) do a better job of inverse
# telecine or deinterlacing.  While there is support for some of the
# yuv4mpeg utilities in transcode as plugin filters, I've added here some
# additional capabilities that basically use yuvdenoise, yuvcorrect or
# yuvkineco as "pre-processors" for transcode.  Hiding the ugliness of this
# from someone who just wants a nice conversion is a good thing.  Of course,
# if you want to do something more complicated, please do, but this is
# intended to handle only modestly complex usage.
#
# As an additional "feature", this script automates the creation of an ogg
# media file from the transcoded file.  Ogg is a media container format,
# much like AVI.  Most people know ogg from Ogg Vorbis, which is a very good
# lossy audio codec.  The container format, though also accepts video
# streams.  Specifying "--ogg" on the command line will generate a separate
# ogg audio file (since most avi-aware codecs in transcode cannot handle ogg
# Vorbis).  After processing is complete, the script uses the 'ogmmerge'
# utility to combine the video and audio streams together into a single ogg
# file.  ogmmerge is not part of the ogg development download.  You can get
# it here: http://www.bunkus.org/videotools/ogmtools/
#
# Determining the correct video and audio bandwith to use is a bit annoying
# at times.  This script attempts to help with this by having some bandwidth
# estimation capabilities.  "--max-length" allows you to specify the maximum
# MiB to be used by the final output file.  The script will figure out the
# appropriate video bandwidth.  It also does some automatic calculation of
# audio bandwidths (essentially for mp3 56kbps for mono and 128kbps for
# stereo, and for vorbis/ogg, 48 and 96 respectively (Vorbis gives similar
# quality at those rates)).  If you specify the video and/or audio bitrates
# with -b and -a, those values are used, and not any calculated ones.  Smart
# users can do better than the calculation, but not too often.
#
# The default output codec is "xvidcvs".  This is the most up-to-date
# xvid/divx encoder.  If you do not have the necessary xvid libs around, you
# can specify another video code with "--videocodec".  Other typical values
# might be "odivx", "ffmpeg" or "xvid".
#
# I am partial to xvid.  You can find it here: http://www.xvid.org/
#
# Currently the script will process only one input file.  If you have
# multiple, you should create a edit list for them, and provide that edit
# list to the script as the input file.
#
# Most of the script is sanity-checking.  Or comments.  Or logging.
#
# Example usage:
#
# simple case:
#   lavtc.sh -o out.avi -b 1000 -a 128 in.eli
#     use lots of defaults, no clipping, or resize or deinterlace, just use
#     1000kbps for video and 128kbps for audio)
#
# something more complicated:
#   lavtc.sh -o out.avi --two-pass --max-length 700 in.eli
#     mostly use defaults, but do two-pass encoding, and determine the bitrates
#     necessary to fit the resulting output.avi onto an 80 min CDR )
#
#   lavtc.sh --clip 20,20,10,10 --scale 0.75 -o out.avi -b 1000 in.eli
#     clip 20 pixels from the top and left and 10 from the bottom and right,
#     then scale the result by 75%.
#
#   lavtc.sh --pulldown=kineco -o out.avi --ogg --luma 75 --chroma 75 in.eli
#     uses yuvkineco to perform inverse telecine, adjusts the luma and chroma
#     contrasts, and creates an ogg media file as output.  The final output is
#     written to "out.ogm".
# 
# Dependencies (script checks the availability of each as needed):
#	(always) getopt, which, dc, cut, tr, expr, lavinfo, transcode
#	(for two-pass automation) md5sum
# 	(for yuvdenoise deinterlace) yuvdenoise, yuvcorrect
#	(for yuvkineco inverse telecine) yuvkineco, yuvcorrect
#	(for "--ogg") ogmmerge
#
# History:
#	This script replaces the lav2divx utility
#	2003-06-01: Initial version (Shawn Sulma)
#

appName=`basename ${0}`
appVersion="0.1"

#
# Helpers.  These might some day belong in an include script file themself.
#
assertExists ()
{
	[ -x `which ${1}` ] || log_fail ${1} not found. ${2}${2+ requires this.}
}

assertNumber ()
{
	if [[ `expr "${1}" : "[0-9${3}]*" 2> /dev/null` -ne ${#1} ]] ; then
		log_error "${2}${2+:} \"${1}\" is not a valid value"
		exit 1;
	fi
}

assertRange ()
{
	assertNumber ${1} ${3}
	if [[ ${1} -lt 0 || ${1} -gt ${2} ]] ; then
		log_error "${3}${3+:} \"${1}\" outside allowed range 0-${2}"
		exit 1 ;
	fi
}

#
# parse a string into multiple parts separated by any of "xX,._+:-".  This is useful
# for geometry strings: "000x000+000+000".  It ignores the separators.  The result is
# stored in the array ${geometry}
#
parseGeometry ()
{
	geometry=(`echo "${1}" | tr ' xX,._+:-' '         '`)
}

#
# logging mechanism.  It handles different levels of output.  Yes, it's not
# too efficient, but this is a wrapper script, and performance of this script
# is not required to be stellar.
#
debug=3
info=2
warn=1
error=0
logLevelText=( "**ERROR" "## WARN" "== INFO" "--DEBUG" )

log_debug () { __log ${debug} $* ; }
log_info () { __log ${info} $* ; }
log_warn () { __log ${warn} $* ; }
log_error () { __log ${error} $* ; }
log_fail () { __log ${error} $* ; exit 1 ; }
__log ()
{
	if [[ ${logLevel=2} -ge ${1} ]] ; then
		level=${1}
		shift
		echo "${logLevelText[${level}]}: [${appName}] $*" >&2
	fi
}

#
# Usage and Help display
#
usage ()
{
	cat << EOF
${appName} ${appVersion} - a transcode wrapper script for lavtools

Usage:    ${appName} -o <output file> [options] <input file>
For help: ${appName} -h

EOF
}

helptext ()
{
	usage
	cat << EOF
Options:
  -h | --help		  this help output
  -o | --output=<file>	  output to <file>
  -b | --videorate=<n>	  use <n> kilobits/second as the video bitrate
  -a | --audiorate=<n>	  use <n> kilobits/second as the audio bitrate
  -m | --mono		  convert the audio to mono before encoding
  -v | --verbose=<n>	  select verbosity level (0-3, higher outputs more)
  --maxlength=<n>	  calculate video bandwidth for output size <n> MiB
  --two-pass		  perform two-pass encoding where possible
  --clip=<t>,<l>,<b>,<r>  clip number of pixels from top, left, bottom, right
  --frame=<w>x<h>+<x>+<y> use the specified frame geometry (clip to)
  --resize=<w>,<h>	  resize to specified width and height
  --scale=<scale>	  scale by the factor <scale> (1.00 = 100%)
  --deinterlace[=<type>]  set deinterlace/inverse telecine type
 			  (type is "smart", "denoise", "kineco" or "ivtc")
  --line-switch		  swap lines prior to processing
  --luma=<contrast>	  set luma contrast
  --chroma=<contrast>	  set chroma contrast (saturation)
  --frames=<from>-<to>	  only process frames <from> through <to>
  --frames=<number>	  only process first <number> frames
  --status-delay=<frames> update status line every <frames> frames
  --videocodec=<codec>	  use the specified codec (xvid, ffmpeg, etc)
  --grayscale		  convert to black and white
  --quality=<n>		  use quality level <n> (1-5, default 5)
  --crispness=<n>	  use crispness <n> (default 100)
  --ogg			  generate an ogg media file (use Vorbis not MP3)
  --norm={n,p}		  declare norm to be "n" or "p" (passed to preprocess)

EOF
}

parseParams ()
{
args=`getopt -n ${appName} -o o:b:a:mv:hV -l  output:,videorate:,audiorate:,mono,verbose:,max-length:,two-pass,ogg,clip:,frame:,scale:,deinterlacer::,pulldown::,deinterlace::,line-switch,luma:,chroma:,frames:,status-delay:,grayscale,videocodec:,crispness:,quality:,resize:,scale:,help,norm:,greyscale,version,norm: -- "$@"`

	if [ $? != 0 ] ; then
		log_fail "Cannot continue."
	fi
	eval set -- "$args"
	while true ; do
		case "${1}" in
			-h|--help) helptext >&2 ; exit 0;;
			-o|--output) output="${2}" ; shift 2 ;;
			-b|--video-rate) 
				assertNumber ${2} "video bandwidth"
				videoBW="${2}" ; 
				shift 2 ;;
			-a|--audio-rate) 
				assertNumber ${2} "audio bandwidth" ; 
				audioBW="${2}" ;
				shift 2 ;;
			-m|--mono) audioOutChannels=1 ; shift ;;
			-v|--verbose) 
				assertRange ${2} 3 "verbosity level" ;
				logLevel=${2} ; 
				shift 2 ;;
			--max-length) 
				assertNumber ${2} "maximum output size"  ; 
				maxlength="${2}" ; 
				shift 2 ;;
			--two-pass) 
				assertExists md5sum "${1}" ;
				twopass=1 ; 
				shift ;;
			--ogg) 
				assertExists ogmmerge "${1}" ;
				ogg=1 ; 
				shift ;;
			--clip) clip="${2}" ; shift 2 ;;
			--frame) frame="${2}" ; shift 2 ;;
			--line-switch) flip=1 ; shift ;;
			--resize) resize="${2}" ; shift 2 ;;
			--scale) 
				videoScale="${2}" ; 
				assertNumber ${2} "video scaling factor" . ; 
				shift 2 ;;
			--deinterlace*|--pulldown)
				case "${2}" in
					denoise|kineco)
						assertExists yuv${2} "${1} ${2}"
						deinterlacer="${2}" ;;
					smart|ivtc)
						deinterlacer="${2}" ;;
					"") deinterlacer="smart" ;;
					*) log_fail "Unknown deinterlacer type: ${2}" ;;
				esac ; shift 2 ;;
			--luma) 
				assertNumber ${2} "luma contrast" ; 
				assertExists yuvdenoise "${1}"
				lumaContrast="${2}" ; 
				shift 2 ;;
			--chroma) 
				assertNumber ${2} "chroma contrast" ;
				assertExists yuvdenoise "${1}"
				chromaContrast="${2}" ; 
				shift 2 ;;
			--frames) frameRange="${2}" ; shift 2 ;;
			--status-delay) 
				assertNumber ${2} "status display delay" ;
				delay="${2}" ;
				shift 2 ;;
			--grayscale|--greyscale) grayscale="1" ; shift ;;
			--videocodec) videoOutCodec="${2}" ; shift 2 ;;
			--quality) 
				assertNumber ${2} "quality" ;
				quality="${2}" ; 
				shift 2 ;;
			--crispness) 
				assertNumber ${2} "crispness" ; 
				crispness="${2}" ; 
				shift 2 ;;
			--norm) norm="${2}" ; shift 2 ;;
			-V|--version) usage ; exit 0 ;;
			--) shift ; break ;;
			*) log_fail "Internal error at ${1}" ;;
		esac
	done
	# transcode can only handle one file input.  This script will eventually 
	# handle more, but for now...
	inFile="${1}"
	if [[ -z ${output} ]] ; then
		log_error "output file required"
		usage
		exit 1;
	elif [[ -z ${inFile} ]] ; then
		log_error "one input file required"
		usage
		exit 1;
	elif [ ! -e ${inFile} ] ; then
		log_fail "input file ${inFile} does not exist"
	fi
}

#
# read information about the input file.  There's probably a better way to
# do this, but this works well enough.
#
readHeader ()
{
	header="`lavinfo ${norm++}${norm} ${inFile}`"
	for line in ${header} ; do
		value=`echo ${line} | cut -d '=' -f2`
		case "${line}" in
			video_width*) videoWidth=${value} ;;
			video_height*) videoHeight=${value} ;;
			video_fps*) videoFrameRate=${value} ;;
			video_inter*) videoIsInterlaced=${value} ;;
			video_frames*) totalFrames=${value} ;;
			audio_bits*) audioInBits=${value} ;;
			audio_chans*) audioInChannels=${value} ;;
			audio_rate*) audioInRate=${value} ;;
			num_video_files*) numVideoFiles=${value} ;;
		esac
	done
	videoSeconds=`dc -e "0 k ${totalFrames} ${videoFrameRate} / p"`
}

#
# For kineco and yuvdenoise deinterlace, a fifo is required.  Ensure one
# exists, and clean up when we're done with it.
#
ensureFifo ()
{
	fifo="${outputBaseName}.fifo"
	if [ -p ${fifo} ] ; then
		log_debug "Fifo already established"
	elif [ -e ${fifo} ] ; then
		log_fail "${fifo} is not a FIFO.  Cannot continue." ;
	else
		log_debug "Creating fifo ${fifo}"
		log_debug mkfifo ${fifo}
		mkfifo ${fifo}
		needToCleanupFifo=1
	fi
}
cleanupFifo ()
{
	if [ -p ${fifo} ] ; then
		log_debug "removing fifo ${fifo}"
		log_debug rm ${fifo}
		rm ${fifo}
	fi
}

#
# If we're not using a pre-processing pipe, we can perform line switching
# through a filter plugin.
#
setFlipArgs ()
{
	if [[ ${flip=0} = 1 && ${videoIsInterlaced} = 1 ]] ; then 
		flipArgs="-J fields=flip"
	fi
}

#
# Set up for handling the different types of deinterlacers and inverse telecine
# filters.  Some require separate filter entries, while others need pre-
# processing pipelines.  The latte introduces complications because now the
# audio has to be passed to transcode separately.
#
setDeinterlaceArgs ()
{
	# set up for deinterlacing.  In some cases, the actual deinterlacing 
	# is finally triggered elsewhere.
	#
	# get the audio and video from the input file (default case)
	#
	inputFile="${inFile}"
	inputVideoCodec="lav"
	if [[ ${videoIsInterlaced} = 0 || ${deinterlacer=none} = "none" ]] ; then
		deinterlacerText="No deinterlacing"
		setFlipArgs
	elif [ ${deinterlacer} = "smart" ] ; then
		deinterlacerText="SmartDub deinterlacer"
		deinterlaceArgs="-J smartdeinter=cubic=1:diffmode=2:highq=1"
		setFlipArgs
	elif [ ${deinterlacer} = "ivtc" ] ; then
		deinterlacerText="Transcode ivtc inverse telecine"
		deinterlaceArgs="-J ivtc,32detect=force_mode=5,decimate"
		setFlipArgs
	else
		ensureFifo
		#
		# We have to turn off auto-probing.  Thus we have to provide
		# information about the video and audio input ourselves.
		#
		cineRate=`dc -e "6 k 24000 1001 / p"`
		deinterlaceArgs="-H 0 -f ${cineRate},1 -M 0 -g ${videoWidth}x${videoHeight} -e ${audioInRate},${audioInBits},${audioInChannels}"
		#
		# get the video from the fifo and the audio from the input
		#
		inputFile="${fifo} -p ${inFile}"
		inputVideoCodec="yuv4mpeg"
		inputAudioCodec=",lav"
		if [ ${deinterlacer} = "denoise" ] ; then
			# in theory, the builtin yuvdenoise filter plugin should
			# be able to do this.  In practise, I could not find a 
			# practical way to tell it to _only_ deinterlace.
			deinterlacerText="yuvdenoise deinterlacer"
		elif [ ${deinterlacer} = "kineco" ] ; then
			#
			# yuvkineco 1 pass inverse telecine
			#
			deinterlacerText="yuvkineco inverse telecine"
			# since the incoming stream is now at a different frame rate, we need
			# to recalculate the number of frames that can be seen and used by transcode
			totalFrames=`dc -e "8 k ${cineRate} ${videoFramesRate} / ${totalFrames} * 0 k 1 + 1 / p"`
		else
			log_fail "Unknown deinterlacer/pulldown: ${deinterlacer}"
		fi
	fi
}

setAudioArgs ()
{
	#
	# if there's no audio, don't bother with it.
	#
	if [[ ${audioInChannels} -gt 0 ]] ; then
		#
		# keep the same bitrate and sample size
		#
		audioOutBits=${audioInBits}
		audioOutRate=${audioInRate}
		if [[ -z ${audioOutChannels} ]] ; then
			audioOutChannels=${audioInChannels}
		fi
		#
		# If the output bitrate isn't set, determine it.
		#
		if [[ -z ${audioBW} ]] ; then
			if [[ -n ${ogg} ]] ; then
				# Ogg Vorbis compresses better
				let audioBW="48*${audioOutChannels}"
			else
				if [ ${audioOutChannels} = 1 ] ; then
					audioBW=56
				else
					let audioBW="64*${audioOutChannels}"
				fi
			fi
		fi
		audioArgs="-E ${audioOutRate},${audioOutBits},${audioOutChannels} -b ${audioBW}"
		if [[ -n ${ogg} ]] ; then
			oggName="${outputBaseName}.audio.ogg"
		fi
	fi
}

setVideoBW ()
{
	if [[ -n ${maxlength} ]] ; then
		#
		# if not set, guess an appropriate bandwidth.
		# This is, in some ways, a voodoo calculation.  Well, to be
		# precise, I'm guessing at the additional 6 bytes of overhead
		# per frame.  In practise, I've found this calculation is
		# actually quite useful; conservative without being too much so.
		#
		let availableBytes="${maxlength}*1024*1024"
		# bandwidth calculation
		let bytesIndex="${totalFrames}*8"
		let bytesAudio="${videoSeconds}*${audioBW-0}*128"
		let bytesOverhead="${totalFrames}*6"
		let remainder="${availableBytes}-${bytesAudio}-${bytesIndex}-${bytesOverhead}"
		bandwidth=`dc -e "${remainder} ${videoSeconds} / 128 / p"`
	fi
	videoBW=${videoBW-${bandwidth-1000}}
}

setClipArgs ()
{
	if [[ -n ${clip} ]] ; then
		parseGeometry ${clip}
		clipTop=${geometry[0]}
		clipLeft=${geometry[1]}
		clipBottom=${geometry[2]}
		clipRight=${geometry[3]}
	elif [[ -n ${frame} ]] ; then
		parseGeometry ${frame}
		assertRange ${geometry[0]} ${videoWidth} "frame width"
		assertRange ${geometry[1]} ${videoHeight} "frame height"
		assertRange ${geometry[2]} ${videoWidth} "frame x offset"
		assertRange ${geometry[3]} ${videoHeight} "frame y offset"
		#
		# convert the geometry string to clipping pixels.
		#
		let clipBottom=${videoHeight}-${geometry[1]}-${geometry[3]}
		let clipTop=${geometry[3]}
		let clipRight=${videoWidth}-${geometry[0]}-${geometry[2]}
		let clipLeft=${geometry[2]}
	fi
	assertRange ${clipTop=0} ${videoHeight} "top clipping parameter"
	assertRange ${clipBottom=0} ${videoHeight} "bottom clipping parameter"
	assertRange ${clipLeft=0} ${videoWidth} "left clipping parameter"
	assertRange ${clipRight=0} ${videoWidth} "right clipping paramter"
	let totalClipped="${clipTop}+${clipBottom}+${clipRight}+${clipLeft}"
	if [ ${totalClipped} != 0 ] ; then
		clippingArgs="-j ${clipTop},${clipLeft},${clipBottom},${clipRight}"
	fi
}

setResizeArgs ()
{
	let clippedHeight="${videoHeight}-${clipTop}-${clipBottom}"
	let clippedWidth="${videoWidth}-${clipLeft}-${clipRight}"
	if [[ -n ${resize} ]] ; then
		videoOutWidth=${resize%%[x./-, ]*}
		videoOutHeight=${resize##*[x./-, ]}
		assertNumber ${videoOutWidth} "target width"
		assertNumber ${videoOutHeight} "target height"
	else
		assertNumber ${videoScale} "output scaling factor" .
		videoScale=${videoScale-1.00}
		videoOutWidth=`dc -e "0 k ${clippedWidth} ${videoScale} * 1 / p"`
		videoOutHeight=`dc -e "0 k ${clippedHeight} ${videoScale} * 1 / p"`
	fi
	if [[ ${videoOutHeight} != ${clippedHeight} || ${videoOutWidth} != ${clippedWidth} ]] ; then
		resizeArgs="-Z ${videoOutWidth}x${videoOutHeight}"
	fi
}

setEnhanceArgs ()
{
	[[ -n "${lumaContrast}" || -n "${chromaContrast}" ]] && enhanceArgs="-J yuvdenoise=mode=2"
	[[ -n ${lumaContrast} ]] && enhanceArgs="${enhanceArgs}:luma_contrast=${lumaContrast}"
	[[ -n ${chromaContrast} ]] && enhanceArgs="${enhanceArgs}:chroma_contrast=${chromaContrast}"
	[[ -n ${grayscale} ]] && enhanceArgs="-K ${enhanceArgs}"
}

setFramesRangeArgs ()
{
	if [[ `expr "${frameRange=0}" : "[0-9]*[^0-9][0-9]*" 2> /dev/null` -eq ${#frameRange} ]] ; then 
		firstFrame="${frameRange%%[.,-]**}"
		lastFrame="${frameRange##*[.,-]}"
	else
		firstFrame=1
		lastFrame=${frameRange}
		assertNumber ${lastFrame} "frames to encode"
	fi
	if [[ -z $firstFrame ]] ; then 
		firstFrame=0 ;
	fi
	if [[ -z $lastFrame ]] ; then 
		lastFrame=0 ; 
	fi
	# normalise negatives
	if [ ${firstFrame} -lt 0 ] ; then
		let firstFrame=${totalFrames}${firstFrame}
	fi
	if [ ${lastFrame} -lt 0 ] ; then
		let lastFrame=${totalFrames}${lastFrame}
	fi
	# now check for valid ranges.
	if [ ${firstFrame} = 0 -a ${lastFrame} = 0 ] ; then
		firstFrame=1
		lastFrame=${totalFrames}
	elif [ ${lastFrame} = 0 ] ; then
		lastFrame=${firstFrame}
		firstFrame=1
	elif [ ${firstFrame} = 0 ] ; then
		firstFrame=1
	fi
	# upper bounds
	if [ ${lastFrame} -gt ${totalFrames} ] ; then
		lastFrame=${totalFrames}
	fi
	if [ ${firstFrame} -gt ${lastFrame} ] ; then
		firstFrame=${lastFrame}
	fi
	# generate arg
	framesRangeArgs="-c ${firstFrame}-${lastFrame}"
}

setStatusArgs ()
{
	if [[ ${logLevel} -lt 2 ]] ; then
		statusArgs="-q 0" ;
	elif [[ ${logLevel} -eq 2 ]] ; then
		statusArgs="-q 1" ;
		statusOnly="1" ;
	else
		statusArgs="-q 1" ;
	fi
	if [[ ${logLevel} -gt 0 ]] ; then
		delayArgs=" --print_status ${delay=1} "
	fi
}

initialize ()
{
outputBaseName="${output%.*}"
readHeader
setDeinterlaceArgs
setAudioArgs
setClipArgs
setResizeArgs
setEnhanceArgs
setFramesRangeArgs
setStatusArgs
setVideoBW

	videoOutCodec="${videoOutCodec-xvidcvs}"
	quality="${quality-5}"
	crispness="${crispness-144}"
	log_info "encoding ${inFile}"
	log_info "input video: ${videoWidth}x${videoHeight}"
	log_info "input total length: ${totalFrames} frames (${videoSeconds} seconds)"
	log_info "deinterlace type: ${deinterlacer} (input is " `if [[ ${videoIsInterlaced} = 0 ]] ; then echo -n "not " ; fi ` " interlaced)"
	if [[ ${audioInChannels} = 0 ]] ; then
		log_info "no audio on input"
	else
		log_info "input audio: ${audioInRate},${audioInBits},${audioInChannels}"
		log_info "output audio: ${audioOutRate},${audioOutBits},${audioOutChannels}"
		log_info "output audio bandwidth: ${audioBW}"
		if [[ -n ${ogg} ]] ; then
			log_info "using ogg vorbis for audio "
		fi
	fi
	log_info "clipping: left ${clipLeft-0}, top ${clipTop-0}, right ${clipRight-0}, bottom ${clipBottom-0}"
	log_info "resize scale: ${videoScale-n/a}, result ${videoOutWidth}x${videoOutHeight}"
	log_info "contrast: ${lumaContrast-n/a}, saturation: ${chromaContrast-n/a}"
	log_info "encoding frames: ${firstFrame} to ${lastFrame}"
	log_info "video bandwidth: estimated ${bandwidth-n/a}, used ${videoBW}"
	log_info "transcode video out codec: ${videoOutCodec}"

transcodeArgs="\
-i ${inputFile} \
${clippingArgs} \
${resizeArgs} \
${flipArgs} \
${deinterlaceArgs} \
${enhanceArgs}
${audioArgs} \
-Q ${quality} \
-o ${output} \
-w ${videoBW},,${crispness} \
-V -k ${framesRangeArgs} ${statusArgs} ${delayArgs}"

}

#
#
# Main section of the script. It handles the creation of the pipeline 
# (if necessary) as well as the management of two pass encodings (i.e. the log
# file and running the separate passes).
#
#
for required in expr transcode dc getopt cut tr lavinfo ; do
	[ -x `which ${required}` ] || log_fail "${required} not found.  Cannot continue."
done
parseParams "$@"
initialize
pass=1
while [ ${pass} != 2 ] ;
do
	if [[ -n ${twopass} ]] ; then
		passArgs=""
		if ( md5sum -c ${outputBaseName}.pass1.md5sum &> /dev/null ) then
			# if [[ ${pass} = 3 ]] ; then
			pass=2
			inputAudioCodec=""
			if [[ -n ${ogg} ]] ; then
				passArgs="-m ${oggName}"
				outputAudioCodec=",ogg"
			else
				outputAudioCodec=""
			fi
			log_info "Valid first pass log_file found"
		else
			pass=1
			log_warn "First pass log_file not found or not valid"
			log_debug "removing spurious first pass log_file."
			log_debug "rm ${outputBaseName}.pass1.log &> /dev/null"
			rm ${outputBaseName}.pass1.log &> /dev/null
			passArgs=""
			outputAudioCodec=",null"
			inputAudioCodec=",null"
		fi
		passArgs="-R ${pass},${outputBaseName}.pass1.log ${passArgs}"
	else
		# single pass.
		log_info "single pass encoding only." >&2
	fi

	log_info "beginning pass ${pass}" >&2
	# set up pipeline if needed (using kineco or yuvdenoise deinterlacer)
	if [[ ${videoIsInterlaced} = 1 && ( "${deinterlacer}" = "kineco" || "${deinterlacer}" = "denoise" ) ]] ; then 
		if [[ -n ${flip} ]] ; then
			corrector="yuvcorrect -v 0 -T LINE_SWITCH"
		else
			corrector="cat"
		fi
		if [ ${deinterlacer} = "kineco" ] ; then
			deinterlaceCmd="yuvkineco -F 1 "
		else
			deinterlaceCmd="yuvdenoise -I -v 0 "
		fi
		log_info "invoking deinterlacer/pulldown thread"
		log_debug "lav2yuv ${inFile} | ${corrector} | ${deinterlaceCmd} > ${fifo} &"
		lav2yuv ${inFile} | ${corrector} | ${deinterlaceCmd} > ${fifo} &
	fi
	if [[ "${audioInChannels}" = 0 ]] ; then
		inputAudioCodec=",null"
		outputAudioCodec=",null"
	elif [[ ${pass} = 2 ]] ; then
		inputAudioCodec=",lav"
	fi
	inputCodecArgs="-x ${inputVideoCodec}${inputAudioCodec}"
	outputCodecArgs="-y ${videoOutCodec}${outputAudioCodec}"
	#
	# this is the big call
	#
	log_debug `for arg in /usr/local/bin/transcode ${passArgs} ${transcodeArgs} ${inputCodecArgs} ${outputCodecArgs} ; do echo -n "${arg} " ; done` " <EOL>"
	/usr/local/bin/transcode \
		${passArgs} \
		${transcodeArgs} \
		${inputCodecArgs} \
		${outputCodecArgs} 
	errcode=$?
	# errcode=0
	if [ ${errcode} != 0 ] ; then
		exit ${errcode}
	fi
	if [[ -n ${twopass} ]] ; then
		if [ ${pass} = 1 ] ; then
			log_debug "md5sum ${outputBaseName}.pass1.log >  ${outputBaseName}.pass1.md5sum"
			md5sum ${outputBaseName}.pass1.log > ${outputBaseName}.pass1.md5sum
		else
			log_debug "mv ${outputBaseName}.pass1.log ${outputBaseName}.pass1.log.old"
			mv ${outputBaseName}.pass1.log ${outputBaseName}.pass1.log.old
		fi
	else
		break;
	fi
done
if [[ -n ${needToCleanupFifo} ]]; then
	cleanupFifo ;
fi
log_debug "chmod -x ${output}"
chmod -x ${output}
if [[ -n ${ogg} && ${audioInChannels} -gt 0 ]] ; then
	# need to merge the two together.
	log_debug "using ogmmerge to generate ogg media file"
	log_debug "ogmmerge -o ${outputBaseName}.ogm -D ${output} -A ${oggName}"
	ogmmerge -o ${outputBaseName}.ogm -A ${output} -D ${oggName}
fi
