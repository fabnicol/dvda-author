/**************************************************************************** 
  * yuvinactive.c
  * Copyright (C) 2003 Bernhard Praschinger 
  * 
  * Sets a area in the yuv frame to black
  * 
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation; either version 2 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "yuv4mpeg.h"

#define LUMA 16
#define CHROMA 128

/* some defintions */
struct area_s {
      int width;    /**< width of the area */
      int height;   /**< height of the area */
      int voffset;  /**< vertical offset from the top left boarder */
      int hoffset;  /**< horizontal offest from the top left boarder */
              };

struct color_yuv {
      int luma;       /**< the luma to use */
      int chroma_b;    /**< the chroma Cb to use */
      int chroma_r;    /**< the chroma Cr to use */
         };

int verbose = 1;

/* protoypes */
static void print_usage(char *progname);
void copy_area(struct area_s , int , int , uint8_t *frame[], int );
void process_commandline(int argc, char *argv[], struct area_s *inarea, 
 int *darker, int *copy_pixel, struct color_yuv *coloryuv, int *average_pixel);
void fillarea(char area[20], struct area_s *inarea);
void set_darker(struct area_s inarea, int horz, int vert, uint8_t *frame[], int darker);
void set_inactive(struct area_s inarea, int horz, int vert, uint8_t *frame[], struct color_yuv *coloryuv);
void set_yuvcolor(char area[20], struct color_yuv *coloryuv);
void average_area(struct area_s inarea, int horz, int vert, uint8_t *frame[], int average_pixel);
void average_block(int go_h, int go_w, int horz, int plane, int *orig_offset, uint8_t *frame[]);

/* Here we start the programm */

/** The typical help output */
static void print_usage(char *progname)
{
  fprintf(stderr, "%s usage: -i XxY+XOFF+YOFF\n\n",progname);
  fprintf(stderr, " -h -H            - print out this help\n");
  fprintf(stderr, " -i X+Y+XOFF+YOFF - the area which will be set inactive, from top left\n");
  fprintf(stderr, "                  - X=Width, Y=Height, XOFF=VerticalOffset, YOFF=HorizonalOffset\n");
  fprintf(stderr, " -s luma,Cb,Cr    - set the filling color in yuv format\n");
  fprintf(stderr, " -d num [1-100]   - set how much darker the area will be, in percent\n");
  fprintf(stderr, " -c num           - copy num sourrounding lines into the area\n");
  fprintf(stderr, " -a num           - use num pixles for on square to average the area\n");
  fprintf(stderr, "\n");
  exit(1);
}

/** Here we process the command line options */
void process_commandline(int argc, char *argv[], struct area_s *inarea, 
  int *darker, int *copy_pixel, struct color_yuv *coloryuv, int *average_pixel)
{
int c;
char area [20];

while ((c = getopt(argc, argv, "Hhv:i:s:d:c:a:")) != -1)
  {
  switch (c)
    {
       case 'v':
         verbose = atoi(optarg);
         if ( verbose < 0 || verbose > 2)
           print_usage(argv[0]);
         if (verbose == 2)
           mjpeg_info("Set Verbose = %i", verbose);
         break;
       case 'i':
         strncpy(area, optarg, 20); /* This part shows how to process */
         fillarea(area, inarea);   /* command line options */
         break;
       case 's':
         strncpy(area, optarg, 20); 
         set_yuvcolor(area, coloryuv);
         break;
       case 'd':
         *darker = atoi(optarg);
         break;
       case 'c':
         *copy_pixel = atoi(optarg);
         break;
       case 'a':
         *average_pixel = atoi(optarg);
         break;
       case 'H':
       case 'h':
         print_usage(argv[0]);
    }
  }

/* Checking if we have used the -i option */
if ( ((*inarea).height == 0) && ((*inarea).width == 0) )
  mjpeg_error_exit1("You have to use the -i option");

/* Checking the range of the darker -d option */
if ( (*darker < 0) || (*darker > 100) )
  mjpeg_error_exit1("You can only make the area 1-100 percent darker");
else
  mjpeg_info("Setting the area %i percent darker", *darker);

/* Checking the copy pixel option */
if (*copy_pixel != 0)
  {
  if ( ((*inarea).height/2) < *copy_pixel)
    {
       mjpeg_error("You can only copy half of the height into the area"); 
       mjpeg_error_exit1("lower the copy pixel value below: %i ", 
                   ((*inarea).height/2));
    }

  if ( (*copy_pixel % 2) != 0)
    mjpeg_error_exit1("you have to use a even number of lines to copy into the field");

  if ( ((*inarea).height % *copy_pixel) != 0)  
    mjpeg_error_exit1("the height has to be a multiply of the copy pixel value"); 

  mjpeg_info("Number of rows using for coping into the area %i", *copy_pixel);
  }

/* Checking the average pixel option */
if (*average_pixel != 0)
  {
    if ( (*average_pixel > (*inarea).height) || 
         (*average_pixel > (*inarea).width) )
      mjpeg_error_exit1("The pixles used for the average must be less the the inactive area");

  if ( (*average_pixel % 2) != 0)  
   mjpeg_error_exit1("you have to use a even number for the average pixels"); 

  mjpeg_info("Number of pixels used for averaging %i", *average_pixel);
  }
}
  
/** Here we set the color to use for filling the area */
void set_yuvcolor(char area[20], struct color_yuv *coloryuv)
{
int i;
unsigned int u1, u2, u3;

  i = sscanf (area, "%i,%i,%i", &u1, &u2, &u3);

  if ( 3 == i )
    {
      (*coloryuv).luma    = u1;
      (*coloryuv).chroma_b = u2;
      (*coloryuv).chroma_r = u3;
      if ( ((*coloryuv).luma > 235) || ((*coloryuv).luma < 16) )
        mjpeg_error_exit1("out of range value for luma given: %i, \n"
                          " allowed values 16-235", (*coloryuv).luma);
      if ( ((*coloryuv).chroma_b > 240) || ((*coloryuv).chroma_b < 16) )
        mjpeg_error_exit1("out of range value for Cb given: %i, \n"
                          " allowed values 16-240", (*coloryuv).chroma_b);
      if ( ((*coloryuv).chroma_r > 240) || ((*coloryuv).chroma_r < 16) )
        mjpeg_error_exit1("out of range value for Cr given: %i, \n"
                          " allowed values 16-240", (*coloryuv).chroma_r);

       mjpeg_info("got luma %i, Cb %i, Cr %i ", 
                 (*coloryuv).luma, (*coloryuv).chroma_b, (*coloryuv).chroma_r );
    }
  else 
    mjpeg_error_exit1("Wrong number of colors given, %s", area);
}

/** Here we cut out the number of the area string */
void fillarea(char area[20], struct area_s *inarea)
{
int i;
unsigned int u1, u2, u3, u4;

  /* Cuting out the numbers of the stream */
  i = sscanf (area, "%ix%i+%i+%i", &u1, &u2, &u3, &u4);

  if ( 4 == i)  /* Checking if we have got 4 numbers */
    {
       (*inarea).width = u1;
       (*inarea).height = u2;
       (*inarea).hoffset = u3;
       (*inarea).voffset = u4;

      if ( (((*inarea).width % 2) != 0) || (((*inarea).height % 2) != 0) ||
           (((*inarea).hoffset% 2)!= 0) || (((*inarea).voffset% 2) != 0)   )
        mjpeg_error_exit1("At least one argument no even number");

      if (verbose >= 1)
         mjpeg_info("got the area : W %i, H %i, Xoff %i, Yoff %i",
         (*inarea).width,(*inarea).height,(*inarea).hoffset,(*inarea).voffset); 
    }
  else 
    mjpeg_error_exit1("Wrong inactive sting given: %s", area);

}

/** Here we copy the surrounding information into the area */
void copy_area(struct area_s inarea, int horz, int vert, uint8_t *frame[], 
               int copy_pixel)
{
uint8_t *plane_l, *plane_cb, *plane_cr;
unsigned char *temp_pix_l, *temp_pix_cb, *temp_pix_cr;
int i,j, offset_pix, copy_offset_pix, chroma_lines;

temp_pix_l  = (unsigned char *)malloc(inarea.width);
temp_pix_cb = (unsigned char *)malloc(inarea.width/2);
temp_pix_cr = (unsigned char *)malloc(inarea.width/2);

plane_l  = frame[0];
plane_cb = frame[1];
plane_cr = frame[2];

/* In the first step we copy the luma data*/
offset_pix = (horz * inarea.voffset) + inarea.hoffset;
i=0;
while (i < (inarea.height/2)) /* copying lines from the top down */
  {
  copy_offset_pix = (horz * (inarea.voffset - copy_pixel) + inarea.hoffset);

  for (j = 0; j < copy_pixel; j++)
    {
       memcpy(temp_pix_l, (plane_l+(copy_offset_pix+(j*horz))), inarea.width);
       memcpy((plane_l+offset_pix), temp_pix_l, inarea.width);
       offset_pix += horz;
//mjpeg_info("copy_offset %i, offset %i von j %i, von i %i", copy_offset_pix, offset_pix, j ,i);
    } 
    i += copy_pixel; /* we copy more lines in one step */
  }

while (i < inarea.height) /* copying lines from the bottom up */
  {
  copy_offset_pix = (horz * (inarea.voffset + inarea.height) + inarea.hoffset);

  for (j = 0; j < copy_pixel; j++)
    {
       memcpy(temp_pix_l, (plane_l+copy_offset_pix+(j*horz)), inarea.width);
       memcpy((plane_l+offset_pix), temp_pix_l, inarea.width);
       offset_pix += horz;
// mjpeg_info("wert von offset_pix %i, von j %i, von i %i", copy_offset_pix, j ,i);
    } 
    i += copy_pixel; /* we copy more lines in one step */
  }

/* In the 2nd step we copy the chroma data */
offset_pix = (horz * inarea.voffset)/4 + (inarea.hoffset/2);

if ((inarea.height%4) != 0)
    chroma_lines = ((inarea.height/2 -1) /2);
else 
    chroma_lines = inarea.height /4;

i=0;
while ( i < chroma_lines)
  {
  copy_offset_pix = ((horz/2) * (inarea.voffset/2 + inarea.height/2)
                    + inarea.hoffset/2);
                                                                                
  for (j = 0; j < (copy_pixel/2); j++)
    {
       memcpy(temp_pix_cb,(plane_cb+copy_offset_pix+(j*horz)),(inarea.width/2));
       memcpy((plane_cb+offset_pix), temp_pix_cb, (inarea.width/2));
       memcpy(temp_pix_cr,(plane_cr+copy_offset_pix+(j*horz)),(inarea.width/2));
       memcpy((plane_cr+offset_pix), temp_pix_cr, (inarea.width/2));
       offset_pix += (horz/2);
    }
    i += (copy_pixel/2); /* we copy more lines in one step */
  }
while ( i < (inarea.height/2))
  {
  copy_offset_pix = ((horz/2) * (inarea.voffset/2 + inarea.height/2)
                    + inarea.hoffset/2);
                                                                                
  for (j = 0; j < (copy_pixel/2); j++)
    {
       memcpy(temp_pix_cb,(plane_cb+copy_offset_pix+(j*horz)),(inarea.width/2));       memcpy((plane_cb+offset_pix), temp_pix_cb, (inarea.width/2));
       memcpy(temp_pix_cr,(plane_cr+copy_offset_pix+(j*horz)),(inarea.width/2));       memcpy((plane_cr+offset_pix), temp_pix_cr, (inarea.width/2));
       offset_pix += (horz/2);
    }
    i += (copy_pixel/2); /* we copy more lines in one step */
  }

free(temp_pix_l);
temp_pix_l = NULL;
free(temp_pix_cb);
temp_pix_cb = NULL;
free(temp_pix_cr);
temp_pix_cr = NULL;
}

/** Here we set a n x n area to a average color */
void average_block(int go_h, int go_w, int horz, int plane, int *orig_offset, uint8_t *frame[])
{
unsigned char *temp_pix;
int i, j, summe, offset_pix;

temp_pix = (unsigned char*)malloc(1);
*temp_pix=0;
offset_pix = *orig_offset;
summe = 0;

for ( i=0; i < go_h; i++)
  {
    for (j=0; j < go_w; j++)
      {
         memcpy(temp_pix,(frame[plane]+offset_pix),1);
         summe += (int)*temp_pix;
         offset_pix++;
      } 
    offset_pix += horz - j;
  }

*temp_pix = (unsigned char)(summe / (go_h * go_w));
offset_pix = *orig_offset;
 
for (i = 0; i < go_h; i++)
  {
     for (j = 0; j < go_w; j++)
       memcpy((frame[plane]+offset_pix+j), temp_pix, 1);
        
     offset_pix += horz;
  }

free(temp_pix);
temp_pix=NULL;
}

/** Here we average the area */
void average_area(struct area_s inarea, int horz, int vert, uint8_t *frame[],
                  int average_pixel)
{
int orig_offset, sub_offset, plane;
int go_w, go_h, togo_w, togo_h;

orig_offset = (horz* inarea.voffset) + inarea.hoffset; 
sub_offset = ((horz/2) * (inarea.voffset/2)) + (inarea.hoffset/2);

go_w = average_pixel; 
go_h = average_pixel; 
togo_w = inarea.width - go_w ; /* we decrease here one block, else we  */
togo_h = inarea.height - go_h; /* that solves a problem in the while */ 
plane = 0; /* here we set that we wnat to use the first plane of the frame */

while ( go_h != 0 )
  { 
    while ( go_w != 0)
    {
     average_block(go_h, go_w, horz, plane, &orig_offset, frame);
     average_block(go_h/2, go_w/2, horz/2, plane+1, &sub_offset, frame);
     average_block(go_h/2, go_w/2, horz/2, plane+2, &sub_offset, frame);

     orig_offset += go_w;
     sub_offset += go_w/2;

     if ( (togo_w - go_w) >= 0 )
       togo_w -= go_w;   /* normal decrease of the square horicontal*/
     else if (togo_w != 0)
       {
       go_w = togo_w;    /* the last few pixels */
       togo_w = 0;
       }
     else
       go_w = 0;         /* this row finished averaging the pixels */
    }

   /* Here we go to the next row we have to average,first line+ (width-1line) */
   orig_offset = orig_offset+ (horz -inarea.width) + (horz *(average_pixel -1));
   sub_offset = sub_offset+ ((horz/2) - (inarea.width/2) +
                            ((horz/2) *(average_pixel/2) -1)); 
   /* we also have to reset the go_w variable, that cost me hours .... */ 
   go_w = average_pixel;
   togo_w = inarea.width - average_pixel ; 

   if ( (togo_h - go_h) >= 0 )
     togo_h -= go_h;   /* normal decrease of the square vertical */
   else if (togo_h != 0)
     {
     go_h = togo_h;    /* the last few pixels */
     togo_h = 0;
     }
   else
     go_h = 0;         /* this field finished averaging the pixels */

  }

}

/** Here we set the area darker, only touching luma */
void set_darker(struct area_s inarea, int horz, int vert, uint8_t *frame[],
                int darker)
{
int i, n, hoffset_pix; 
uint8_t *plane_l;
unsigned char *temp_pix;
unsigned char *pix;
float dark;

dark  = 1 - (darker* 0.01);

temp_pix = (unsigned char *)malloc(1);
pix = (unsigned char *)malloc(1);
*temp_pix=0;
*pix=0;

/* First we do the luma */
plane_l  = frame[0];
hoffset_pix = (horz * inarea.voffset) + inarea.hoffset;

for (i = 0; i < inarea.height; i++)
  {
    for (n = 0; n < inarea.width; n++)
      {
         memcpy( temp_pix, (plane_l+hoffset_pix), 1);
         *pix = 16 + (int)((*temp_pix - 16) * dark);

         if (*pix < 16 ) /* We take care that we don't produce values */
           *pix = 16;    /* which should not be used */

         memset( (plane_l + hoffset_pix), *pix, 1);

         hoffset_pix++;
      }
    hoffset_pix += (horz - inarea.width) ;
  }


/* And then the Cr and Cb */
plane_l  = frame[1];
hoffset_pix = ((horz/2) * (inarea.voffset/2)) + (inarea.hoffset/2);

for (i = 0; i < (inarea.height/2); i++)
  {
    for (n = 0; n < (inarea.width/2); n++)
      {
         memcpy( temp_pix, (plane_l+hoffset_pix), 1);
         *pix = 128 + (int)((*temp_pix - 128) * dark);
         memset( (plane_l + hoffset_pix), *pix, 1);
         hoffset_pix++;
      }
    hoffset_pix += ((horz - inarea.width) /2) ;
  }

plane_l  = frame[2];
hoffset_pix = ((horz/2) * (inarea.voffset/2)) + (inarea.hoffset/2);

for (i = 0; i < (inarea.height/2); i++)
  {
    for (n = 0; n < (inarea.width/2); n++)
      {
         memcpy( temp_pix, (plane_l+hoffset_pix), 1);
         *pix = 128 + (int)((*temp_pix - 128) * dark);
         memset( (plane_l + hoffset_pix), *pix, 1);
         hoffset_pix++;
      }
    hoffset_pix += ((horz - inarea.width) /2) ;
  }

free(temp_pix);
temp_pix=NULL;
free(pix);
pix=NULL;
}

/** Here is the first stage of setting the stream to black */
void set_inactive(struct area_s inarea, int horz, int vert, uint8_t *frame[],
                  struct color_yuv *coloryuv)
{
int i, hoffset_pix;
uint8_t *plane_l, *plane_cb, *plane_cr;

plane_l = frame[0];
plane_cb= frame[1];
plane_cr= frame[2];

/* Number of pixels for the luma */
hoffset_pix = (horz * inarea.voffset) + inarea.hoffset;

for (i = 0; i < inarea.height; i++) /* Setting the Luma */
  {
    memset( (plane_l + hoffset_pix), (*coloryuv).luma , (inarea.width) );
    hoffset_pix += horz;
  } 

/* Number of pixels chroma */
hoffset_pix = ((horz / 2)  * (inarea.voffset/2) ) + (inarea.hoffset / 2 );

for (i = 0; i < (inarea.height/2); i++) /*Setting the chroma */
  {
    memset( (plane_cb + hoffset_pix), (*coloryuv).chroma_b, (inarea.width/2) );
    memset( (plane_cr + hoffset_pix), (*coloryuv).chroma_r, (inarea.width/2) );
    hoffset_pix += (horz/2);
  }

}

/** MAIN */
int main( int argc, char **argv)
{
int i, frame_count;
int horz, vert;      /* width and height of the frame */
uint8_t *frame[3];  /*pointer to the 3 color planes of the input frame */
struct area_s inarea;
struct color_yuv coloryuv;
int input_fd = 0;    /* std in */
int output_fd = 1;   /* std out */
int darker = 0;  /* how much darker should the image be */
int copy_pixel = 0; /* how much pixels we should use for filling up the area */
int average_pixel = 0; /* how much pixel to use for average */
y4m_stream_info_t istream, ostream;
y4m_frame_info_t iframe;

inarea.width=0; inarea.height=0; inarea.voffset=0; inarea.hoffset=0;

coloryuv.luma    = LUMA;  /*Setting the luma to black */
coloryuv.chroma_b = CHROMA; /*Setting the chroma to center, means white */
coloryuv.chroma_r = CHROMA; /*Setting the chroma to center, means white */

(void)mjpeg_default_handler_verbosity(verbose);

  /* processing commandline */
  process_commandline(argc, argv, &inarea, &darker, &copy_pixel, &coloryuv,
                      &average_pixel);

  y4m_init_stream_info(&istream);
  y4m_init_stream_info(&ostream);
  y4m_init_frame_info(&iframe);

  /* First read the header of the y4m stream */
  i = y4m_read_stream_header(input_fd, &istream);
  
  if ( i != Y4M_OK)   /* a basic check if we really have y4m stream */
    mjpeg_error_exit1("Input stream error: %s", y4m_strerr(i));
  else 
    {
      /* Here we copy the input stream info to the output stream info header */
      y4m_copy_stream_info(&ostream, &istream);

      /* Here we write the new output header to the output fd */
      y4m_write_stream_header(output_fd, &ostream);

      horz = y4m_si_get_width(&istream);   /* get the width of the frame */
      vert = y4m_si_get_height(&istream);  /* get the height of the frame */

      if ( (inarea.width + inarea.hoffset) > horz)
      mjpeg_error_exit1("Input width and offset larger than framewidth,exit");
 
      if ( (inarea.height + inarea.voffset) > vert)
      mjpeg_error_exit1("Input height and offset larger than frameheight,exit");

      /* Here we allocate the memory for on frame */
      frame[0] = malloc( horz * vert );
      frame[1] = malloc( (horz/2) * (vert/2) );
      frame[2] = malloc( (horz/2) * (vert/2) );

      /* Here we set the initial number of of frames */
      /* We do not need it. Just for showing that is does something */
      frame_count = 0 ; 

      /* This is the main loop here can filters effects, scaling and so 
      on be done with the video frames. Just up to your mind */
      /* We read now a single frame with the header and check if it does not
      have any problems or we have alreaddy processed the last without data */
      while(y4m_read_frame(input_fd, &istream, &iframe, frame) == Y4M_OK)
        {
           frame_count++; 

           /* You can do something usefull here */
           if (darker != 0)
             set_darker(inarea, horz, vert, frame, darker);
           else if (copy_pixel != 0)
             copy_area(inarea, horz, vert, frame, copy_pixel);
           else if (average_pixel != 0)
             average_area(inarea, horz, vert, frame, average_pixel);
           else
             set_inactive(inarea, horz, vert, frame, &coloryuv);

           /* Now we put out the read frame */
           y4m_write_frame(output_fd, &ostream, &iframe, frame);
        }

      /* Cleaning up the data structures */
      y4m_fini_stream_info(&istream);
      y4m_fini_stream_info(&ostream);
      y4m_fini_frame_info(&iframe);

    }

    /* giving back the memory to the system */
    free(frame[0]);
    frame[0] = 0;
    free(frame[1]);
    frame[1] = 0;
    free(frame[2]);
    frame[2] = 0;

  exit(0); /* exiting */ 
}
