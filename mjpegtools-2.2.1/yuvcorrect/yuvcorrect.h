// Data type for yuvcorrect overall functionnality
typedef struct
{
  uint8_t verbose;
  uint8_t mode;			// =0 for non-interactive mode, =1 for interactive corrections in full mode, =2 in half mode
  uint8_t stat;			// =1 if statistics informations are to be displayed
  int ImgFrame;			// Only useful for yuvcorrect_tune
  int RefFrame;			// Only useful for yuvcorrect_tune
  uint8_t rgbfirst;		// =1 if rgb corrections are to be applied before yuv ones
}
overall_t;

// Data type for yuvcorrect_tune reference frame
typedef struct
{
  unsigned int width;
  unsigned int height;
  uint8_t *ref;
  y4m_frame_info_t info;
  y4m_stream_info_t streaminfo;
}
ref_frame_t;



// Data type for yuvcorrect frame characteristics
typedef struct
{
  // General frame characteristics
//   y4m_ratio_t sample_aspect_ratio = Y4M_SAR_UNKNOWN; // see yuv4mpeg.h and yuv4mpeg_intern.h for possible values
//   int interlaced = -1; // =Y4M_ILACE_NONE for progressive/not-interlaced, Y4M_ILACE_TOP_FIRST for top interlaced, Y4M_ILACE_BOTTOM_FIRST for bottom interlaced
  unsigned long int length;
  y4m_frame_info_t info;
  // Y characteristics
  uint8_t *y;
  unsigned int y_width;
  unsigned int y_height;
  unsigned long int nb_y;
  // UV characteristics
  uint8_t *u, *v;
  unsigned int ss_h;
  unsigned int ss_v;
  unsigned int uv_width;
  unsigned int uv_height;
  unsigned long int nb_uv;
  // Two fields
  uint8_t *field1, *field2;
}
frame_t;

// Data type for general frame correction 
typedef struct
{
  // HEADER management
  uint8_t no_header;		// =0 by default, =1 to suppress header output
  y4m_stream_info_t streaminfo;
  // Frame corrections
  uint8_t line_switch;		// =0 by default, =1 if line switching is activated
  int8_t  field_move;		// =0 by default, =1 if bottom field is moved one frame forward, =-1 if it is the top field 
}
general_correction_t;

// Data type for correction in the YUV space
typedef struct
{
  // LUMINANCE correction
  uint8_t luma;			// =1 if luminance correction is activated
  uint8_t *luminance;
  float Gamma;
  uint8_t InputYmin;
  uint8_t InputYmax;
  uint8_t OutputYmin;
  uint8_t OutputYmax;
  // CHROMINANCE correction
  uint8_t chroma;		// =1 if chrominance correction is activated
  uint8_t *chrominance;
  float UVrotation;
  float Ufactor;
  uint8_t Urotcenter;
  float Vfactor;
  uint8_t Vrotcenter;
  uint8_t UVmin;
  uint8_t UVmax;
}
yuv_correction_t;

/*
 * Data type for correction in the RGB space
*/
typedef struct
{
  // RGB correction
  uint8_t rgb;			// =1 if RGB correction is activated
  uint8_t *new_red;
  uint8_t *new_green;
  uint8_t *new_blue;
  float RGamma;
  float GGamma;
  float BGamma;
  uint8_t InputRmin;
  uint8_t InputRmax;
  uint8_t OutputRmin;
  uint8_t OutputRmax;
  uint8_t InputGmin;
  uint8_t InputGmax;
  uint8_t OutputGmin;
  uint8_t OutputGmax;
  uint8_t InputBmin;
  uint8_t InputBmax;
  uint8_t OutputBmin;
  uint8_t OutputBmax;
  // Acceleration through tabulation of all multiplicative operations
  uint8_t *luma_r, *luma_g, *luma_b;
  int8_t *u_r, *u_g, *u_b;
  int8_t *v_r, *v_g, *v_b;
  int16_t *RUV_v, *GUV_v, *GUV_u, *BUV_u;

}
rgb_correction_t;

// Functions Prototypes
void handle_args_overall (int argc, char *argv[], overall_t * overall);
int yuvcorrect_y4m_read_frame (int fd, y4m_stream_info_t *si, frame_t * frame, uint8_t line_switch);
int yuvcorrect_luminance_init (yuv_correction_t * yuv_correct);
int yuvcorrect_chrominance_init (yuv_correction_t * yuv_correct);
int yuvcorrect_luminance_treatment (frame_t * frame,
				    yuv_correction_t * yuv_correct);
int yuvcorrect_chrominance_treatment (frame_t * frame,
				      yuv_correction_t * yuv_correct);
int bottom_field_storage (frame_t * frame, uint8_t oddeven, uint8_t * field1,
			  uint8_t * field2);
int top_field_storage (frame_t * frame, uint8_t oddeven, uint8_t * field1,
		       uint8_t * field2);
int bottom_field_replace (frame_t * frame, uint8_t oddeven, uint8_t * field1,
			  uint8_t * field2);
int top_field_replace (frame_t * frame, uint8_t oddeven, uint8_t * field1,
		       uint8_t * field2);
int main (int argc, char *argv[]);
uint8_t clip_0_255   (int16_t number);
int8_t  clip_127_127 (int16_t number);
void yuvstat (frame_t * frame);
uint8_t yuvcorrect_nearest_integer_division (unsigned long int p,
					     unsigned long int q);
int yuvcorrect_RGB_treatment (frame_t * frame,
			      rgb_correction_t * rgb_correct);
int yuvcorrect_RGB_init (rgb_correction_t * rgb_correct);

void
handle_args_yuv_rgb (int argc, char *argv[], yuv_correction_t * yuv_correct, rgb_correction_t * rgb_correct);
void initialisation1(int, frame_t * frame, general_correction_t * gen_correct,
		     yuv_correction_t * yuv_correct, rgb_correction_t * rgb_correct);
void initialisation2(yuv_correction_t * yuv_correct, rgb_correction_t * rgb_correct);
void ref_frame_init(int fd,ref_frame_t *ref_frame);
