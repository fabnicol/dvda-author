/*
    Copyright 2003 Matthew Marjanovic <maddog@mir.com>

    This file is part of y4mscaler.

    y4mscaler is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    y4mscaler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with y4mscaler; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "y4m-config.h"
#include "debug.h"


#include "ysScaling.H"
#include "scaler-matto.H"
#ifdef _EXPERIMENTAL
#include "scaler-jit.H"
#include "scaler-exp.H"
#endif /* _EXPERIMENTAL */

#include <string.h>


void ysScaling::_create_factory_list(void)
{
#ifdef _EXPERIMENTAL
  _factory_count = 4;
#else
  _factory_count = 2;
#endif
  _factory_list = new factory_info[_factory_count];

  _factory_list[0].id = "default";
  _factory_list[0].factory = new mattoScalerFactory();

  _factory_list[1].id = "matto";
  _factory_list[1].factory = new mattoScalerFactory();
#ifdef _EXPERIMENTAL
  _factory_list[2].id = "jit";
  _factory_list[2].factory = new jitScalerFactory();

  _factory_list[3].id = "exp";
  _factory_list[3].factory = new expScalerFactory();
#endif
}


void ysScaling::_destroy_factory_list(void)
{
  if (_factory_list != NULL) {
    for (int i = 0; i < _factory_count; i++) {
      delete _factory_list[i].factory;
    }
    delete[] _factory_list;
    _factory_list = NULL;
  }
}


ysScaling::ysScaling(void) : 
  _factory(NULL),
  _mono(0),
  _line_switching(0),
  _swap_ilace(0)
{
  _create_factory_list();
  for (int i = 0; i < SC_MAX_SCALERS; i++)
    _scalers[i] = NULL;
}


ysScaling::~ysScaling()
{
  _destroy_factory_list();
  _factory = NULL;
  destroy_scalers();
}



void ysScaling::require_factory(void) {
  if (_factory == NULL)
    set_factory(_factory_list[0].factory);
}



void ysScaling::describe_keywords(FILE *fp, const char *prefix) const 
{
  fprintf(fp, "%smode=mono\n", prefix);
  fprintf(fp, "%smode=lineswitch\n", prefix);
  fprintf(fp, "%sscaler=scaler-name\n", prefix);
  fprintf(fp, "%s  Available scalers:\n", prefix);
  int maxlen = 0;
  for (int i = 0; i < _factory_count; i++) {
    int x = strlen(_factory_list[i].id);
    if (x > maxlen) maxlen = x;
  }
  for (int i = 0; i < _factory_count; i++) {
    fprintf(fp, "%s    '%s'%*s - %s %s\n", prefix,
	    _factory_list[i].id, maxlen - (int)strlen(_factory_list[i].id), "",
	    _factory_list[i].factory->description(),
	    (i==0 ? "(default)" : ""));
  }
  fprintf(fp, "%soption=scaler-option\n", prefix);
  fprintf(fp, "%s  Use 'option=help' to see options for chosen scaler.\n", prefix);
}



void ysScaling::parse_keyword(char *optarg)
{
  if (!strcasecmp(optarg, "MODE=MONO")) {
    _mono = 1;

  } else if (!strcasecmp(optarg, "MODE=LINESWITCH")) {
    _line_switching = 1;

  } else if (!strncasecmp(optarg, "SCALER=", 7)) {
    
    if (_factory != NULL) {
      mjpeg_error_exit1("A scaler has already been chosen: '%s'", optarg);
    }
    for (int i = 0; i < _factory_count; i++) {
      if (!strcasecmp(optarg+7, _factory_list[i].id)) {
	set_factory(_factory_list[i].factory);
	break;
      }
    }
    if (_factory == NULL) {
      mjpeg_error_exit1("Unrecognized scaler selected:  '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "OPTION=", 7)) {

    char *option = optarg+7;
    require_factory();
    if (!strcasecmp(option, "HELP")) {
      fprintf(stdout, "Options for scaler '%s':\n", _factory->description());
      _factory->describe_options(stdout, "  ");
      exit(1);

    } else {
      if (_factory->parse_option(option)) {
	mjpeg_error_exit1("Unrecognized scaler option: '%s'", option);
      }
    }

  } else {
    mjpeg_error_exit1("Unrecognized scaling parameter:  '%s'", optarg);
  }
  
}




void ysScaling::describe_parameters() const
{
  mjpeg_info("=== SCALING parameters: ================");
  if (_factory != NULL) {
    mjpeg_info("| Scaler:  %s", _factory->description());
    _factory->describe_parameters(mjpeg_info, "|          ");
  } else
    mjpeg_info("| Scaler:  *NONE*");
  if (_mono)
    mjpeg_info("| MONO:  process luma only");
  if (_line_switching)
    mjpeg_info("| LINESWITCH:  swap scanline pairs");
  if (_swap_ilace)
    mjpeg_info("| SWAP-ILACE:  drop first field, reframe stream");
}


#if 0
void ysScaling::check_parameters(const ysSource &source,
				 const ysTarget &target)
{
  require_factory();

  _swap_ilace = 0;
  if (source.stream().interlace() == Y4M_ILACE_NONE) {
    if (target.stream().interlace() != Y4M_ILACE_NONE) {
      mjpeg_error_exit1("Cannot make interlaced stream from progressive.");
    }
  } else {   /* source is interlaced/mixed */
    if (target.stream().interlace() == Y4M_ILACE_NONE) {
      if (source.stream().interlace() != Y4M_ILACE_MIXED) {
        mjpeg_error("Cannot make progressive stream from interlaced.");
        mjpeg_error_(" (Try '-I ilace=TOP_ONLY/BOTTOM_ONLY'; see manpage.)");
      } else {
        mjpeg_error("Cannot make progressive stream from mixed-mode.");
      }
      exit(1);
    } 
    if (target.stream().interlace() != source.stream().interlace()) {
      mjpeg_warn("Will swap interlacing modes (by dropping first field).");
      _swap_ilace = 1;
    }
  }

  if (_line_switching && (trg_lace == Y4M_ILACE_NONE)) {
    mjpeg_warn("LINESWITCH has no effect on non-interlaced output!");
  }
}
#endif

void ysScaling::check_parameters(const ysSource &source,
				 const ysTarget &target)
{
  require_factory();

  _swap_ilace = 0;
  _vertically_mixed_source = 
    (source.stream().subsampling().ratio().y().denominator() != 1);
  _anomalous_mixtures_are_fatal = 0;
  int src_lace = source.stream().interlace();
  int trg_lace = target.stream().interlace();

  switch (src_lace) {
  case Y4M_ILACE_NONE:
    if (trg_lace != Y4M_ILACE_NONE)
      mjpeg_error_exit1("Cannot make interlaced stream from progressive!");
    break;
  case Y4M_ILACE_TOP_FIRST:
  case Y4M_ILACE_BOTTOM_FIRST:
    if (trg_lace == Y4M_ILACE_NONE) {
      mjpeg_error("Cannot make progressive stream from interlaced.");
      mjpeg_error(" (Try '-I ilace=TOP_ONLY/BOTTOM_ONLY'; see manpage.)");
      exit(1);
    }
    if (trg_lace != src_lace) {
      mjpeg_warn("Will swap interlacing modes (by dropping first field).");
      _swap_ilace = 1;
    }
    break;
  case Y4M_ILACE_MIXED:
    if (trg_lace != Y4M_ILACE_MIXED) {
      mjpeg_error_exit1("Cannot make non-mixed interlacing from mixed!");
    }
    // IF source is vertically subsampled and mixed-mode, 
    // and IF any vertical processing is performed --- meaning:
    //          o vertical scale factor is not 1/1
    //          o y-offset of active region is not an integer
    //     OR target is still vertically subsampled
    // THEN processing will fail on anomalous interlacing mode (i.e. ip, pi).
    // (y4mscaler output is always non-anomalous pp or ii!)
    if (_vertically_mixed_source &&
        ( (target.y_ratio() != ysRatio(1,1)) ||
          !(source.active_region().offset().y().is_integral()) ||
          (target.stream().subsampling().ratio().y().denominator() != 1) )) {
      _anomalous_mixtures_are_fatal = 1;
      mjpeg_warn("(Beware:  Processing will fail midstream if an anomalous");
      mjpeg_warn("          interlace mixture (ip/pi) is encountered.)");
    }
    break;
  default:
    mjpeg_error_exit1("Unknown source interlacing mode!");
    break;
  }

  if (_line_switching) {
    if (target.stream().interlace() == Y4M_ILACE_NONE)
      mjpeg_warn("LINESWITCH has no effect on non-interlaced output!");
    if (target.stream().interlace() == Y4M_ILACE_NONE)
      mjpeg_warn("LINESWITCH has no effect on mixed-interlaced output!");
  }
}


/*
 * Handling of chroma offset:
 *
 * Proper siting of chroma subsamples requires an additional active_region
 *  offset for the chroma plane scalers.  Since this offset may be different
 *  for each plane and for each *field* (in interlaced streams), up to five
 *  different scaler setups may be required.
 *
 * The additional offset is:  qS - (qT / s),
 *  where qS is the source "effective_sample_offset()",
 *        qT is the target "effective_sample_offset()",
 *        s is the scale ratio for the axis.
 *
 * Note that if interlacing is being swapped, then qS and qT must still
 *  correspond to the actual fields, which will be different.  Hence
 *  the arguments for 'src_field' and 'tgt_field'.
 *
 */

ysRatioPoint chroma_active_offset(ysSubsampling::Plane plane,
				  ysSubsampling::Field src_field,
				  ysSubsampling::Field tgt_field,
				  const ysSource &source,
				  const ysTarget &target)
{
  DBG("ss eso (%f, %f)  ts eso (%f, %f)\n",
	  source.stream().subsampling().effective_sample_offset(src_field, plane).x().to_double(),
	  source.stream().subsampling().effective_sample_offset(src_field, plane).y().to_double(),
	  target.stream().subsampling().effective_sample_offset(src_field, plane).x().to_double(),
	  target.stream().subsampling().effective_sample_offset(src_field, plane).y().to_double());
  return
    (source.active_region().offset() 
     * source.stream().subsampling().ratio()
     / ( (src_field == ysSubsampling::FRAME) ? ysPoint(1,1) : ysPoint(1,2) ))

    + source.stream().subsampling().effective_sample_offset(src_field, plane)

    - ( target.stream().subsampling().effective_sample_offset(tgt_field, plane)
	/ ysRatioPoint(target.x_ratio(), target.y_ratio()) );
}


void ysScaling::_create_frame_scalers(const ysSource &source,
					    const ysTarget &target)
{
  int planes = target.stream().planes();

  _scalers[SC_PROG_Y] = _factory->new_scaler();
  _scalers[SC_PROG_Y]->setup(source.stream().framedim(0),
                             source.active_region().offset(),
                             source.matte_region(),
                             target.stream().framedim(0),
                             target.active_region(),
                             target.x_ratio(), target.y_ratio(),
                             source.bgcolor()(0));
  if ((planes > 1) && (!_mono)) {
    ysRatioPoint offsetCb = chroma_active_offset(ysSubsampling::PLANE_Cb,
						 ysSubsampling::FRAME,
						 ysSubsampling::FRAME,
						 source, target);
    ysRatioPoint offsetCr = chroma_active_offset(ysSubsampling::PLANE_Cr,
						 ysSubsampling::FRAME,
						 ysSubsampling::FRAME,
						 source, target);
    ysRatioPoint src_ssRatio = source.stream().subsampling().ratio();
    ysRatioPoint tgt_ssRatio = target.stream().subsampling().ratio();

    ysRatioPoint c_scale = 
      ysRatioPoint(target.x_ratio(), target.y_ratio()) *
      tgt_ssRatio / src_ssRatio;
    
    _scalers[SC_PROG_CB] = _factory->new_scaler();
    _scalers[SC_PROG_CB]->setup(source.stream().framedim(1),
		       offsetCb,
		       ysRegion(source.matte_region().dim() * src_ssRatio,
				source.matte_region().offset() * src_ssRatio),
		       target.stream().framedim(1),
		       ysRegion(target.active_region().dim() * tgt_ssRatio,
				target.active_region().offset() * tgt_ssRatio),
		       c_scale.x(), c_scale.y(), //target.x_ratio(), target.y_ratio(),
		       source.bgcolor()(1)
		       );
    _scalers[SC_PROG_CR] = _factory->new_scaler();
    _scalers[SC_PROG_CR]->setup(source.stream().framedim(2),
		       offsetCr,
		       ysRegion(source.matte_region().dim() * src_ssRatio,
				source.matte_region().offset() * src_ssRatio),
		       target.stream().framedim(2),
		       ysRegion(target.active_region().dim() * tgt_ssRatio,
				target.active_region().offset() * tgt_ssRatio),
		       c_scale.x(), c_scale.y(), //target.x_ratio(), target.y_ratio(),
		       source.bgcolor()(2)
		       );
  }
  if (planes > 3) {
    _scalers[SC_PROG_A] = _factory->new_scaler();
    _scalers[SC_PROG_A]->setup(source.stream().framedim(0),
                               source.active_region().offset(),
                               source.matte_region(),
                               target.stream().framedim(0),
                               target.active_region(),
                               target.x_ratio(), target.y_ratio(),
                               source.bgcolor()(3));
  }
  
}


void ysScaling::_create_field_scalers(const ysSource &source,
				      const ysTarget &target)
{
  int planes = target.stream().planes();

  _scalers[SC_INTER_Y] = _factory->new_scaler();
  _scalers[SC_INTER_Y]->setup(source.stream().fielddim(0),
			source.active_region().offset() / ysPoint(1,2),
			ysRegion(source.matte_region().dim() / ysPoint(1,2),
				 source.matte_region().offset() / ysPoint(1,2)),
			target.stream().fielddim(0),
			ysRegion(target.active_region().dim() / ysPoint(1,2),
				 target.active_region().offset() / ysPoint(1,2)),
			target.x_ratio(), target.y_ratio(),
			source.bgcolor()(0)
			);
  if ((planes > 1) && (!_mono)) {
    ysRatioPoint offsetCbUpper;
    ysRatioPoint offsetCrUpper;
    ysRatioPoint offsetCbLower;
    ysRatioPoint offsetCrLower;
    if (_swap_ilace) {
      /* if swapping ilace, then Source LOWER field --> Target UPPER field,
	                         and vice-versa.
      */
      offsetCbUpper = 
	chroma_active_offset(ysSubsampling::PLANE_Cb,
			     ysSubsampling::LOWER_FIELD,
			     ysSubsampling::UPPER_FIELD,
			     source, target);
      offsetCrUpper =
	chroma_active_offset(ysSubsampling::PLANE_Cr,
			     ysSubsampling::LOWER_FIELD,
			     ysSubsampling::UPPER_FIELD,
			     source, target);
      offsetCbLower =
	chroma_active_offset(ysSubsampling::PLANE_Cb,
			     ysSubsampling::UPPER_FIELD,
			     ysSubsampling::LOWER_FIELD,
			     source, target);
      offsetCrLower =
	chroma_active_offset(ysSubsampling::PLANE_Cr,
			     ysSubsampling::UPPER_FIELD,
			     ysSubsampling::LOWER_FIELD,
			     source, target);
    } else {
      /* if not swapping, then Source LOWER field --> Target LOWER field,
	                   etc.
      */
      offsetCbUpper = 
	chroma_active_offset(ysSubsampling::PLANE_Cb,
			     ysSubsampling::UPPER_FIELD,
			     ysSubsampling::UPPER_FIELD,
			     source, target);
      offsetCrUpper =
	chroma_active_offset(ysSubsampling::PLANE_Cr,
			     ysSubsampling::UPPER_FIELD,
			     ysSubsampling::UPPER_FIELD,
			     source, target);
      offsetCbLower =
	chroma_active_offset(ysSubsampling::PLANE_Cb,
			     ysSubsampling::LOWER_FIELD,
			     ysSubsampling::LOWER_FIELD,
			     source, target);
      offsetCrLower =
	chroma_active_offset(ysSubsampling::PLANE_Cr,
			     ysSubsampling::LOWER_FIELD,
			     ysSubsampling::LOWER_FIELD,
			     source, target);
    }
    
    ysRatioPoint src_ssRatio = source.stream().subsampling().ratio();
    ysRatioPoint tgt_ssRatio = target.stream().subsampling().ratio();

    ysRatioPoint c_scale = 
      ysRatioPoint(target.x_ratio(), target.y_ratio()) *
      tgt_ssRatio / src_ssRatio;
    
    _scalers[SC_UPPER_CB] = _factory->new_scaler();
    _scalers[SC_UPPER_CB]->setup(source.stream().fielddim(1),
				 offsetCbUpper,
				 ysRegion(source.matte_region().dim() 
					  * src_ssRatio / ysPoint(1,2),
					  source.matte_region().offset()
					  * src_ssRatio / ysPoint(1,2)),
				 target.stream().fielddim(1),
				 ysRegion(target.active_region().dim()
					  * tgt_ssRatio / ysPoint(1,2),
					  target.active_region().offset()
					  * tgt_ssRatio / ysPoint(1,2)),
				 c_scale.x(), c_scale.y(),
				 source.bgcolor()(1)
				 );
    _scalers[SC_UPPER_CR] = _factory->new_scaler();
    _scalers[SC_UPPER_CR]->setup(source.stream().fielddim(2),
				 offsetCrUpper,
				 ysRegion(source.matte_region().dim() 
					  * src_ssRatio / ysPoint(1,2),
					  source.matte_region().offset()
					  * src_ssRatio / ysPoint(1,2)),
				 target.stream().fielddim(2),
				 ysRegion(target.active_region().dim()
					  * tgt_ssRatio / ysPoint(1,2),
					  target.active_region().offset()
					  * tgt_ssRatio / ysPoint(1,2)),
				 c_scale.x(), c_scale.y(),
				 source.bgcolor()(2)
				 );
    _scalers[SC_LOWER_CB] = _factory->new_scaler();
    _scalers[SC_LOWER_CB]->setup(source.stream().fielddim(1),
				 offsetCbLower,
				 ysRegion(source.matte_region().dim() 
					  * src_ssRatio / ysPoint(1,2),
					  source.matte_region().offset()
					  * src_ssRatio / ysPoint(1,2)),
				 target.stream().fielddim(1),
				 ysRegion(target.active_region().dim()
					  * tgt_ssRatio / ysPoint(1,2),
					  target.active_region().offset()
					  * tgt_ssRatio / ysPoint(1,2)),
				 c_scale.x(), c_scale.y(),
				 source.bgcolor()(1)
				 );
    _scalers[SC_LOWER_CR] = _factory->new_scaler();
    _scalers[SC_LOWER_CR]->setup(source.stream().fielddim(2),
				 offsetCrLower,
				 ysRegion(source.matte_region().dim() 
					  * src_ssRatio / ysPoint(1,2),
					  source.matte_region().offset()
					  * src_ssRatio / ysPoint(1,2)),
				 target.stream().fielddim(2),
				 ysRegion(target.active_region().dim()
					  * tgt_ssRatio / ysPoint(1,2),
					  target.active_region().offset()
					  * tgt_ssRatio / ysPoint(1,2)),
				 c_scale.x(), c_scale.y(),
				 source.bgcolor()(2)
				 );
  }
  
  if (planes > 3) {
    _scalers[SC_INTER_A] = _factory->new_scaler();
    _scalers[SC_INTER_A]->setup(source.stream().fielddim(0),
                                source.active_region().offset() / ysPoint(1,2),
                                ysRegion(source.matte_region().dim() / ysPoint(1,2),
                                         source.matte_region().offset() / ysPoint(1,2)),
                                target.stream().fielddim(0),
                                ysRegion(target.active_region().dim() / ysPoint(1,2),
                                         target.active_region().offset() / ysPoint(1,2)),
                                target.x_ratio(), target.y_ratio(),
                                source.bgcolor()(3)
                                );
  }
}




/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


void ysScaling::_process_frames(int fd_in, int fd_out,
				ysSource &source, ysTarget &target)
{
  int err;
  y4m_frame_info_t frameinfo;
  uint8_t *in_frame[MAX_PLANES];
  uint8_t *out_frame[MAX_PLANES];
  int planes_in = source.stream().planes();
  int planes_out = target.stream().planes();

  y4m_init_frame_info(&frameinfo);

  for (int i = 0; i < MAX_PLANES; i++) {
    out_frame[i] = NULL;
    in_frame[i] = NULL;
  }
  for (int i = 0; i < planes_in; i++) {
    in_frame[i] = new uint8_t[source.stream().framedim(i).area()];
  }
  for (int i = 0; i < planes_out; i++) {
    out_frame[i] = new uint8_t[target.stream().framedim(i).area()];
    if ( ((i == PLANE_CB) || (i == PLANE_CR)) &&
         (_mono) ) {
      memset(out_frame[i], 128, target.stream().framedim(i).area());
    } else {
      memset(out_frame[i],
             target.bgcolor()(i), target.stream().framedim(i).area());
    }
  }

  int frame_num = 0;
  while (1) {
    mjpeg_info ("Frame number %d", frame_num);
    err = source.read_frame(fd_in, &frameinfo, in_frame);
    if (err != Y4M_OK) goto done;

    /* Scale luma always. */
    _scalers[SC_PROG_Y]->scale(in_frame[PLANE_Y], out_frame[PLANE_Y]);
    /* Scale chroma maybe. */
    if ((planes_in > 1) && (planes_out > 1) && (!_mono)) {
      _scalers[SC_PROG_CB]->scale(in_frame[PLANE_CB], out_frame[PLANE_CB]);
      _scalers[SC_PROG_CR]->scale(in_frame[PLANE_CR], out_frame[PLANE_CR]);
    }
    /* Scale alpha maybe. */
    if ((planes_in > 3) && (planes_out > 3)) {
      _scalers[SC_PROG_A]->scale(in_frame[PLANE_A], out_frame[PLANE_A]);
    }

    err = target.write_frame(fd_out, &frameinfo, out_frame);
    if (err != Y4M_OK) goto done;
    
    frame_num++;
  }
  
 done:
  if (err == Y4M_ERR_EOF)
    mjpeg_info("End of stream at frame %d.", frame_num);
  else
    mjpeg_error_exit1("Failure at frame %d:  %s",
		      frame_num, y4m_strerr(err));
  
  for (int i = 0; i < MAX_PLANES; i++) {
    delete[] in_frame[i];
    delete[] out_frame[i];
  }
  y4m_fini_frame_info(&frameinfo);
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


void ysScaling::_process_fields(int fd_in, int fd_out,
				ysSource &source, ysTarget &target)
{
  int err;
  y4m_frame_info_t frameinfo;

  uint8_t **in_upper = new uint8_t *[MAX_PLANES];
  uint8_t **in_lower = new uint8_t *[MAX_PLANES];
  uint8_t **in_other = new uint8_t *[MAX_PLANES];
  uint8_t *out_upper[MAX_PLANES];
  uint8_t *out_lower[MAX_PLANES];
  int planes_in = source.stream().planes();
  int planes_out = target.stream().planes();

  y4m_init_frame_info(&frameinfo);

  for (int i = 0; i < MAX_PLANES; i++) {
    in_upper[i] = NULL;
    in_lower[i] = NULL;
    in_other[i] = NULL;
    out_upper[i] = NULL;
    out_lower[i] = NULL;
  }
  for (int i = 0; i < planes_in; i++) {
    in_upper[i] = new uint8_t[source.stream().fielddim(i).area()];
    in_lower[i] = new uint8_t[source.stream().fielddim(i).area()];
    in_other[i] = new uint8_t[source.stream().fielddim(i).area()];
  }
  for (int i = 0; i < planes_out; i++) {
    out_upper[i] = new uint8_t[target.stream().fielddim(i).area()];
    out_lower[i] = new uint8_t[target.stream().fielddim(i).area()];
    if ( ((i == PLANE_CB) || (i == PLANE_CR)) &&
         (_mono) ) {
      memset(out_upper[i], 128, target.stream().fielddim(i).area());
      memset(out_lower[i], 128, target.stream().fielddim(i).area());
    } else {
      memset(out_upper[i], 
             target.bgcolor()(i), target.stream().fielddim(i).area());
      memset(out_lower[i],
             target.bgcolor()(i), target.stream().fielddim(i).area());
    }
  }

  int frame_num = 0;
  while (1) {

    mjpeg_info("Frame number %d", frame_num);
    err = source.read_fields(fd_in, &frameinfo, in_upper, in_lower);
    if (err != Y4M_OK) goto done;

    if (_line_switching) {
      uint8_t **extra = in_upper;
      in_upper = in_lower;
      in_lower = extra;
    }

    if (_swap_ilace) {
      if (target.stream().interlace() == Y4M_ILACE_TOP_FIRST) {
	/* target is top-field-first */
	if (frame_num == 0) {
	  err = source.read_fields(fd_in, &frameinfo, in_other, in_lower);
	  if (err != Y4M_OK) goto done;
	}
	uint8_t **extra = in_upper;
	in_upper = in_other;
	in_other = extra;
      } else {
	/* target is bottom-field-first */
	if (frame_num == 0) {
	  err = source.read_fields(fd_in, &frameinfo, in_upper, in_other);
	  if (err != Y4M_OK) goto done;
	}
	uint8_t **extra = in_lower;
	in_lower = in_other;
	in_other = extra;
      }
    }

    /* Scale luma always. */
    _scalers[SC_INTER_Y]->scale(in_upper[PLANE_Y], out_upper[PLANE_Y]);
    _scalers[SC_INTER_Y]->scale(in_lower[PLANE_Y], out_lower[PLANE_Y]);
    /* Scale chroma maybe. */
    if ((planes_in > 1) && (planes_out > 1) && (!_mono)) {
      _scalers[SC_UPPER_CB]->scale(in_upper[PLANE_CB], out_upper[PLANE_CB]);
      _scalers[SC_UPPER_CR]->scale(in_upper[PLANE_CR], out_upper[PLANE_CR]);
      _scalers[SC_LOWER_CB]->scale(in_lower[PLANE_CB], out_lower[PLANE_CB]);
      _scalers[SC_LOWER_CR]->scale(in_lower[PLANE_CR], out_lower[PLANE_CR]);
    }
    /* Scale alpha maybe. */
    if ((planes_in > 3) && (planes_out > 3)) {
      _scalers[SC_INTER_A]->scale(in_upper[PLANE_A], out_upper[PLANE_A]);
      _scalers[SC_INTER_A]->scale(in_lower[PLANE_A], out_lower[PLANE_A]);
    }
    /* Write fields. */
    err = target.write_fields(fd_out, &frameinfo, out_upper, out_lower);

    if (err != Y4M_OK) goto done;
    frame_num++;
  }

 done:
  if (err == Y4M_ERR_EOF)
    mjpeg_info("End of stream at frame %d.", frame_num);
  else {
    mjpeg_error("Failure at frame %d:  %s", frame_num, y4m_strerr(err));
    perror("sys:");
    exit(1);
  }

  for (int i = 0; i < MAX_PLANES; i++) {
    delete[] in_upper[i];
    delete[] in_lower[i];
    delete[] in_other[i];
    delete[] out_upper[i];
    delete[] out_lower[i];
  }
  delete[] in_upper;
  delete[] in_lower;
  delete[] in_other;
  y4m_fini_frame_info(&frameinfo);
}



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


void ysScaling::_process_mixed(int fd_in, int fd_out,
                               ysSource &source, ysTarget &target)
{
  int err;
  y4m_frame_info_t frameinfo;

  uint8_t *in_frame[MAX_PLANES];
  uint8_t *in_upper[MAX_PLANES];
  uint8_t *in_lower[MAX_PLANES];
  uint8_t *out_frame[MAX_PLANES];
  uint8_t *out_upper[MAX_PLANES];
  uint8_t *out_lower[MAX_PLANES];

  int planes_in = source.stream().planes();
  int planes_out = target.stream().planes();

  y4m_init_frame_info(&frameinfo);

  for (int i = 0; i < MAX_PLANES; i++) {
    out_frame[i] = NULL;
    out_upper[i] = NULL;
    out_lower[i] = NULL;
    in_frame[i] = NULL;
    in_upper[i] = NULL;
    in_lower[i] = NULL;
  }
  for (int i = 0; i < planes_in; i++) {
    in_frame[i] = new uint8_t[source.stream().framedim(i).area()];
    in_upper[i] = new uint8_t[source.stream().fielddim(i).area()];
    in_lower[i] = new uint8_t[source.stream().fielddim(i).area()];
  }
  for (int i = 0; i < planes_out; i++) {
    out_frame[i] = new uint8_t[target.stream().framedim(i).area()];
    out_upper[i] = new uint8_t[target.stream().fielddim(i).area()];
    out_lower[i] = new uint8_t[target.stream().fielddim(i).area()];
    if ( ((i == PLANE_CB) || (i == PLANE_CR)) &&
         (_mono) ) {
      memset(out_frame[i], 128, target.stream().framedim(i).area());
      memset(out_upper[i], 128, target.stream().fielddim(i).area());
      memset(out_lower[i], 128, target.stream().fielddim(i).area());
    } else {
      memset(out_frame[i],
             target.bgcolor()(i), target.stream().framedim(i).area());
      memset(out_upper[i], 
             target.bgcolor()(i), target.stream().fielddim(i).area());
      memset(out_lower[i],
             target.bgcolor()(i), target.stream().fielddim(i).area());
    }
  }

  int frame_num = 0;
  while (1) {

    mjpeg_info("Frame number %d", frame_num);
    err = source.read_frame_header(fd_in, &frameinfo);
    if (err != Y4M_OK) goto done;
    
    int t_sampling = y4m_fi_get_temporal(&frameinfo);
    int s_sampling = y4m_fi_get_spatial(&frameinfo);
    int sampling;

    if (_vertically_mixed_source && (t_sampling != s_sampling)) {
      sampling = s_sampling;
      mjpeg_info("Anomalous mixed-mode frame!  T=%c S=%c",
                 (t_sampling == Y4M_SAMPLING_PROGRESSIVE) ? 'p' :
                 (t_sampling == Y4M_SAMPLING_INTERLACED) ? 'i' : '?',
                 (s_sampling == Y4M_SAMPLING_PROGRESSIVE) ? 'p' :
                 (s_sampling == Y4M_SAMPLING_INTERLACED) ? 'i' : '?');
      if (_anomalous_mixtures_are_fatal) {
        mjpeg_error("Only chroma upsampling (vertical) of anomalous frames");
        mjpeg_error(" is allowed.  (See manpage.)");
        exit(1);
      }
    } else {
      sampling = t_sampling;
    }

    /* Always produce pp or ii output! */
    /* And, always preserve t_sampling mode in output...
       Any processing here using s_sampling mode will only be one-time deal
        for chroma-upsampling. */
    y4m_fi_set_spatial(&frameinfo, t_sampling);
    y4m_fi_set_temporal(&frameinfo, t_sampling);

    if (sampling == Y4M_SAMPLING_PROGRESSIVE) {
      /* Read frame. */
      err = source.read_frame_data(fd_in, &frameinfo, in_frame);
      if (err != Y4M_OK) goto done;
      /* Scale luma always. */
      _scalers[SC_PROG_Y]->scale(in_frame[PLANE_Y], out_frame[PLANE_Y]);
      /* Scale chroma maybe. */
      if ((planes_in > 1) && (planes_out > 1) && (!_mono)) {
        _scalers[SC_PROG_CB]->scale(in_frame[PLANE_CB], out_frame[PLANE_CB]);
        _scalers[SC_PROG_CR]->scale(in_frame[PLANE_CR], out_frame[PLANE_CR]);
      }
      /* Scale alpha maybe. */
      if ((planes_in > 3) && (planes_out > 3)) {
        _scalers[SC_PROG_A]->scale(in_frame[PLANE_A], out_frame[PLANE_A]);
      }
      /* Write frame. */
      err = target.write_frame(fd_out, &frameinfo, out_frame);

    } else { /* == Y4M_SAMPLING_INTERLACED */
      /* Read fields. */
      err = source.read_fields_data(fd_in, &frameinfo, in_upper, in_lower);
      if (err != Y4M_OK) goto done;
      /* Scale luma always. */
      _scalers[SC_INTER_Y]->scale(in_upper[PLANE_Y], out_upper[PLANE_Y]);
      _scalers[SC_INTER_Y]->scale(in_lower[PLANE_Y], out_lower[PLANE_Y]);
      /* Scale chroma maybe. */
      if ((planes_in > 1) && (planes_out > 1) && (!_mono)) {
        _scalers[SC_UPPER_CB]->scale(in_upper[PLANE_CB], out_upper[PLANE_CB]);
        _scalers[SC_UPPER_CR]->scale(in_upper[PLANE_CR], out_upper[PLANE_CR]);
        _scalers[SC_LOWER_CB]->scale(in_lower[PLANE_CB], out_lower[PLANE_CB]);
        _scalers[SC_LOWER_CR]->scale(in_lower[PLANE_CR], out_lower[PLANE_CR]);
      }
      /* Scale alpha maybe. */
      if ((planes_in > 3) && (planes_out > 3)) {
        _scalers[SC_INTER_A]->scale(in_upper[PLANE_A], out_upper[PLANE_A]);
        _scalers[SC_INTER_A]->scale(in_lower[PLANE_A], out_lower[PLANE_A]);
      }
      /* Write fields. */
      err = target.write_fields(fd_out, &frameinfo, out_upper, out_lower);
    }

    if (err != Y4M_OK) goto done;
    frame_num++;
  }

 done:
  if (err == Y4M_ERR_EOF)
    mjpeg_info("End of stream at frame %d.", frame_num);
  else {
    mjpeg_error("Failure at frame %d:  %s", frame_num, y4m_strerr(err));
    exit(1);
  }

  for (int i = 0; i < MAX_PLANES; i++) {
    delete[] in_frame[i];
    delete[] in_upper[i];
    delete[] in_lower[i];
    delete[] out_frame[i];
    delete[] out_upper[i];
    delete[] out_lower[i];
  }
  y4m_fini_frame_info(&frameinfo);

}



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/



void ysScaling::create_scalers(const ysSource &source, const ysTarget &target)
{
  destroy_scalers();
  require_factory();

  switch (target.stream().interlace()) {
  case Y4M_ILACE_NONE:
    _create_frame_scalers(source, target);
    break;
  case Y4M_ILACE_TOP_FIRST:
  case Y4M_ILACE_BOTTOM_FIRST:
    _create_field_scalers(source, target);
    break;
  case Y4M_ILACE_MIXED:
    _create_frame_scalers(source, target);
    _create_field_scalers(source, target);
    break;
  }
}



void ysScaling::destroy_scalers()
{
  for (int i = 0; i < SC_MAX_SCALERS; i++) {
    delete _scalers[i];
    _scalers[i] = NULL;
  }
}


void ysScaling::process_stream(int fd_in, int fd_out,
			       ysSource &source, ysTarget &target)
{
  switch (target.stream().interlace()) {
  case Y4M_ILACE_NONE:
    _process_frames(fd_in, fd_out, source, target);
    break;
  case Y4M_ILACE_TOP_FIRST:
  case Y4M_ILACE_BOTTOM_FIRST:
    _process_fields(fd_in, fd_out, source, target);
    break;
  case Y4M_ILACE_MIXED:
    _process_mixed(fd_in, fd_out, source, target);
    break;
  }
}
