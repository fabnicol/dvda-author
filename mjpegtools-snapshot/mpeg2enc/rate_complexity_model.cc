//
// C++ Interface: rate_complexity_model
//
// Description: Bit-rate / complexity statistics model of input stream
//              This is used to figure out nominal target bit-rate that should be
//				set overall bit-rate (== size) target of the sequence given
//				a specified quantisation floor...
//
//
// Author: Andrew Stevens <andrew.stevens@mnet-online.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "rate_complexity_model.hh"
#include "cpu_accel.h"
#include <cassert>
#include <math.h>
#include <limits>

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


//===============================================================================================

// BucketSetSampling -  Representation of Statistical distribution based on maintaining a bounded set
//                  of buckets each covering a particular sub-range with a mean
//


class BucketSetSampling
{
protected:
  class Bucket
  {
  private:
	  Bucket(); // forbidden
  public:
    Bucket( double sample )
      : sum( sample )
      , min( sample )
      , max( sample )
      , samples( 1.0 )
    {
    }

    double Mean() const { return sum / samples; }
    double Samples() const { return samples; }
    double sum;
    double min;
    double max;
    double samples;
  };

public:
  typedef std::vector<Bucket> BucketVector;


  BucketSetSampling( unsigned int size_limit )
    : m_size_max( size_limit )
  {
  }

  BucketVector &Buckets()
  {
    return buckets;
  }
  
  /*************
  *
  * AddSample - Extend statistical distribution model with a new sample
  *
  ************/
  
  void AddSample( double sample )
  {
  
    if( buckets.size() < m_size_max )
    {
      // Until the sample bucket set reaches the specizied maximum size
      // every statistic is inserted into its own 'sample bucket'.
      InsertSingleSampleBucket( sample );
    }
    else
    {
      // Once the specified bucket set size is reached each statistic is
      // either inserted into a bucket within whose min/max range it fits
      // or two buckets are merged to make space for a single-sample bucket.
      CombineBucket( sample );
    }
  }



protected:

  static double NeighbourDistance( std::vector<Bucket>::iterator l)
  {
    // As 'distance' we use the product of the sum of the seperation
    // mean seperation multiplied by the log of the samples involved.
    // The idea is to keep seperate buckets that either cover a large
    // population or very different image complexities
    std::vector<Bucket>::iterator r = l + 1;
    double distance = (r->min - l->max) * log( l->samples + r->samples ) ;
    return distance;
  }


  void InsertSingleSampleBucket( double sample )
  {
      // Binary search for the insertion point for the new single-sample
      // ub and lb are *inclusive* bounds on the possible insertion point
    Bucket  newbucket( sample );
    unsigned int lb = 0 ;
    unsigned int ub = buckets.size();
    while( ub-lb > 0 )
    {
        unsigned int pivot = (ub-lb)/2;
        if( buckets[pivot].min >= sample )
        {
          ub = pivot;
        }
        else
        {
          lb = pivot;
        }
    }

  // Insert new sample
  buckets.insert( buckets.begin() + lb,  newbucket );
  }


  void CombineBucket( double sample )
  {
      // Binary search for a bucket 
      // ub and lb are *inclusive* bounds on the range
      // of buckets that cannot contain the new xhi
    assert( buckets.size() > 1 );
    unsigned int lb = 0 ;
    unsigned int ub = buckets.size()-1;
    unsigned int pivot = 0;
    while( ub-lb > 0 )
    {
        pivot = (ub-lb)/2;
        if( buckets[pivot].min > sample )
        {
            ub = pivot;
        }
        else if( buckets[pivot].max < sample )
        {
            lb = pivot;
        }
        else
        {
          // Containing bucket found!!
          break;
        }
    }
  
      // No containing bucket found? ==> Make space by mergeing
      // neighbours producing smallest variance in combined bucket 
      if( ub-lb == 0 )
      {
        MergeClosestNeighbours();
        Bucket  newbucket( sample );
        buckets.insert( buckets.begin()+lb, newbucket );
      }
      else
      {
        Bucket &bucket = buckets[pivot];
        bucket.sum += sample;
        bucket.samples += 1.0;
      }
  
  }



  void MergeClosestNeighbours()
  {
      assert( buckets.size() > 1 );
      //
      // Find the closest neighbours according to the 'NeighbourDistance'
      // metric
      std::vector<Bucket>::iterator i,last,min_dist_i;
      double min_dist = std::numeric_limits<double>::max();

      for( i = buckets.begin(); i+1 < buckets.end(); ++i )
      {
        double dist = NeighbourDistance( i );
        if( dist < min_dist )
        {
        	min_dist = dist;
        	min_dist_i = i;
        }
      }
  
      // Merge the neighbours!
      std::vector<Bucket>::iterator m1 = min_dist_i+1;
      min_dist_i->sum += m1->sum;
      min_dist_i->max = m1->max;
      min_dist_i->samples += m1->samples;
      buckets.erase(m1);
  }


protected:
  unsigned m_size_max;
  BucketVector buckets;
};


//===============================================================================================

// RateComplexityModel - Model of rate / complexity behaviour of encoded MPEG video sequence
//
// TODO: Allocation function (calculates allocated bits from complexity metric)
// is currently hard-wired.
//



RateComplexityModel::RateComplexityModel()
  : m_model( new BucketSetSampling( SAMPLING_POINTS ) )
  , m_total_xhi( 0.0 )
  , m_mean_xhi( 1.0 )
  , m_max_bitrate( 0.0 )
{
}

RateComplexityModel::~RateComplexityModel()
{
  delete m_model;
}

void RateComplexityModel::SetRateControlParams( double max_bitrate,
                                                double allocation_exp )
{
  m_max_bitrate = max_bitrate;
  m_allocation_exp = allocation_exp;
}

void RateComplexityModel::AddComplexitySample( double xhi )
{
  m_model->AddSample( xhi );
  m_total_xhi += xhi;
  m_num_samples += 1;
  m_mean_xhi = m_total_xhi / m_num_samples;
}


/*********************************************
*
* FrameBitRate - Calculate the bitrate for bit-allocation
* for a frame of complexity \c xhi from \c control_bitrate
* approximating
*
***********************************************/

double RateComplexityModel::FrameBitRate( double xhi, double control_bitrate )
{
  return fmin( control_bitrate * xhi * pow( xhi/m_mean_xhi, m_allocation_exp ),
               m_max_bitrate );
}


/*********************************************
*
* PredictedBitrate - Calculate the expected bitrate based
*                  on the complexity statistics
*
***********************************************/

double RateComplexityModel::PredictedBitrate( double control_bitrate)
{
  BucketSetSampling::BucketVector &buckets = m_model->Buckets();
  BucketSetSampling::BucketVector::iterator i;
  double sum_rates = 0.0;
  for( i = buckets.begin(); i  < buckets.end(); ++i )
  {
    sum_rates +=
		FrameBitRate( i->Mean(), control_bitrate ) * i->Samples();
  }

  return sum_rates / m_num_samples;
}

/*********************************************
*
* UpdateRateCoefficient Find the rate-coefficient for
* which a sequence with the measured statistics should
* produce the target bitrate
*
***********************************************/

double RateComplexityModel::FindControlBitrate( double target_bitrate,
                                                 double init_control_bitrate,
                                                 double tolerance )
{
  double control_bitrate = init_control_bitrate;
  double rate = PredictedBitrate(control_bitrate);

  // Find the correct initial direction and a reasonable initial step-size
  // for the search
  double delta = init_control_bitrate * (target_bitrate-rate)/target_bitrate;


  // PredictedBitrate is monotonic in the rate coefficient...
  for(;;)
  {

    if( fabs( rate - target_bitrate ) / target_bitrate < tolerance )
      return control_bitrate;

    double new_rate = PredictedBitrate(control_bitrate+delta);
    if( fabs(new_rate-target_bitrate) < fabs( rate - target_bitrate ) )
    {
      control_bitrate += delta;
      rate = new_rate;
      // If sign of difference changes we need a smaller step in reverse
      // direction next time...
      // N.b. If the sign is the same we retain the current step in case
      // the solution is too far to reach in less that the current step size.
      if( (new_rate-target_bitrate < 0) ^ (rate-target_bitrate < 0) )
      {
        delta = - delta / 2.0;
      }
    }
    else
    {
      // If the solution didn't improve we've over-shot and need a smaller step-size
      delta /= 2.0;
    }
  }
}
