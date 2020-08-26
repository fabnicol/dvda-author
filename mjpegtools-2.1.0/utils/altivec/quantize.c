/* quantize.c, this file is part of the
 * AltiVec optimized library for MJPEG tools MPEG-1/2 Video Encoder
 * Copyright (C) 2002  James Klicman <james@klicman.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "cpu_accel.h"

#include "altivec_quantize.h"
#include "vectorize.h"
#include "../mjpeg_logging.h"

void enable_altivec_quantization(struct QuantizerCalls *calls, int opt_mpeg1)
    {
    int d_quant_nonintra, d_weight_intra, d_weight_nonintra, d_iquant_intra;
    int d_iquant_nonintra;

    d_quant_nonintra = disable_simd("quant_nonintra");
    d_weight_intra = disable_simd("quant_weight_intra");
    d_weight_nonintra = disable_simd("quant_weight_nonintra");
    d_iquant_intra = disable_simd("iquant_intra");
    d_iquant_nonintra = disable_simd("iquant_nonintra");

#if ALTIVEC_TEST_QUANTIZE
#  if defined(ALTIVEC_BENCHMARK)
    mjpeg_info("SETTING AltiVec BENCHMARK for QUANTIZER!");
#  elif defined(ALTIVEC_VERIFY)
    mjpeg_info("SETTING AltiVec VERIFY for QUANTIZER!");
#  endif
#else
    mjpeg_info("SETTING AltiVec for QUANTIZER!");
#endif

#if ALTIVEC_TEST_FUNCTION(quant_non_intra)
    calls->pquant_non_intra = ALTIVEC_TEST_SUFFIX(quant_non_intra);
#else
    if (d_quant_nonintra == 0)
        calls->pquant_non_intra = quant_non_intra_altivec;
#endif

#if ALTIVEC_TEST_FUNCTION(quant_weight_coeff_intra)
    calls->pquant_weight_coeff_intra = ALTIVEC_TEST_SUFFIX(quant_weight_coeff_intra);
#else
    if (d_weight_intra == 0)
       calls->pquant_weight_coeff_intra = quant_weight_coeff_intra_altivec;
#endif

#if ALTIVEC_TEST_FUNCTION(quant_weight_coeff_inter)
    calls->pquant_weight_coeff_inter = ALTIVEC_TEST_SUFFIX(quant_weight_coeff_inter);
#else
    if (d_weight_nonintra == 0)
        calls->pquant_weight_coeff_inter = quant_weight_coeff_inter_altivec;
#endif

    if  (opt_mpeg1)
        {
#if ALTIVEC_TEST_FUNCTION(iquant_non_intra_m1)
	calls->piquant_non_intra = ALTIVEC_TEST_SUFFIX(iquant_non_intra_m1);
#else
        if (d_iquant_nonintra == 0)
	    calls->piquant_non_intra = iquant_non_intra_m1_altivec;
#endif
#if ALTIVEC_TEST_FUNCTION(iquant_intra_m1)
	calls->piquant_intra = ALTIVEC_TEST_SUFFIX(iquant_intra_m1);
#else
         if (d_iquant_intra == 0)
	    calls->piquant_intra = iquant_intra_m1_altivec;
#endif
         }
     else
         {
#if ALTIVEC_TEST_FUNCTION(iquant_non_intra_m2)
	calls->piquant_non_intra = ALTIVEC_TEST_SUFFIX(iquant_non_intra_m2);
#else
        if (d_iquant_nonintra == 0)
	   calls->piquant_non_intra = iquant_non_intra_m2_altivec;
#endif
#if ALTIVEC_TEST_FUNCTION(iquant_intra_m2)
	calls->piquant_intra = ALTIVEC_TEST_SUFFIX(iquant_intra_m2);
#else
        if (d_iquant_intra == 0)
	   calls->piquant_intra = iquant_intra_m2_altivec;
#endif
        }

    if (d_quant_nonintra)
       mjpeg_info(" Disabling quant_non_intra");
    if (d_iquant_intra)
       mjpeg_info(" Disabling iquant_intra");
    if (d_iquant_nonintra)
       mjpeg_info(" Disabling iquant_nonintra");
    if (d_weight_intra)
       mjpeg_info(" Disabling quant_weight_intra");
    if (d_weight_nonintra)
        mjpeg_info(" Disabling quant_weight_nonintra");
    }
