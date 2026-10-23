#ifndef RATE_COMPLEXITY_MODEL_HH
#define RATE_COMPLEXITY_MODEL_HH

//
// C++ Interface: rate_complexity_model
//
// Description: Bit-rate / complexity statistics model of input stream
//              This is used to figure out the quantisation floor needed 
//              to hit the overall bit-rate (== size) target of the sequence.
//
//
// Author: Andrew Stevens <andrew.stevens@mnet-online.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "config.h"
#include <vector>

/*  (C) 2000-2004 Andrew Stevens */

/*  This is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */



class BucketSetSampling;

class RateComplexityModel
{
public:
  RateComplexityModel();
  ~RateComplexityModel();

  void SetRateControlParams( double max_bitrate,
                             double allocation_exp );
  void AddComplexitySample( double xhi );

  double FindControlBitrate( double target_bitrate,
                              double init_control_bitrate,
                              double tolerance = 0.01 );


  unsigned int GetNumSamples() const
  {
	  return m_num_samples;
  }

  double GetMeanComplexity() const
  {
	  return m_mean_xhi;
  }

  double FrameBitRate( double xhi, double control_bitrate );

protected: // Methods

  double PredictedBitrate( double control_bitrate);

protected:  // Variables

  static const unsigned int SAMPLING_POINTS = 128;

  BucketSetSampling *m_model;
  double 			m_total_xhi;
  unsigned int      m_num_samples;
  double 			m_mean_xhi;

  double m_target_bitrate;
  double m_allocation_exp;
  double m_max_bitrate;
};

#endif /* RATE_COMPLEXITY_MODEL_HH */
