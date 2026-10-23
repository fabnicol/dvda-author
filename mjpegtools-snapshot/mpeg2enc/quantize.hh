#ifndef _QUANTIZER_HH
#define _QUANTIZER_HH

/*  (C) 2003 Andrew Stevens */

/*  This Software is free software; you can redistribute it
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

#include "mjpeg_types.h"
#include "quantize_ref.h"

class EncoderParams;
class Quantizer : public QuantizerCalls
{
public:
	Quantizer( EncoderParams &_encoder );
	void Init();
	~Quantizer();

	inline void
	QuantIntra( int16_t *src, int16_t *dst,
				int q_scale_type,
				int dcprec,
				int dctsatlim,
				int *nonsat_mquant)
		{
			quant_intra( workspace, src, dst, 
						 q_scale_type, dcprec, 
						 dctsatlim, nonsat_mquant );
		}

	inline
	int QuantInter( int16_t *src, int16_t *dst,
					int q_scale_type, 
					int dctsatlim,
					int *nonsat_mquant )
		{
			return (*pquant_non_intra)( workspace, src, dst, 
										q_scale_type, dctsatlim, 
										nonsat_mquant );
		}

	
    inline int 
	WeightCoeffIntra( int16_t *blk )
		{
			return 	(*pquant_weight_coeff_intra)(workspace, blk);
		}
	
	inline int
	WeightCoeffInter( int16_t *blk )
		{
			return (*pquant_weight_coeff_inter)(workspace, blk );
		}


	inline void IQuantIntra( int16_t *src, int16_t *dst, int dc_prec, int mquant )
		{
			(*piquant_intra)(workspace, src, dst, dc_prec, mquant );
		}


	inline void
	IQuantInter( int16_t *src, int16_t *dst, int mquant )
		{
			(*piquant_non_intra)(workspace, src, dst, mquant );
		}

private:
	QuantizerWorkSpace *workspace;
	EncoderParams &encparams;
	//int dctsatlim;
};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
