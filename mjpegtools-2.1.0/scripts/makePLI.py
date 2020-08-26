#Make a lavpipe pli file from a simpler input file.
#Use python makePLI.py --man for detailed usage.

#Assumptions:
#   All transitions are the same length.
#   All transitions have the same characteristics.
#   All input streams have the same characteristics.

#  Copyright (C) 2002, Bob Matlin <bob.kannon@excite.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You probably received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



import os.path
import sys
import string

class Clip:
    #default values
    __offsetUnits = 's'
    __lengthUnits = 's'
    __framesPerSec = 29.97 #NTSC
    __lenOrStop = 'length' #determines whether or not the lenOrStop param specifies length or stop time/position
    
    def __init__(self,fileName=None, offset=None, lenOrEnd=None):
        """
        The default constructor permits changing default values.
        
        fileName - the name of the file; may contain path information

        offset - the time at which to start playing expressed in specified units;
        unit default is seconds

        lenOrStop - the length or the end position of the clip expressed in specified
        units; unit default is seconds 
        """
        if (fileName == None): return

        noffset = 0
        if (Clip.__offsetUnits == 's'): noffset = float(offset)
        elif (Clip.__offsetUnits == 'f'): noffset = int(offset)

        nlenOrEnd = 0
        if (Clip.__lengthUnits == 's'): nlenOrEnd = float(lenOrEnd)
        elif (Clip.__lengthUnits == 'f'): nlenOrEnd = int(lenOrEnd)
        
        self.fileName = fileName
        self.offsetTime = self.__getOffsetTime(noffset)
        self.lengthTime = self.__getLengthTime(nlenOrEnd)
        framesPerSec = self.__framesPerSec
        self.offsetFrames = self.__getOffsetFrames(noffset)
        self.lengthFrames = self.__getLengthFrames(nlenOrEnd)
        if (self.offsetFrames > 0): self.lastFrame = self.offsetFrames -1 
        else: self.lastFrame = self.offsetFrames
        self.clipEnd = self.offsetFrames + self.lengthFrames
        ##print "fileName: " + self.fileName + ", clipEnd: " + str(self.clipEnd)


    def setOffsetUnits(self,units):
        """
        units - one of s (seconds) or f (frames)
        """
        Clip.__offsetUnits = units


    def setLengthUnits(self,units):
        """
        units - one of s (seconds) or f (frames)
        """
        Clip.__lengthUnits = units


    def setLenOrStopType(self,lenOrStop):
        """
        lenOrStop - either 'length' or 'stop'
        """
        if (lenOrStop != 'length' and lenOrStop != 'stop'):
            raise "(makePLI.py) Invalid length/stop specifier; must be one of [length,stop]."
        Clip.__lenOrStopType = lenOrStop


    def __getOffsetTime(self,offset):
        """
        offset - the clip offset, expressed in specified class default units
        """
        if (Clip.__offsetUnits == 's'): return(offset)
        elif (Clip.__offsetUnits == 'f'): return(offset/Clip.__framesPerSec)


    def __getOffsetFrames(self,offset):
        """
        offset - the clip offset, expressed in specified class default units
        """
        if (Clip.__offsetUnits == 's'): return(int(offset * Clip.__framesPerSec))
        elif (Clip.__offsetUnits == 'f'): return(offset)


    def __getLengthTime(self,lenOrEnd):
        """
        lenOrEnd - the length or end of clip, expressed in class default units
        """
        if (Clip.__lengthUnits == 's'): return(lenOrEnd)
        elif (Clip.__lengthUnits == 'f'): return(lenOrEnd/Clip.__framesPerSec)


    def __getLengthFrames(self,lenOrEnd):
        """
        lenOrEnd - the length or end of clip, expressed in class default units
        """
        f = 0
        if (Clip.__lengthUnits == 's'): f = int(lenOrEnd * Clip.__framesPerSec)
        elif (Clip.__lengthUnits == 'f'): f = lenOrEnd
        if (Clip.__lenOrStopType == 'stop'): f = f - self.offsetFrames
        return(f)


class PPLIFile:
    """
    A PPLIFile is the file containing the directives and file list needed to
    make a pli file.
    """
    def __init__(self):
        self.__lineNum = 0

        
    def handleLine(self,line):
        """
        Takes action on a line obtained from a ppli file.

        return - a Clip if the line contained clip data, None otherwise 
        """
        self.__lineNum = self.__lineNum + 1

        s = line.strip()
        if (len(s) == 0): return(None)
        
        if (s.find('#') == 0): return(None) #comment lines start with '#'; ignore them
        if (s.find('$') == 0):
            #this is a clip directive
            return(self.handleDirective(s))
                
        #else this is a clip
        a = s.split()
        if (len(a) != 3):
            raise Exception("(makePLI.py - definition file line " + str(self.__lineNum) + "): A clip line must look like: fileName offset length.")
        clip = Clip(a[0],a[1],a[2])
        return(clip)


    def handleDirective(self,line):
        """
        Take an action on a directive line.
        """
        s = line
        a = s.split()
        if (a[0] == '$format'):
            if (len(a) != 3):
                raise Exception("(makePLI.py - definition file line " + str(self.__lineNum) + "): a $format directive must look like: $format offset:<units> length:<units>")

            else: 
                clip = Clip()
                #get the offset units
                aa = a[1].split(":")
                if (len(aa) != 2):
                    raise Exception("(makePLI.py - definition file line " + str(self.__lineNum) + "): a $format directive must look like: $format offset:<units> length:<units>")
                clip.setOffsetUnits(aa[1])

                #get the length/stop type and units
                aa = a[2].split(":")
                if (len(aa) != 2):
                    raise Exception("(makePLI.py - defnition file line " + str(self.__lineNum) + "): a $format directive must look like: $format offset:<units> length:<units>")
                try:
                    clip.setLenOrStopType(aa[0])
                except:
                    (t,v,tb) = sys.exc_info()
                    msg = "(makePLI.py - definition file line " + str(self.__lineNum) + "): " + str(t)
                    raise msg
            
            clip.setLengthUnits(aa[1])
            return(None)
        else:
            raise Exception("(makePLI.py - definition file line " + str(self.__lineNum) + "): Invalid directive; valid directives: $format.")


def getUsage():
    s = "\nUsage: python makePLI.py [--help] [--man] --file fileName [--transLen length]\n"
    s = s +  "where 'fileName' is the name of the definition file \n"
    s = s +  "and 'length' is the number of frames in each transition.\n"
    s = s + "--man prints a detailed usage document;\n"
    s = s + "--help prints this message.\n"
    return(s)


def getMan():
    s = getUsage()
    s = s + "\nOverview\n\n"
    s = s + "This program produces an output file that uses the mjpeg tools lavpipe, \n"
    s = s + "lav2yuv, and transist.flt. It has been used to create the video sequence\n"
    s = s + "for a video montage. When the output file is processed with lavpipe, the\n"
    s = s + "result is an avi file containing video only; audio can be added with lavaddwav.\n\n"

    s = s + "Assumptions:\n"
    s = s + "\tall transitions are the same length;\n"
    s = s + "\tall transitions have the same characteristics;\n"
    s = s + "\tall input streams have the same characteristics.\n\n"


    s = s + "Process\n\n"
    s = s + "Given an input definition file (referred to as a ppli file - pre pli), makePLI.py\n"
    s = s + "will produce an output file (referred to as a pli file; see man lavpipe) suitable\n"
    s = s + "for submission to lavpipe. The output of lavpipe will be an mjpeg avi file with\n"
    s = s + "equal transitions between the given video clips.\n\n"

    st = 1
    s = s + str(st) + " Create the definition file\n\n"
    s = s + "The definition file consists of: directives that control the behavior of makePLI.py;\n"
    s = s + "clips that specify the video clip sources and order; and comments. Directives \n"
    s = s + "must preceed clips and comments may be placed anywhere.\n\n"
    sst = 1
    s = s + str(st) + "." + str(sst) + " Directives\n\n"
    s = s + "Directives are lines that begin with '$<directiveName>'. Currently there is only one\n"
    s = s + "directive, $format:\n\n"
    s = s + "$format offset:<units> <lenOrEnd type>:<units>\n"
    s = s + "\twhere <units> is either 's' (seconds) or 'f' (frames);\n"
    s = s + "\t<lenOrEnd type> is either 'length', meaning that the lenOrEnd value\n"
    s = s + "\tspecifies the clip length, or 'stop' meaning that the lenOrEnd value\n"
    s = s + "\tspecifies the end of the clip relative to the offset.\n"
    s = s + "If there is more than one $format line, the last one takes precedence.\n\n"

    s = s + "Format Examples:\n\n"
    s = s + "\t$format offset:s length:s\n"
    s = s + "\t$format offset:f length:s\n"
    s = s + "\t$format offset:f stop:f\n\n"

    st = st + 1
    s = s + str(st) + " Clips\n\n"
    s = s + "Clips are lines that specify the source files, the position to start\n"
    s = s + "in the clip, and the position to end in the clip:\n\n"
    s = s + "<filename> <offset> <lenOrEnd>\n"
    s = s + "\twhere <filename> is the name of the lav source;\n"
    s = s + "\t<offset> is the starting position in the clip as determined by the $format;\n"
    s = s + "\t<lenOrEnd> is the ending position in the clip as determined by the $format.\n\n"

    s = s + "Both start and end may be expressed in frames or seconds as determined by the $format.\n\n"

    s = s + "Clip Examples:\n\n"
    s = s + "\tintro.avi 0 25\n"
    s = s + "\tmain.avi 15.37 30.25\n\n"

    s = s + "Note that the numeric values may be either integers or floats; the proper\n"
    s = s + "type is determined by the $format; if the type is frames, the value should be an int;\n"
    s = s + "if the type is seconds, the value can be a float.\n\n"

    s = s + "A full example\n\n"
    s = s + "Given the input file montage.ppli containing the following:\n\n"
    s = s + "#a sample ppli file\n"
    s = s + "$format offset:f length:s\n"
    s = s + "intro.avi 0 30\n"
    s = s + "clip.avi 61 25.37\n"
    s = s + "end.avi 0 30\n\n"

    s = s + "Running this command from a shell:\n\n"
    s = s + "\t$ python makePLI.py --file montage.ppli > montage.pli\n\n"

    s = s + "produces this output:\n\n"
    s = s + "LAV Pipe List\n"
    s = s + "NTSC\n"
    s = s + "3\n"
    s = s + "lav2yuv -v 0 -o $o -f $n intro.avi\n"
    s = s + "lav2yuv -v 0 -o $o -f $n clip.avi\n"
    s = s + "lav2yuv -v 0 -o $o -f $n end.avi\n"
    s = s + "868\n"
    s = s + "1\n"
    s = s + "0 0\n"
    s = s + "-\n"
    s = s + "30\n"
    s = s + "2\n"
    s = s + "0 869\n"
    s = s + "1 60\n"
    s = s + "transist.flt -v 0 -s $o -n $n -o 0 -O 255 -d 30\n"
    s = s + "699\n"
    s = s + "1\n"
    s = s + "1 90\n"
    s = s + "-\n"
    s = s + "30\n"
    s = s + "2\n"
    s = s + "1 790\n"
    s = s + "2 0\n"
    s = s + "transist.flt -v 0 -s $o -n $n -o 0 -O 255 -d 30\n"
    s = s + "868\n"
    s = s + "1\n"
    s = s + "2 30\n"
    s = s + "-\n\n"

    s = s + "Error reporting\n\n"
    s = s + "Some attempt is made to spot errors. If a specified clip is not long\n"
    s = s + "enough to satisfy the required lengths, you will get an error message like:\n\n"
    s = s + "(makePLI.py) - intro.avi is too short; total needed: 51 frames; actual length: 44 frames\n\n"

    s = s + "Notes\n\n"
    s = s + "It seems that there is a limit on the number of clips that can be\n"
    s = s + "processed at once. I do not know where the limitation lies. On my system\n"
    s = s + "the limit seems to be about 15 clips. To get around this problem\n"
    s = s + "I break up the lists into two files, then combine the result with\n"
    s = s + "another ppli and lavpipe operation.\n\n"

    s = s + "Author\n\n"
    s = s + "The program and the documentation were written by Bob Matlin <bob.kannon@excite.com>.\n\n"
    return(s)



#main

if (len(sys.argv) < 2 or '--help' == sys.argv[1]):
    print(getUsage())
    sys.exit(0)


#default values
fileName = None
transLen = 30 #each transition is 30 frames

#get arguments
for i in range(len(sys.argv)):
    if (sys.argv[i] == '--file'):
        i = i + 1
        fileName = sys.argv[i]
    elif (sys.argv[i] == '--transLen'):
        i = i + 1
        transLen = int(sys.argv[i])
    elif (sys.argv[i] == '--man'):
        s = getMan()
        print s
        sys.exit(0)
        
if (fileName == None):
    s = "(makePLI.py) - no file specified.\n\n"
    s = s + getUsage()
    raise Exception(s)

if (not os.path.exists(fileName)):
    s = "(makePLI.py) - given file does not exist: '" + str(fileName) + "'\n\n"
    s = s + getUsage()
    raise Exception(s)

    
#read file
f = open(fileName)

flist = []
l = "a"
ppliFile = PPLIFile()
while (l):
    l = f.readline()
    clip = ppliFile.handleLine(l)
    if (clip != None):
        flist.append(clip)
    

#the pli header
s = 'LAV Pipe List\n'
s += 'NTSC\n'
s += str(len(flist))
print s
#the stream list
st = 'lav2yuv -v 0 -o $o -f $n '
for i in range(len(flist)):
    c = flist[i]
    s = st + c.fileName
    print s


#build sequences
#first clip
idx = 0
##print "sequence " + str(idx)
c = flist[idx]
if (1 + transLen >= c.lengthFrames):
    tot = 1 + transLen
    raise "(makePLI.py) - " + c.fileName + " is too short; total needed: " + str(tot) + " frames; actual length: " + str(c.lengthFrames) + " frames"
l = c.lengthFrames - transLen - 1
s = str(l) + '\n'
s += '1\n'
s += str(idx) + ' ' + str(c.lastFrame) + '\n'
c.lastFrame += l
s += '-'
print s

#the middle sequences
transist = 'transist.flt -v 0 -s $o -n $n -o 0 -O 255 -d '
seqn = -1
for idx in range(len(flist) - 1):
    #the transition
    seqn += 2
    ##print "sequence " + str(seqn)
    l = transLen
    c1 = flist[idx]
    c2 = flist[idx + 1]
    if (1 + (2*transLen + 1)  > c2.lengthFrames):
        #there is a 1 frame minimum for this clip on its own; the extra 1 is for 1 frame at the end
        tot = 1 + 2*transLen + 1
        raise "(makePLI.py) - seq " + str(seqn) + ": " + c2.fileName + " is too short; needed: " + str(tot) + ", available: " + str(c2.lengthFrames)
    s = str(l) + '\n'
    s += '2\n'
    s += str(idx) + ' ' + str(c1.lastFrame + 1) + '\n'
    c1.lastFrame += l 
    if (c1.lastFrame >= c1.clipEnd):
        raise "(makePLI.py) - " + c1.fileName + " will go past the clip end."
        
    s += str(idx + 1) + ' ' + str(c2.lastFrame) + '\n'
    c2.lastFrame += l
    s += transist + str(l)
    print s

    if (idx + 1 == len(flist) - 1): continue
    #the next clip
    ##print "sequence " + str(seqn + 1)
    l = c2.lengthFrames - (2*transLen + 1)
    s = str(l) + '\n'
    s += '1\n'
    s += str(idx + 1) + ' ' + str(c2.lastFrame) + '\n'
    c2.lastFrame += l
    if (c2.lastFrame >= c2.clipEnd):
        raise "(makePLI.py) - " + c2.fileName + " will go past the clip end."
    s += '-'
    print s

#last frame
##print "sequence: " + str(seqn + 2)
idx = len(flist) - 1
c = flist[idx]
l = c.lengthFrames - transLen - 1
s = str(l) + '\n'
s += '1\n'
s += str(idx) + ' ' + str(c.lastFrame) + '\n'
c.lastFrame += l
if (c.lastFrame >= c.clipEnd):
    raise "(makePLI.py) - " + c.fileName + " will go past the clip end."
s += '-'
print s

#debug
##for c in flist:
##    print "file: " + c.fileName + ", end: " + str(c.clipEnd) + ", lastFrame: " + str(c.lastFrame)
