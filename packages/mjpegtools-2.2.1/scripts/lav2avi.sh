#!/bin/bash

################################################################################
## CONFIGURATION START #########################################################
################################################################################

# Choose between 650, 700, 800
# All other values mean maximum
preferedSize=700

# Additional parameter for mencoder. For example croping.
#encoderParam="-vop crop=690:570:20:6"

################################################################################
## CONFIGURATION END ###########################################################
################################################################################

# Application variables
usage=1
appB=""
output=${1%.eli}.avi

head "$1" 2> /dev/null | fgrep -q "LAV Edit List" && {
   if [ -f $output ]; then
       echo "ERROR: Remove $output file and restart."
       exit 1
   fi
   usage=0
   #
   # Preparation
   #
   rm -f frameno.avi lavc_stats.txt fifo.wav
   mkfifo fifo.wav

   #
   # Pass 1 (Audio encoding)
   #
   echo "#########################"
   echo "# Entering to 1st phase #"
   echo "#########################"
   lav2wav $1 > fifo.wav &
   lav2yuv $1 | mencoder $encoderParam -ovc frameno -oac mp3lame -audiofile fifo.wav -audio-demuxer 17 -o frameno.avi - | tee out1.txt

   #
   # Pass 2
   #
   recomBitrate1=`cat out1.txt | fgrep "Recommended video bitrate for ${preferedSize}MB" | cut -c 41-`
   if [ -n "$recomBitrate1" ]; then
       appB=":vbitrate=$recomBitrate1"
   fi
   echo "##########################################"
   echo "# Entering to 2nd phase with bitrate $recomBitrate1 #"
   echo "##########################################"

   lav2yuv $1 | mencoder $encoderParam -ovc lavc -lavcopts vcodec=mpeg4:vpass=1$appB -oac copy -o /dev/null - > out2.txt | tee out2.txt

   #
   # Pass 3
   #
   recomBitrate2=`cat out2.txt | fgrep "Recommended video bitrate for ${preferedSize}MB" | cut -c 41-`
   if [ -n "$recomBitrate2" ]; then
       recomBitrate1=$recomBitrate2
       appB=":vbitrate=$recomBitrate1"
   fi
   echo "##########################################"
   echo "# Entering to 3rd phase with bitrate $recomBitrate1 #"
   echo "##########################################"

   rm -f $output
   lav2yuv $1 | mencoder $encoderParam -ovc lavc -lavcopts vcodec=mpeg4:vpass=2$appB -oac copy -o $output -
   rm -f fifo.wav out1.txt out2.txt
}

if [ $usage -eq 1 ]; then
   echo -e "USAGE:\t`basename $0` filename.eli"
   echo -e "\n\tfilename - MJPEG Tools lav editing file\n"
   echo -e "EXAMPLE:\n\t`basename $0` SecondFilm.eli\n"
fi
