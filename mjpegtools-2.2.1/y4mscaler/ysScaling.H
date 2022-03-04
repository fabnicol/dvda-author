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

#ifndef _YS_SCALING_H_
#define _YS_SCALING_H_

#include <stdio.h>
#include "scaler.H"
#include "ysSource.H"
#include "ysTarget.H"


class ysScaling {
private:
  struct factory_info {
    const char *id;
    ysScalerFactory *factory;
  };
  int _factory_count;
  factory_info *_factory_list;

  ysScalerFactory *_factory;
  int _mono;
  int _line_switching;
  int _swap_ilace;


  enum {
    PLANE_Y = 0,
    PLANE_CB,
    PLANE_CR,
    PLANE_A,
    MAX_PLANES
  };

  enum {
    SC_PROG_Y = 0,
    SC_PROG_CB,
    SC_PROG_CR,
    SC_PROG_A,
    SC_INTER_Y,
    SC_UPPER_CB,
    SC_UPPER_CR,
    SC_LOWER_CB,
    SC_LOWER_CR,
    SC_INTER_A,
    SC_MAX_SCALERS
  };
  //  ysScaler **_scalers;
  ysScaler *_scalers[SC_MAX_SCALERS];
  //  int _scaler_count;
  bool _vertically_mixed_source;
  bool _anomalous_mixtures_are_fatal;

  void set_factory(ysScalerFactory *f) {
    //    delete _factory;
    _factory = f;
  }
  void require_factory(void);

  void _create_factory_list(void);
  void _destroy_factory_list(void);

  void _create_frame_scalers(const ysSource &source,
			     const ysTarget &target);
  void _create_field_scalers(const ysSource &source,
			     const ysTarget &target);

  void _process_frames(int fd_in, int fd_out,
		       ysSource &source, ysTarget &target);
  void _process_fields(int fd_in, int fd_out,
		       ysSource &source, ysTarget &target);
  void _process_mixed(int fd_in, int fd_out,
                      ysSource &source, ysTarget &target);
    
public:
  ysScaling(void);
  ~ysScaling();

  ysScalerFactory *factory() const { return _factory; }

  void describe_keywords(FILE *fp, const char *prefix) const;
  void parse_keyword(char *optarg);
  void describe_parameters() const;

  void check_parameters(const ysSource &source, const ysTarget &target);
  void create_scalers(const ysSource &source, const ysTarget &target);
  void process_stream(int fd_in, int fd_out,
		      ysSource &source, ysTarget &target);
  void destroy_scalers();

};


#endif /* _YS_SCALING_H_ */
