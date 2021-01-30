# dvda-author version 20.10 (Oct 2020)  

&nbsp;  
        	   
## 1. Quick start  

### 1.1 Building the toolchain from source:

        ./autogen   
        ./configure    

with elevated rights under *nix platforms:
   
        (sudo) make && (sudo) make install   
   
The patched toolchain source code is inlined in the repository
downloaded with git. If you would like to refresh the toolchain 
using the vanilla packages and applying the patches, you can do
it by configuring as follows:

         ./configure --enable-all-all

Running ./configure should not be necessary under a recent Ubuntu platform
(and possibly most GNU/Linux platforms) as a Makefile is provided:  
just run sudo make && sudo make install.   
You can also download each 
[toolchain package from 
Sourceforge](https://sourceforge.net/projects/dvd-audio/files/dvda-author/Dependencies/)

Mind to apply the patches in the **patches** subdirectory of this URL.
	
The building system has been tested under linux, Free-BSD
and Cygwin. For Windows, you can use the codeblocks project to (re)build
the target application once the dependencies have been built using Cygwin. 
It is advised to use the [MSYS2 port](https://www.msys2.org/) of the GNU GCC 
compiler for `dvda-author` itself, yet most dependencies should be built using
Cygwin. Builds with a recent MSYS2 port of gcc and the codeblocks project yield 
better results than with Cygwin.
 
### 1.2 Running dvda-author-dev

See file **commandline.txt** for usage examples.  See file 
**dvda-author-(version).html** 
for usage details.

As this is a local deployment package, if you create menus or discs, add:

        --bindir=/path/to/executable/dependencies    
		
to your  commandline when running `dvda-author-dev`. If you built the whole 
toolchain from source using the configure script, this directory is **local/bin** 
under the package root directory. You may place this directory elsewhere, 
changing the `--bindir` path argument accordingly.      
    
	
Some common examples for Windows builds:     

1. Simple two-group disc, just the disc system files:  

        local\bin\dvda-author.exe -g C:\Docs\a2_24_44.wav \ 
		-g C:\Docs\a2_24_48.wav -W -o output   

2. Simple two-group disc, just the ISO:  

        local\bin\dvda-author.exe -g C:\Docs\a2_24_44.wav \
		-g C:\Docs\a2_24_48.wav -W --mkisofs

**dvd.iso** will be created under **dvda-author\\temp** as well as DVD-audio files.      

3. Simple two-group disc, to be burned on a DVD:  

        local\bin\dvda-author.exe -g C:\Docs\a2_24_44.wav \
		-g C:\Docs\a2_24_48.wav -W --cdrecord

You might have to run this with administrative rights in some cases. 
If you have several DVD writers, you may have to change your disc to the one
that will be automatically selected.

4. Three-group disc, just the disc system files, with a variety of audio formats
 as input (-S), and increased verbosity (-t):  

        local\bin\dvda-author.exe -g C:\Docs\a2_24_44.wav \
		-g C:\Docs\a4_16_96.flac C:\Docs\a.aiff C:\Docs\b.au \
		-g C:\Docs\a2_24_48.flac -W -S -t

DVD files with be created under **dvda-author\\temp**

5. Simple two-group disc with DVD top menu, burned to disc:  

        local\bin\dvda-author.exe -g C:\Docs\a2_24_44.wav \
		-g C:\Docs\a2_16_192.wav -W --bindir local\bin -t \
		--topmenu  --cdrecord

You might have to run this with administrative rights in some cases. 

6. Complex two-group disc with DVD top menu, from a variety of audio formats, and
an extra group/track link from the DVD-Audio zone to the pre-authored DVD-Video
zone of the disc, and burn it:  

        local\bin\dvda-author.exe -g Docs\a2_24_44.wav \
		-g Docs\a2_16_192.flac Docs\a2_24_44.aiff -S -W \
		--bindir local\bin --topmenu --cdrecord \
		--videodir C:\my_DVD\VIDEO_TS -T 1    

7. Complex 1-group disc with DVD top menu, which remains on-screen on playback,
and two extra group/track links from the DVD-Audio zone to the pre-authored 
DVD-Video zone of the disc, just the ISO:  

        local\bin\dvda-author.exe -g Docs\a*.{wav,flac,aiff} -S -W \
		--bindir local\bin --topmenu  --mkisofs \
		--videodir C:\my_DVD\VIDEO_TS -T 1 -T 2  


## 2. Details

### 2.1. Description

`dvda-author` creates high-definition DVD-Audio disc structures from
either WAV, FLAC/Ogg FLAC, or SoX-supported
audio formats (.au, .aiff, .gsm, etc.)

This version supports multichannel audio up to 5.1 and implements experimental
support for MLP (mono, stereo and multichannel), 
[using the `ffmpeg`API](https://ffmpeg.org), currently only for 16-bit audio 
and multichannel up to 6 channels, 88.2 kHz.   

The software also supports *hybrid* or *universal* DVD-Audio/Video disc
structures, which contain both DVD-Audio and DVD-Video zones.

A navigation feature makes it possible to start playing a video title
from within audio zone or an audio title from within video zone.

Inctructions for use are given in the following files:

  + man page: **dvda-author-(version).1**   
  + html page: **dvda-author-(version).html**   
  + **EXAMPLES**   
  + **TROUBLESHOOT** (user-mentioned issues)   
  + **HOWTO.conf** (for configuration files)   
  + **dvda-author.conf.example**   
  
See website for further details.

### 2.2. Website

The latest version of the software is available
on the [DVD-Audio tools website](https://dvd-audio.sourceforge.io)


### 2.3. Build environment

A GNU-style make system is shipped with this package.

Details on building issues are given in **INSTALL**.

### 2.4. Files

Source code directories are notably:

  + **src**:  main source code files  

  + **libutils**:  Source code for the libc_utils static library   

  + **libats2wav**:  Source code for ats2wav, a DVD-Audio titleset extracter 
  that converts .AOB files into .wav audio files.   

  + **libfixwav**:  Source code for a special library that checks and
  fixes .wav file headers    

  + **libiberty**:  GNU libc code for function replacement(alternative to
  -liberty). Mainly for aging building systems. Will probably be deprecated
  in the near future.   

  + **m4, m4.extra, m4.extra.dvdauthor** M4 macros, especially:   

	* **dvda.m4**:  general-purpose macros that could be reused in other 
	projects.    
	* **auxiliary.m4**: shorter general-purpose macros.   
	* **dependencies.m4**: dvda-author-specific parameters submitted to 
	other M4 macros (using lists).    
	* Other third-party macros may be copied here as
	some platforms may not include them and they are used by nested
	configure scripts.   

Builds are provided in directory **local.xyz** for various platforms.  

Important DATA-type files are:     

+ **dvda-author(version).1**: Man(1) page   
+ **dvda-author(verion).html**: HTML version of the man page   
+ **dvda-author.desktop**: KDE/gnome desktop configuration file     
+ **dvda-author.conf**: configuration file (default command-line options)    
+ **dvda-author.png or .ico** images for desktop.   
+ **dvda-author.nsi**: NSIS script for generating Windows installer.   
+ **BUGS**     
+ **LIMITATIONS**    
+ **HOWTO.conf**: Help file on how to generate a configuration file and use it     
+ **dvda-author.conf.example**: Example of a complex configuration file project   
+ **commandline.txt**:  Complex command line examples        

### 2.5. Installation

This application performs limited patching of other open source
utilities used for creating audio and video menus (`dvdauthor`),
analyzing audio content (`sox`) or creating a special ISO dic image
(`mkisofs`). The patch against `mkisofs` was integrated into recent code by its 
author and maintainer (J. Schilling). A Cygwin patch has been added to
`help2man`. 

The special patches used in this process are automatically downloaded
and applied by the autotools configure script.

If you do not use the GNU build system, check that the configuration
file **dvda-author.conf** is in the same directory as the main executable
(see **INSTALL** for other options).
  
### 2.6. Credits      
    
The core application for stereo audio authoring was created
by David Champan in 2005.  
It was extented to multichannel (with T&L Feldkamp's help),
by Fabrice Nicol between 2008 and 2020. 
Menu-authoring capabilities were added by Fabrice Nicol in 2010.   
A complete redesign of DVD-Audio extraction to PCM, initially written
by D. Chapman (ats2wav, archived) was performed in 2019 by Fabrice Nicol.  
Authoring of DVD-Audio with MLP tracks, with experimental
encoding/decoding of PCM files to/from MLP was added in 2020.

Thanks to:   

  Bahman Negahban for useful hints on FLAC 1.2.1 integration  
  Tomasz Belinea for testing version 08.07.  
  Lee Feldkamp for testing Windows executables and adding experimental audio
  options (-j,-s)  
  Lee and Tim Feldkamp for authoring experimental multichannel support in 2008 
  (corrected and redesigned by F. Nicol in 2018-2020).   
  Jörg Schilling for adding the `-dvd-audio/-dvd-hybrid` feature patch to 
  **cdrtools**  
  and all other authors (see file **AUTHORS** for details).

### 2.7. News

See file **NEWS**

### 2.8. Bugs

See file **BUGS**

### 2.9. Mailing list

   **dvd-audio-devel@lists.sourceforge.net**

