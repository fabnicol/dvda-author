/*
 * y4mspatialfilter.c
 *
 * written by Dan Scholnik <scholnik@ieee.org>
 * spatial FIR filter for noise/bandwidth reduction without scaling
 * takes yuv4mpeg in and spits the same out
 *
 * Usage: y4mspatialfilter [-h] [-v] [-L luma_Xtaps,luma_XBW,luma_Ytaps,luma_YBW] 
 *                                   [-C chroma_Xtaps,chroma_XBW,chroma_Ytaps,chroma_YBW]
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "yuv4mpeg.h"
#include "cpu_accel.h"

#ifdef HAVE_ASM_MMX
#include "mmx.h"
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static void *my_malloc(size_t);
static struct filter *get_coeff(int, float);
static void convolveFrame(u_char *src,int w,int h,int interlace,struct filter *xtap,struct filter *ytap,float *yuvtmp1,float *yuvtmp2);
static void set_accel(int w,int h);
static void usage(char *);

static void (*pframe_i2f)(u_char *,float *,int);
static void (*pframe_f2i)(float *,u_char *,int);
static void (*pconvolveLine)(float *,int,int,int,struct filter *,int,float *);

struct filter {
    int len;
    float **filters;
    float **qfilters; // for SSE/Altivec -- repeats the filter 4 times
};

int main(int argc, char **argv)
{
    int    i, c, interlace, frames, err;
    int    ywidth, yheight, uvwidth, uvheight, ylen, uvlen;
    int    verbose = 0, fdin;
    int    NlumaX = 4, NlumaY = 4, NchromaX = 4, NchromaY = 4;
    float  BWlumaX = 0.8, BWlumaY = 0.8, BWchromaX = 0.7, BWchromaY = 0.7;
    struct filter *lumaXtaps, *lumaYtaps, *chromaXtaps, *chromaYtaps;
    u_char *yuvinout[3];
    float *yuvtmp1,*yuvtmp2;
    y4m_stream_info_t istream, ostream;
    y4m_frame_info_t iframe;

    fdin = fileno(stdin);
    
    y4m_accept_extensions(1);

    /* read command line */
    opterr = 0;
    while   ((c = getopt(argc, argv, "hvL:C:x:X:y:Y:")) != EOF)
	{
	    switch  (c)
		{
		case    'L':
		    sscanf(optarg,"%d,%f,%d,%f",&NlumaX,&BWlumaX,&NlumaY,&BWlumaY);
		    break;
		case    'C':
		    sscanf(optarg,"%d,%f,%d,%f",&NchromaX,&BWchromaX,&NchromaY,&BWchromaY);
		    break;
		case    'x':
		    sscanf(optarg,"%d,%f",&NchromaX,&BWchromaX);
		    break;
		case    'X':
		    sscanf(optarg,"%d,%f",&NlumaX,&BWlumaX);
		    break;
		case    'y':
		    sscanf(optarg,"%d,%f",&NchromaY,&BWchromaY);
		    break;
		case    'Y':
		    sscanf(optarg,"%d,%f",&NlumaY,&BWlumaY);
		    break;
		case    'v':
		    verbose++;
		    break;
		case    '?':
		case    'h':
		default:
		    usage(argv[0]);
		}
	}
    
    if (BWlumaX <= 0.0 || BWlumaX > 1.0)
       mjpeg_error_exit1("Horizontal luma bandwidth '%f' not >0 and <=1.0", BWlumaX);
    if (BWlumaY <= 0.0 || BWlumaY > 1.0)
       mjpeg_error_exit1("Vertical luma bandwidth '%f' not >0 and <=1.0", BWlumaY);
    if (BWchromaX <= 0.0 || BWchromaX > 1.0)
       mjpeg_error_exit1("Horizontal chroma bandwidth '%f' not >0 and <=1.0", BWchromaX);
    if (BWchromaY <= 0.0 || BWchromaY > 1.0)
       mjpeg_error_exit1("Vertical chroma bandwidth '%f' not >0 and <=1.0", BWchromaY);

    /* initialize input stream and check chroma subsampling and interlacing */
    y4m_init_stream_info(&istream);
    y4m_init_frame_info(&iframe);
    err = y4m_read_stream_header(fdin, &istream);
    if (err != Y4M_OK)
	mjpeg_error_exit1("Input stream error: %s\n", y4m_strerr(err));

    if	(y4m_si_get_plane_count(&istream) != 3)
	mjpeg_error_exit1("Only the 3 plane formats supported");

    i = y4m_si_get_interlace(&istream);
    switch (i)
        {
        case Y4M_ILACE_NONE:
	    interlace = 0;
	    break;
        case Y4M_ILACE_BOTTOM_FIRST:
        case Y4M_ILACE_TOP_FIRST:
	    interlace = 1;
	    break;
        default:
	    mjpeg_warn("Unknown interlacing '%d', assuming non-interlaced", i);
	    interlace = 0;
	    break;
        }

    ywidth = y4m_si_get_width(&istream);	/* plane 0 = Y */
    yheight = y4m_si_get_height(&istream);
    ylen = ywidth * yheight;
    uvwidth = y4m_si_get_plane_width(&istream, 1);	/* planes 1&2 = U+V */
    uvheight = y4m_si_get_plane_height(&istream, 1);
    uvlen = y4m_si_get_plane_length(&istream, 1);
    
    /* initialize output stream */
    y4m_init_stream_info(&ostream);
    y4m_copy_stream_info(&ostream, &istream);
    y4m_write_stream_header(fileno(stdout), &ostream);
    
    /* allocate input and output buffers */
    yuvinout[0] = my_malloc(ylen*sizeof(u_char));
    yuvinout[1] = my_malloc(uvlen*sizeof(u_char));
    yuvinout[2] = my_malloc(uvlen*sizeof(u_char));
    yuvtmp1 = my_malloc(MAX(ylen,uvlen)*sizeof(float));
    yuvtmp2 = my_malloc(MAX(ylen,uvlen)*sizeof(float));

    /* get filter taps */
    lumaXtaps   = get_coeff(NlumaX, BWlumaX);
    lumaYtaps   = get_coeff(NlumaY, BWlumaY);
    chromaXtaps = get_coeff(NchromaX, BWchromaX);
    chromaYtaps = get_coeff(NchromaY, BWchromaY);

    set_accel(uvwidth,uvheight);

    if (verbose)
	y4m_log_stream_info(mjpeg_loglev_t("info"), "", &istream);
    
    /* main processing loop */
    for (frames=0; y4m_read_frame(fdin,&istream,&iframe,yuvinout) == Y4M_OK; frames++)
	{
	    if (verbose && ((frames % 100) == 0))
		mjpeg_info("Frame %d\n", frames);
	    
            convolveFrame(yuvinout[0],ywidth,yheight,interlace,lumaXtaps,lumaYtaps,yuvtmp1,yuvtmp2);
            convolveFrame(yuvinout[1],uvwidth,uvheight,interlace,chromaXtaps,chromaYtaps,yuvtmp1,yuvtmp2);
            convolveFrame(yuvinout[2],uvwidth,uvheight,interlace,chromaXtaps,chromaYtaps,yuvtmp1,yuvtmp2);

	    y4m_write_frame(fileno(stdout), &ostream, &iframe, yuvinout);

	}
    
    /* clean up */
    y4m_fini_frame_info(&iframe);
    y4m_fini_stream_info(&istream);
    y4m_fini_stream_info(&ostream);
    exit(0);
}


/* Move memory allocation error checking here to clean up code */
/* Aligns values on 16 byte boundaries (to aid in vectorization) */
static void *my_malloc(size_t size)
{
    void *tmp = malloc(size+15);
    unsigned long addr;
    if (tmp == NULL)
	    mjpeg_error_exit1("malloc(%ld) failed\n", (long)size);
    addr=(unsigned long)tmp;
    addr=(addr+15)&(-16);    
    return (void *)addr;
}


/* Compute 1D filter coefficients (center and right-side taps only) */
/* To minimize artifacts at the boundaries, compute filters of all sizes */
/* from 1 to length and use shorter filters near the edge of the frame */
/* Also, normalize to a DC gain of 1 */
static struct filter *get_coeff(int length, float bandwidth)
{
    int n,k;
    struct filter *f;
    float sum;

    f=my_malloc(sizeof(struct filter));
    f->len=length;
    f->filters=my_malloc((length+1)*sizeof(float *));
    f->qfilters=my_malloc((length+1)*sizeof(float *));

    /* C*sinc(C*n).*sinc(2*n/N); Lanczos-weighted */    
    for(k=0;k<=length;k++)
	{
            f->filters[k]=my_malloc((k+1)*sizeof(float *));
	    f->filters[k][0]=bandwidth;
	    sum=f->filters[k][0];
	    for(n=1;n<=k;n++)
		{
		    f->filters[k][n] = bandwidth * sin(M_PI*bandwidth*n)/(M_PI*bandwidth*n)
                        * sin(M_PI*2*n/(2*k+1))/(M_PI*2.0*n/(2*k+1));
		    sum+=2*f->filters[k][n];
		}
	    for(n=0;n<=k;n++)
		f->filters[k][n]/=sum;

            // qfilters is the same as filters, but each element is repeated 4 times
            f->qfilters[k]=my_malloc(4*(k+1)*sizeof(float *));
            for( n=0; n<(k+1)*4; n++ )
                f->qfilters[k][n]=f->filters[k][n>>2];
	}
    return f;
}

/* These three routines do the bulk of the work */

static void frame_i2f(u_char *src,float *dst,int l)
{
    int i;

    for( i=0; i<l; i++ )
        dst[i]=src[i];
}

static void frame_f2i(float *src,u_char *dst,int l)
{
    int i;

    for( i=0; i<l; i++ )
        dst[i]=src[i]+0.5;
}

/* Routine to perform a 1-dimensional convolution with the result 
   symmetrically truncated to match the input length.  
   Filter is odd and linear phase, only center and right-side taps are specified */

static void convolveLine(float *data,int width,int datastride,int outstride,struct filter *filter,int flen,float *out)
{
    int i;
    float *f=filter->filters[flen];

    for( i=0; i<width; i++ ) {
        int k;
        float *d1=data,*d2=data;
        float tempout=f[0]*data[0];
        for( k=1; k<=flen; k++) {
            d1-=datastride;
            d2+=datastride;
            tempout+=f[k]*(d1[0]+d2[0]);
        }
        /* clip and cast to integer */
        out[0]=MIN(MAX(tempout,0),255);
        out+=outstride;
        data+=1;
    }
}

#ifdef HAVE_ASM_MMX

static void frame_i2f_sse(u_char *src,float *dst,int l)
{
    int i;

    pxor_r2r(mm7,mm7);

    for( i=0; i<l; i+=8 ) {
        movq_m2r(*src,mm0);
        movq_r2r(mm0, mm2);
        punpcklbw_r2r(mm7, mm0);
        punpckhbw_r2r(mm7, mm2);
        movq_r2r(mm0, mm1);
        movq_r2r(mm2, mm3);
        punpcklwd_r2r(mm7, mm0);
        punpckhwd_r2r(mm7, mm1);
        punpcklwd_r2r(mm7, mm2);
        punpckhwd_r2r(mm7, mm3);
        cvtpi2ps_r2r(mm0,xmm0);
        cvtpi2ps_r2r(mm1,xmm1);
        cvtpi2ps_r2r(mm2,xmm2);
        cvtpi2ps_r2r(mm3,xmm3);
        movlps_r2m(xmm0,dst[0]);
        movlps_r2m(xmm1,dst[2]);
        movlps_r2m(xmm2,dst[4]);
        movlps_r2m(xmm3,dst[6]);

        src+=8;
        dst+=8;
    }
    emms();
}

static void frame_f2i_sse(float *src,u_char *dst,int l)
{
    int i;

    // put 128 in all 4 words of mm7
    movd_g2r(128,mm7);
    punpcklwd_r2r(mm7,mm7);
    punpckldq_r2r(mm7,mm7);

    // put 128 in all 8 bytes of mm6
    movd_g2r(128,mm6);
    punpcklbw_r2r(mm6,mm6);
    punpcklwd_r2r(mm6,mm6);
    punpckldq_r2r(mm6,mm6);

    for( i=0; i<l; i+=8 ) {
        movaps_m2r(src[0],xmm0);
        movaps_m2r(src[4],xmm2);
        movhlps_r2r(xmm0,xmm1);
        cvtps2pi_r2r(xmm0,mm0);
        cvtps2pi_r2r(xmm1,mm1);
        movhlps_r2r(xmm2,xmm3);
        cvtps2pi_r2r(xmm2,mm2);
        cvtps2pi_r2r(xmm3,mm3);
        packssdw_r2r(mm1,mm0);
        packssdw_r2r(mm3,mm2);
        psubw_r2r(mm7,mm0);
        psubw_r2r(mm7,mm2);
        packsswb_r2r(mm2, mm0);
        paddb_r2r(mm6, mm0);
        movq_r2m(mm0,dst[0]);

        src+=8;
        dst+=8;
    }

    emms();
}

/* Routine to perform a 1-dimensional convolution with the result 
   symmetrically truncated to match the input length.  
   Filter is odd and linear phase, only center and right-side taps are specified */

static float all0  [4] ATTR_ALIGN(16) = {0,0,0,0};
static float all255[4] ATTR_ALIGN(16) = {255,255,255,255};

static void convolveLine_sse(float *data,int width,int datastride,int outstride,struct filter *filter,int flen,float *out)
{
    int i;
    float *f=filter->qfilters[flen];

    /* arg! why aren't these aligned? */
    movups_m2r(all0[0],  xmm6);
    movups_m2r(all255[0],xmm7);
    for( i=0; i<width; i+=4 ) {
        int k;
        float *d1=data,*d2=data,*ft=f;

        movaps_m2r(data[0],xmm0);
        mulps_m2r (ft[0],  xmm0);

        for( k=1; k<=flen; k++) {
            d1-=datastride;
            d2+=datastride;
            ft+=4;
            movaps_m2r(d1[0],xmm1);
            addps_m2r (d2[0],xmm1);
            mulps_m2r (ft[0],xmm1);
            addps_r2r (xmm1, xmm0);
        }

        /* clip and cast to integer */
        minps_r2r(xmm7,xmm0);
        maxps_r2r(xmm6,xmm0);

        movss_r2m(xmm0,out[0]);
        out+=outstride;
        shufps_r2ri(xmm0,xmm0,1*1 + 0*4 + 2*16 + 3*64);

        movss_r2m(xmm0,out[0]);
        out+=outstride;
        movhlps_r2r(xmm0,xmm0);

        movss_r2m(xmm0,out[0]);
        out+=outstride;
        shufps_r2ri(xmm0,xmm0,1*1 + 0*4 + 2*16 + 3*64);

        movss_r2m(xmm0,out[0]);
        out+=outstride;

        data+=4;
    }
}


#endif

static void convolveField(float *data, int width, int height, int outstride, struct filter *filter, float *output)
{

    int n,datastride,datalength;

    datalength=height/outstride;
    datastride=width*outstride;

    /* leading edge, use filters of increasing width */
    for(n=0;n<filter->len;n++)
	{
            pconvolveLine(data,width,datastride,height,filter,n,output);
            data+=datastride;
            output+=outstride;
	}
    /* center, use full-width filter */
    for(n=filter->len; n<datalength-filter->len; n++)
	{
            pconvolveLine(data,width,datastride,height,filter,filter->len,output);
            data+=datastride;
            output+=outstride;
	}
    /* trailing edge, use filters of decreasing width */
    for(n=datalength-filter->len;n<datalength;n++)
	{
            pconvolveLine(data,width,datastride,height,filter,datalength-n-1,output);
            data+=datastride;
            output+=outstride;
	}
}

static void convolveFrame(u_char *src,int w,int h,int interlace,struct filter *xtap,struct filter *ytap,float *tmp1,float *tmp2)
{
    pframe_i2f(src,tmp1,w*h);

    if (interlace ) {
        convolveField(tmp1,  w,h,2,ytap,tmp2);
        convolveField(tmp1+w,w,h,2,ytap,tmp2+1);
    }
    else
        convolveField(tmp1,w,h,1,ytap,tmp2);

    convolveField(tmp2,h,w,1,xtap,tmp1);

    pframe_f2i(tmp1,src,w*h);
}

static void set_accel(int w,int h)
{
    pframe_i2f=frame_i2f;
    pframe_f2i=frame_f2i;
    pconvolveLine=convolveLine;

#ifdef HAVE_ASM_MMX
    if ( (w&3)==0 && (h&3)==0 ) { // everything must be a multiple of 4
        if( cpu_accel() & ACCEL_X86_SSE ) {
            pframe_i2f=frame_i2f_sse;
            pframe_f2i=frame_f2i_sse;
            pconvolveLine=convolveLine_sse;
        }
    }
#endif
}

static void usage(char *pgm)
{
    fprintf(stderr, "usage: %s [-h] [-v] [-L lumaXtaps,lumaXBW,lumaYtaps,lumaYBW] ", pgm);
    fprintf(stderr, "[-C chromaXtaps,chromaXBW,chromaYtaps,chromaYBW] ");
    fprintf(stderr, "[-x chromaXtaps,chromaXBW] [-X lumaXtaps,lumaXBW] ");
    fprintf(stderr, "[-y chromaYtaps,chromaYBW] [-Y lumaYtaps,lumaYBW]\n");
    fprintf(stderr, "\t-v be somewhat verbose\n");
    fprintf(stderr, "\t-h print this usage summary\n");
    fprintf(stderr, "\tlumaXtaps: length of horizontal luma filter (0 to disable)\n");
    fprintf(stderr, "\tlumaXBW: fractional bandwidth of horizontal luma filter [0-1.0]\n");
    fprintf(stderr, "\tlumaYtaps: length of vertical luma filter (0 to disable)\n");
    fprintf(stderr, "\tlumaYBW: fractional bandwidth of vertical luma filter [0-1.0]\n");
    fprintf(stderr, "\tchromaXtaps: length of horizontal chroma filter (0 to disable)\n");
    fprintf(stderr, "\tchromaXBW: fractional bandwidth of horizontal chroma filter [0-1.0]\n");
    fprintf(stderr, "\tchromaYtaps: length of vertical chroma filter (0 to disable)\n");
    fprintf(stderr, "\tchromaYBW: fractional bandwidth of vertical chroma filter [0-1.0]\n");
    fprintf(stderr, "\n\t-x/-X/-y/-Y change a vertical/horizontal parameter without affecting the other dimension's value\n");
    exit(1);
}
