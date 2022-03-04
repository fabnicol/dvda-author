#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "cpu_accel.h"

#include <stdio.h>

int
main()
	{
	const char **dft;

	printf("The MJPEGTOOLS_SIMD_DISABLE environment variable is a comma sep list of:\n\n");

	for	(dft = disable_simd_flags; *dft; dft++)
		printf("%s\n", *dft);

	printf("\nall\n");
	exit(0);
	}
