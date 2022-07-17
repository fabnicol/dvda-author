#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "cpu_accel.h"

#include "mmxsse_motion.h"
#include "mjpeg_logging.h"

#define SIMD_DO(x,y) if(disable_simd( #x )) mjpeg_info(" Disabling " #x); else p##x = x##_##y

#define SIMD_MMX(x) SIMD_DO(x,mmx)
#define SIMD_MMXE(x) SIMD_DO(x,mmxe)

void enable_mmxsse_motion(int cpucap)
{
    if(cpucap & ACCEL_X86_MMXEXT ) /* AMD MMX or SSE... */
    {
        mjpeg_info( "SETTING EXTENDED MMX for MOTION!");

        SIMD_MMXE(sad_00);

        SIMD_MMXE(sad_01);

        SIMD_MMXE(sad_10);

        SIMD_MMXE(sad_11);

        SIMD_MMXE(sad_sub22);

        SIMD_MMXE(sad_sub44);

        SIMD_MMXE(find_best_one_pel);

        SIMD_MMX(sumsq);

        SIMD_MMX(sumsq_sub22);

        SIMD_MMX(bsumsq);

        SIMD_MMX(bsumsq_sub22);

        SIMD_MMX(variance);

        SIMD_MMXE(bsad);

        SIMD_MMXE(build_sub22_mests);

        SIMD_MMX(build_sub44_mests);

        SIMD_MMXE(mblocks_sub44_mests);
    }
    else if(cpucap & ACCEL_X86_MMX) /* Ordinary MMX CPU */
    {
        mjpeg_info( "SETTING MMX for MOTION!");

        SIMD_MMX(sad_sub22);

        SIMD_MMX(sad_sub44);

        SIMD_MMX(sad_00);

        SIMD_MMX(sad_01);

        SIMD_MMX(sad_10);

        SIMD_MMX(sad_11);

        SIMD_MMX(bsad);

        SIMD_MMX(sumsq);

        SIMD_MMX(sumsq_sub22);

        SIMD_MMX(bsumsq);

        SIMD_MMX(bsumsq_sub22);

        SIMD_MMX(variance);

        // NOT CHANGED pfind_best_one_pel;
        // NOT CHANGED pbuild_sub22_mests	= build_sub22_mests;

        SIMD_MMX(build_sub44_mests);

        SIMD_MMX(mblocks_sub44_mests);
    }
}
