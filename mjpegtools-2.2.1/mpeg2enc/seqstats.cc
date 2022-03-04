
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


#include "config.h"
#include "seqstats.hh"

BinaryDistNode::BinaryDistNode() :
  depth(0)
  {}

BinaryDistNode::BinaryDist()
  {
    buckets = 32;
    dist.resize(buckets);
    for( i = 0; i< buckets; ++i )
      dist[i] = 0.0;
    lower ;
    
  }

void BinaryDistNode::InsertSample( double x )
{
    if( 
}

bool BinaryDistNode::InsertSample( double x, int node_idx  )
{

  BinaryDistNode &node = dist[node_idx];

  // Leaf nodes act as 'buckets'
  if( node.max_child_depth == 0 )
  {
    if( x < node.lower || x > node.upper)
    {
      // Split bucket...
      node.sigma += x;
      node.n += 1;
      double lower = fmin(x, node.lower);
      double upper = fmax(x, node.upper);

      int lower = node.n / 2.0;
      int upper = node.n - lower;
      double median = node.sigma * lower / node.n;
      double right
      NewLeafNode( x, 1, x, , node.lower, node_idx*2+1 );
      NewLeafNode( node.sigma, node.n, node.lower, node.upper, node_idx*2+2 );
      node.max_child_depth = 1;


      return true;
    }
    else if(  )
    {
      node.max_child_depth = 1;
      node.used_children
      node.sigma += x;
      node.n += 1;
      NewLeafNode( x, x, lower, node_idx*2+2);
      return true;
    }
    else
    
  }


}
