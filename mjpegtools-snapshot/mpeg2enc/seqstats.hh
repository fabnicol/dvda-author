#ifndef _SEQSTATS_HH
#define _SEQSTATS_HH

/* seqstats.hh, sequence statistics for bitrate control */



/* (C) 2006 Andrew Stevens */

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


class BinaryDistNode
{
friend class BinaryDist;
protected:
   BinaryDistNode();
  double lower;
  double upper;
  double sigma;
  double n;
  int max_child_depth;
  int used_children;
};


class BinaryDist
{
public:
  BinaryDist();

  InsertSample( double x );

protected:
  int buckets;
  double upper;
  double lower;
  std::vector<double> dist;
};


#endif // _SEQSTATS_HH