/*
 * $Id: y4mhist.c,v 1.13 2013/07/17 23:59:09 sms00 Exp $
 *
 * Simple program to print a crude histogram of the Y'CbCr values for YUV4MPEG2
 * stream.  Usually used with a small number (single) of frames but that's not
 * a requirement.  Reads stdin until end of stream and then prints the Y'CbCr
 * counts.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yuv4mpeg.h>

unsigned long long y_stats[256], u_stats[256], v_stats[256];
unsigned long long fy_stats[256], fu_stats[256], fv_stats[256];
unsigned long long ly_stats[256], lu_stats[256], lv_stats[256];
unsigned char vectorfield[260][260];
int scalepercent;
/* the l?_stats means till last frame, and f?_stat means the actual frame */

/* For the graphical history output */
#ifdef HAVE_SDLgfx
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

/* Defining some colors */
#define white 0xFFFFFFFF
#define black 0x000000FF
#define red  0xFF0000FF
#define blue  0x0000FFFF
#define gray   0xA5A5A5FF
/* Defining the size of the screen */
#define width 640
#define width_v 940  /* when we use the vectorscope whe have a differnet size */
#define heigth 590

/* Defining where the histograms are */
#define sum1_x 40
#define sum1_y 30
#define frm1_x 340
#define frm1_y 30 
#define sum2_x 40
#define sum2_y 180
#define frm2_x 340
#define frm2_y 180 
#define sum3_x 40
#define sum3_y 340
#define frm3_x 340
#define frm3_y 340 
#define stat 490
#define vector_x 790
#define vector_y 200

	/* needed for SDL */
	SDL_Surface *screen;
	int desired_bpp;
        Uint32 video_flags;
	long number_of_frames;

void HandleEvent()
	{
        SDL_Event event;

        /* Check for events */
        while	(SDL_PollEvent(&event))
		{
		switch	(event.type)
			{
                        case SDL_KEYDOWN:
                        case SDL_QUIT:
				exit(0);
                        }
        	}
	}

void ClearScreen(SDL_Surface *screen)
	{
        int i;
        /* Set the screen to black */
        if	(SDL_LockSurface(screen) == 0 )
		{
                Uint32 Black;
                Uint8 *pixels;
                Black = SDL_MapRGB(screen->format, 0, 0, 0);
                pixels = (Uint8 *)screen->pixels;
                for	( i=0; i<screen->h; ++i )
			{
                        memset(pixels, Black,
                                screen->w*screen->format->BytesPerPixel);
                        pixels += screen->pitch;
                	}
                SDL_UnlockSurface(screen);
        	}
	}

/* Here we draw the layout of the histogram */
void draw_histogram_layout(int x_off, int y_off)
	{
	int i, offset;
	char text[3];

	hlineColor(screen, x_off, x_off+257, y_off+101 , white);
	vlineColor(screen, x_off-1, y_off, y_off+101, white);
	vlineColor(screen, x_off+257,y_off,y_off+101, white);

	if (scalepercent == 0)
		{
		for (i = 0; i < 15; i++)
	  		{
			offset = i*16+ 16;
			vlineColor(screen, x_off+offset, y_off+101, y_off+105, white);
			}

		vlineColor(screen, x_off+128, y_off+106, y_off+110, white);

		/* Description of the axis */
		stringColor(screen, x_off-5 +16 , y_off+115, "16", white);
		stringColor(screen, x_off-9 +128, y_off+115, "128", white);
		stringColor(screen, x_off-9 +240, y_off+115, "240", white);
		}
	else
		{
		for (i =0; i< 11;i++)
			{
			offset = (int)i*21.9 + 16;
			vlineColor(screen, x_off+offset, y_off+101, y_off+105, white);

			}

		}
			
	for (i = 0; i < 6; i++) /*Labeling the y axis in percent */
		{
			hlineColor(screen, x_off-5, x_off, y_off+ i*20, white);
			offset = 100- i*20;
			sprintf(text, "%i", offset);
			stringColor(screen, x_off-30 , y_off+ i*20 -3 , text, white);
		}
	}

/* init the screen */
void y4m_init_area(SDL_Surface *screen)
	{
/* Drawing for the summ of frames */
	draw_histogram_layout(sum1_x, sum1_y);
	draw_histogram_layout(frm1_x, frm1_y);
	draw_histogram_layout(sum2_x, sum2_y);
	draw_histogram_layout(frm2_x, frm2_y);
	draw_histogram_layout(sum3_x, sum3_y);
	draw_histogram_layout(frm3_x, frm3_y);

	SDL_UpdateRect(screen,0,0,0,0);
	}

void make_text(int x_off, int y_off, char text[25])
	{

	boxColor(screen, x_off, y_off-20, x_off+256, y_off-5, black); 
	stringColor(screen, x_off+10, y_off-12, text, white);
	}

/* Here we draw the text description for the histograms */
void make_histogram_desc(long number_of_frames)
	{
	char framestat[25];

	sprintf(framestat, "Y Stat till frame %ld", number_of_frames);
	make_text(sum1_x, sum1_y, framestat);
	sprintf(framestat, "U Stat till frame %ld", number_of_frames);
	make_text(sum2_x, sum2_y, framestat);
	sprintf(framestat, "V Stat till frame %ld", number_of_frames);
	make_text(sum3_x, sum3_y, framestat);

	sprintf(framestat, "Y Stat curent frame");
	make_text(frm1_x, frm1_y, framestat);
	sprintf(framestat, "U Stat curent frame");
	make_text(frm2_x, frm2_y, framestat);
	sprintf(framestat, "V Stat curent frame");
	make_text(frm3_x, frm3_y, framestat);
	}

/* Here we draw the souroundings for the color points */
void makepoint(int laeng, int winkel)
{
	int p1_x, p1_y, p2_x, p2_y;
	int sm,la;

	sm = 5;
	la = 10;

	/* That 57,3 makes the conversation from Grad to radiant */
	p1_x = round ( (laeng+la) * cos ((winkel-10) /57.3) );
	p1_y = round ( (laeng+la) * sin ((winkel-10) /57.3) );
	p2_x = round ( (laeng+sm) * cos ((winkel-10) /57.3) );
	p2_y = round ( (laeng+sm) * sin ((winkel-10) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);
	p2_x = round ( (laeng+la) * cos ((winkel-5) /57.3) );
	p2_y = round ( (laeng+la) * sin ((winkel-5) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);

	p1_x = round ( (laeng-la) * cos ((winkel-10) /57.3) );
	p1_y = round ( (laeng-la) * sin ((winkel-10) /57.3) );
	p2_x = round ( (laeng-sm) * cos ((winkel-10) /57.3) );
	p2_y = round ( (laeng-sm) * sin ((winkel-10) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);
	p2_x = round ( (laeng-la) * cos ((winkel-5) /57.3) );
	p2_y = round ( (laeng-la) * sin ((winkel-5) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);

	p1_x = round ( (laeng+la) * cos ((winkel+10) /57.3) );
	p1_y = round ( (laeng+la) * sin ((winkel+10) /57.3) );
	p2_x = round ( (laeng+sm) * cos ((winkel+10) /57.3) );
	p2_y = round ( (laeng+sm) * sin ((winkel+10) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);
	p2_x = round ( (laeng+la) * cos ((winkel+5) /57.3) );
	p2_y = round ( (laeng+la) * sin ((winkel+5) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);

	p1_x = round ( (laeng-la) * cos ((winkel+10) /57.3) );
	p1_y = round ( (laeng-la) * sin ((winkel+10) /57.3) );
	p2_x = round ( (laeng-sm) * cos ((winkel+10) /57.3) );
	p2_y = round ( (laeng-sm) * sin ((winkel+10) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);
	p2_x = round ( (laeng-la) * cos ((winkel+5) /57.3) );
	p2_y = round ( (laeng-la) * sin ((winkel+5) /57.3) );
	lineColor(screen, vector_x + p1_x, vector_y - p1_y, 
							vector_x + p2_x, vector_y - p2_y,blue);

	//mjpeg_debug(" 1x %i, 1y %i, 2x %i, 2y %i", p1_x, p1_y, p2_x, p2_y);
}

/* Here we draw the vectorscope layout */
void make_vectorscope_layout()
	{
	int i, p1_x, p1_y, p2_x, p2_y;
	double temp;
	char text[4];

	boxColor(screen, vector_x-130, vector_y+130, 
			 vector_x+130, vector_y-130, black); 

	hlineColor(screen, vector_x-20, vector_x-128, vector_y, white);
	hlineColor(screen, vector_x+20, vector_x+128, vector_y, white);
	vlineColor(screen, vector_x, vector_y-20, vector_y-128, white);
	vlineColor(screen, vector_x, vector_y+20, vector_y+128, white);
	circleColor(screen,vector_x, vector_y, 20, white);
	circleColor(screen,vector_x, vector_y, 50, white);
	circleColor(screen,vector_x, vector_y, 80, white);
	circleColor(screen,vector_x, vector_y,112, white);
	
	for (i =0; i<36; i++)
		{		/* Here we draw the degree scale */
			temp = 112* cos (i*(10/57.3)) ;  /* sin & cos only accept radiant */
			p1_x = round(temp);					/* and not degree input */
			temp = 112* sin (i*(10/57.3) );
			p1_y = round(temp);
			temp = 117* cos (i*(10/57.3) );
			p2_x = round(temp);
			temp = 117* sin (i*(10/57.3) );
			p2_y = round(temp);

			lineColor(screen, vector_x + p1_x, vector_y + p1_y,
									vector_x + p2_x, vector_y + p2_y, white);
		}

	sprintf(text, "0"); /* Creating the grad description */
	stringColor(screen, vector_x + 135 , vector_y - 3, text, white);
	sprintf(text, "90");
	stringColor(screen, vector_x - 7 , vector_y - 145, text, white);
	sprintf(text, "180");
	stringColor(screen, vector_x - 155 , vector_y - 3, text, white);
	sprintf(text, "270");
	stringColor(screen, vector_x - 10 , vector_y + 135, text, white);

	makepoint( 90, 60); /* length, arc, for purple */	
	makepoint( 96, 104); /* length, arc, for red */	
	makepoint( 67, 166); /* length, arc, for yellow */	
	makepoint( 90, 241); /* length, arc, for green */	
	makepoint( 96, 283); /* length, arc, for cyan */	
	makepoint( 67, 346); /* length, arc, for blue */	

	}

/* Here we draw the histogram statistice for the summ of the frames */
void make_histogram_stat(int peak_y, int peak_u, int peak_v, 
				int x_off, int y_off)
{
	char framestat[25];

	sprintf(framestat, "Peak Y at : %i", peak_y);
	stringColor(screen, x_off+10, y_off+10, framestat, white);
	sprintf(framestat, "Peak U at : %i", peak_u);
	stringColor(screen, x_off+10, y_off+25, framestat, white);
	sprintf(framestat, "Peak V at : %i", peak_v);
	stringColor(screen, x_off+10, y_off+40, framestat, white);

}

/* Here we delete the old histograms */
void clear_histogram_area()
	{ 
	/* clearing the histogram areas */
	boxColor(screen, sum1_x, sum1_y-1, sum1_x+256, sum1_y+100, black); 
	boxColor(screen, sum2_x, sum2_y-1, sum2_x+256, sum2_y+100, black); 
	boxColor(screen, sum3_x, sum3_y-1, sum3_x+256, sum3_y+100, black); 

	boxColor(screen, frm1_x, frm1_y-1, frm1_x+256, frm1_y+100, black); 
	boxColor(screen, frm2_x, frm2_y-1, frm2_x+256, frm2_y+100, black); 
	boxColor(screen, frm3_x, frm3_y-1, frm3_x+256, frm3_y+100, black); 

	/* clearing frame statistics */
	boxColor(screen, sum1_x,stat, frm1_x+256,stat+90,black); 
	} 

/* Show the frame stat */
void make_stat()
	{
	int percent_y, percent_u, percent_v, percent_fy, percent_fu, percent_fv;
	long long num_pixels_y, num_pixels_u, num_pixels_v;
	long long f_pixels_y, f_pixels_u, f_pixels_v;
	int peak_y, peak_u, peak_v, peak_fy, peak_fu, peak_fv;
	int i,j;

	peak_y = 0; peak_u = 0; peak_v = 0;
	peak_fy = 0; peak_fu = 0; peak_fv = 0;
	num_pixels_y = y_stats[0];
	num_pixels_u = u_stats[0];
	num_pixels_v = v_stats[0];

	f_pixels_y = fy_stats[0];
	f_pixels_u = fu_stats[0];
	f_pixels_v = fv_stats[0];

/* geting the maimal number for all frames */
	for	(i = 0; i < 255; i++)
  		{  /* getting the maximal numbers for Y, U, V for all frames */
		if	(num_pixels_y < y_stats[i]) 
			{
			num_pixels_y = y_stats[i];
			peak_y = i;
			}
		if	(num_pixels_u < u_stats[i]) 
			{
			num_pixels_u = u_stats[i];
			peak_u = i;
			}
		if	(num_pixels_v < v_stats[i]) 
			{
			num_pixels_v = v_stats[i];
			peak_v = i;
			}

/* getting the maximal numbers for Y, U, V for the current frame */
		fy_stats[i]= y_stats[i] - ly_stats[i];
		ly_stats[i] = y_stats[i];
		if	(f_pixels_y < fy_stats[i])
			{
			f_pixels_y = fy_stats[i];
			peak_fy = i;
			}

		fu_stats[i]= u_stats[i] - lu_stats[i];
		lu_stats[i] = u_stats[i];
		if	(f_pixels_u < fu_stats[i])
			{
			f_pixels_u = fu_stats[i];
			peak_fu = i;
			}
	
		fv_stats[i]= v_stats[i] - lv_stats[i];
		lv_stats[i] = v_stats[i];
		if	(f_pixels_v < fv_stats[i])
			{
			f_pixels_v = fv_stats[i];
			peak_fv = i;
			}
  		} 

	num_pixels_y = (num_pixels_y /100);
	num_pixels_u = (num_pixels_u /100);
	num_pixels_v = (num_pixels_v /100);

	f_pixels_y = (f_pixels_y /100);
	f_pixels_u = (f_pixels_u /100);
	f_pixels_v = (f_pixels_v /100);

/* The description for the histogram */
	make_histogram_desc(number_of_frames);
	number_of_frames++;
	make_vectorscope_layout(); /*draw the vectorscope basic layout*/

	clear_histogram_area(); /* Here we delete the old histograms */
	
	make_histogram_stat(peak_y, peak_u, peak_v, sum1_x, stat);
	make_histogram_stat(peak_fy, peak_fu, peak_fv, frm1_x , stat);

	for	(i = 0; i < 255; i++)
		{
		percent_y = (y_stats[i] / num_pixels_y);
		percent_u = (u_stats[i] / num_pixels_u);
		percent_v = (v_stats[i] / num_pixels_v);

		percent_fy = (fy_stats[i] / f_pixels_y);
		percent_fu = (fu_stats[i] / f_pixels_u);
		percent_fv = (fv_stats[i] / f_pixels_v);

		if	((i < 16) || (i > 235)) /* Y luma */
			{   /* Red means out of the allowed range */
			vlineColor(screen,(sum1_x+i),sum1_y+100,
					((sum1_y+100)-percent_y),red);
			vlineColor(screen,(frm1_x+i),frm1_y+100,
					((frm1_y+100)-percent_fy),red);
			}
		else    
			{
			vlineColor(screen,(sum1_x+i),sum1_y+100,
					((sum1_y+100)-percent_y),gray);
			vlineColor(screen,(frm1_x+i),frm1_y+100,
					((frm1_y+100)-percent_fy),gray);
			}

		if	((i < 16) || (i > 240)) /* U V chroma */
			{   /* Red means out of the allowed range */
			vlineColor(screen,(sum2_x+i),sum2_y+100,
					((sum2_y+100)-percent_u),red);
			vlineColor(screen,(sum3_x+i),sum3_y+100,
					((sum3_y+100)-percent_v),red);
			vlineColor(screen,(frm2_x+i),frm2_y+100,
					((frm2_y+100)-percent_fu),red);
			vlineColor(screen,(frm3_x+i),frm3_y+100,
					((frm3_y+100)-percent_fv),red);
			}
		else    
			{
			vlineColor(screen,(sum2_x+i),sum2_y+100,
					((sum2_y+100)-percent_u),gray);
			vlineColor(screen,(sum3_x+i),sum3_y+100,
					((sum3_y+100)-percent_v),gray);
			vlineColor(screen,(frm2_x+i),frm2_y+100,
					((frm2_y+100)-percent_fu),gray);
			vlineColor(screen,(frm3_x+i),frm3_y+100,
					((frm3_y+100)-percent_fv),gray);
			}
		}

		for (i=0; i < 260; i++)
			for (j=0; j < 260; j++)
				if (vectorfield[i][j]== 1)
					pixelColor(screen, (vector_x+ i- 127), (vector_y+ j- 132) ,red);

	SDL_UpdateRect(screen,0,0,0,0);
	}
#endif /* HAVE_SDLgfx */

static void
usage(void)
	{
	fprintf(stderr, "usage: [-t] [-p] [-s num] \n");
	fprintf(stderr, "  -t      emit text summary even if graphical mode enabled\n");
	fprintf(stderr, "  -p      label the scale in percent not absolute numbers\n");
	fprintf(stderr, "  -s num  enable also the vectorscope, allowed numbers 1-16\n");

	exit(1);
	}

int
main(int argc, char **argv)
	{
	int	i, fdin, ss_v, ss_h, chroma_ss, textout;
	int 	do_vectorscope;
	int	pwidth, pheight; /* Needed for the vectorscope */
	int	plane0_l, plane1_l, plane2_l;
	u_char	*yuv[3], *cp;
#ifdef	HAVE_SDLgfx
	int	j;
	int temp_x, temp_y;
	u_char	*cpx, *cpy;
#endif
	y4m_stream_info_t istream;
	y4m_frame_info_t iframe;

	do_vectorscope = 0;
	scalepercent = 0;

#ifdef	HAVE_SDLgfx
	textout = 0;
#else
	textout = 1;
#endif

	while	((i = getopt(argc, argv, "tps:")) != EOF)
		{
		switch	(i)
			{
			case	't':
				textout = 1;
				break;
			case	'p':
				scalepercent = 1;
				break;
			case	's':
				do_vectorscope = atoi(optarg);
				break;
			default:
				usage();
			}
		}

#ifdef HAVE_SDLgfx
	if ( (do_vectorscope < 0) || (do_vectorscope >16) )
		usage();

	/* Initialize SDL */
	desired_bpp = 8; 
	video_flags = 0;
	video_flags |= SDL_DOUBLEBUF;
	number_of_frames = 1;

	memset(fy_stats, '\0', sizeof (fy_stats));
	memset(ly_stats, '\0', sizeof (ly_stats));

        if	( SDL_Init(SDL_INIT_VIDEO) < 0 ) 
                mjpeg_error_exit1("Couldn't initialize SDL:%s",SDL_GetError());
        atexit(SDL_Quit);                       /* Clean up on exit */
        /* Initialize the display */
	if (do_vectorscope == 0)
	        screen = SDL_SetVideoMode(width,heigth,desired_bpp,video_flags);
	else
	        screen=SDL_SetVideoMode(width_v,heigth,desired_bpp,video_flags);

        if	(screen == NULL)
                mjpeg_error_exit1("Couldn't set %dx%dx%d video mode: %s",
                                width, heigth, desired_bpp, SDL_GetError());

	SDL_WM_SetCaption("y4mhistogram", "y4mhistogram");

	y4m_init_area(screen); /* Here we draw the basic layout */
#endif /* HAVE_SDLgfx */

	fdin = fileno(stdin);

	y4m_accept_extensions(1);

	y4m_init_stream_info(&istream);
	y4m_init_frame_info(&iframe);

	if	(y4m_read_stream_header(fdin, &istream) != Y4M_OK)
		mjpeg_error_exit1("stream header error");

        if      (y4m_si_get_plane_count(&istream) != 3)
                mjpeg_error_exit1("Only 3 plane formats supported");

	pwidth = y4m_si_get_width(&istream);
	pheight = y4m_si_get_height(&istream);
	chroma_ss = y4m_si_get_chroma(&istream);
	ss_h = y4m_chroma_ss_x_ratio(chroma_ss).d;
	ss_v = y4m_chroma_ss_y_ratio(chroma_ss).d;


	plane0_l = y4m_si_get_plane_length(&istream, 0);
	plane1_l = y4m_si_get_plane_length(&istream, 1);
	plane2_l = y4m_si_get_plane_length(&istream, 2);

	yuv[0] = malloc(plane0_l);
	if	(yuv[0] == NULL)
		mjpeg_error_exit1("malloc(%d) plane 0", plane0_l);
	yuv[1] = malloc(plane1_l);
	if	(yuv[1] == NULL)
		mjpeg_error_exit1(" malloc(%d) plane 1", plane1_l);
	yuv[2] = malloc(plane2_l);
	if	(yuv[2] == NULL)
		mjpeg_error_exit1(" malloc(%d) plane 2\n", plane2_l);

	while	(y4m_read_frame(fdin,&istream,&iframe,yuv) == Y4M_OK)
		{
		for	(i = 0, cp = yuv[0]; i < plane0_l; i++, cp++)
			y_stats[*cp]++; /* Y' */
		for	(i = 0, cp = yuv[1]; i < plane1_l; i++, cp++)
			u_stats[*cp]++;	/* U */
		for	(i = 0, cp = yuv[2]; i < plane2_l; i++, cp++)
			v_stats[*cp]++;	/* V */
#ifdef HAVE_SDLgfx
			
		if (do_vectorscope >= 1 )
		{
		
		for (i=0; i<260; i++) /* Resetting the vectorfield */
			for (j=0;j<260;j++)
				vectorfield[i][j]=0;

		cpx = yuv[1];
		cpy = yuv[2];

		for (i=0; i < (pheight/ss_h); i++)
			{
			for (j = 0; j < (pwidth/ss_v); j++)
				{
					cpx++;
					cpy++;

					/* Have no idea why I have to multiply it with that values
					   But than the vectorsscope works correct. If someone has
						a explantion or better fix tell me. Bernhard */
					temp_x = round( 128+ ((*cpx-128) * 0.7857) );
					temp_y = round( 128+ ((*cpy-128) * 1.1143) );
					vectorfield[temp_x][temp_y*-1]=1;
				}

				/* Here we got to the n'th next line if needed */
				i   = i + (do_vectorscope-1);
				cpy = cpy + (pwidth/ss_v) * (do_vectorscope-1);
				cpx = cpx + (pwidth/ss_v) * (do_vectorscope-1);
			}

		}
		make_stat(); /* showing the sats */

		SDL_UpdateRect(screen,0,0,0,0); /* updating all */

		/* Events for SDL */
		HandleEvent();
#endif
		}
	y4m_fini_frame_info(&iframe);
	y4m_fini_stream_info(&istream);

	if	(textout)
		{
		for	(i = 0; i < 255; i++)
			printf("Y %d %lld\n", i, y_stats[i]);
		for	(i = 0; i < 255; i++)
			printf("U %d %lld\n", i, u_stats[i]);
		for	(i = 0; i < 255; i++)
			printf("V %d %lld\n", i, v_stats[i]);
		}
	exit(0);
	}
