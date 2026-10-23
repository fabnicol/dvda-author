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

#include <mjpeg_logging.h>

#include "ysScaling.H"
#include "ysSource.H"
#include "ysTarget.H"


void print_version(char *argv[])
{
  printf("%s is y4mscaler %d.%d\n", argv[0],
         YS_VERSION_MAJOR, YS_VERSION_MINOR);
  printf("Copyright 2004 Matthew J. Marjanovic\n");
}


void print_usage(char *argv[],
		 const ysSource &source, const ysTarget &target,
		 const ysScaling &scaling)
{
  print_version(argv);
  fprintf(stdout, "\n");
  fprintf(stdout, "[This is just a summary --- read the manpage!]\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "usage: %s [options...]\n", argv[0]);
  fprintf(stdout, "\n");
  fprintf(stdout, "  -I input_parameter:\n");
  source.describe_keywords(stdout, "      ");
  fprintf(stdout, "\n");
  fprintf(stdout, "  -O output_parameter:\n");
  target.describe_keywords(stdout, "      ");
  fprintf(stdout, "\n");
  fprintf(stdout, "  -S scaling_parameter:\n");
  scaling.describe_keywords(stdout, "      ");
  fprintf(stdout, "\n");
  fprintf(stdout, "  -v N  verbosity: 0=quiet, 1=normal, 2=debug\n");
  fprintf(stdout, "  -V    show version info and exit\n");
  fprintf(stdout, "  -h    show this help message\n");
}


enum parsing_mode_t {
  GLOBAL = 0,  /* stream-independent arguments */
  SOURCE,      /* '-I' input arguments */
  DEST         /* '-O' output arguments */
};


static void parse_args(int argc, char *argv[],
                       ysSource &source, ysTarget &target,
		       ysScaling &scaling, parsing_mode_t mode)
{
  int verbosity = 1;
  int c;

  optind = 1;
  while ((c = getopt(argc, argv, "I:O:S:v:hV")) != -1) {
    switch (mode) {

      case GLOBAL:     /* process 'global' options only */
	switch (c) {
	case 'v':
	  verbosity = atoi (optarg);
	  if (verbosity < 0 || verbosity > 2) {
	    mjpeg_error_exit1("Verbosity level must be [0..2]");
	  }
	  break;
	case 'h':
	  print_usage(argv, source, target, scaling);
	  exit(0);
	  break;
	case 'V':
	  print_version(argv);
	  exit(0);
	  break;
	case 'S':
	  scaling.parse_keyword(optarg);
	  break;
	case '?':
	  mjpeg_error("Unknown option character:  '%c'", c);
	  mjpeg_error("Use '-h' to get usage hints.");
	  exit(1);
	  break;
	case 'I':
	case 'O':
	default:
	  break;
	}
	break;

    case SOURCE:      /* process stream-dependent options only */
      switch (c) {
      case 'I':
	source.parse_keyword(optarg);
	break;
      case '?':
	mjpeg_error("Unknown option character:  '%c'", c);
	mjpeg_error("Use '-h' to get usage hints.");
	exit(1);
	break;
      case 'O':
      case 'S':
      case 'v':
      case 'h':
      default:
	break;
      }
      break;

    case DEST:      /* process stream-dependent options only */
      switch (c) {
      case 'O':
	target.parse_keyword(source, optarg);
	break;
      case '?':
	mjpeg_error("Unknown option character:  '%c'", c);
	mjpeg_error("Use '-h' to get usage hints.");
	exit(1);
	break;
      case 'I':
      case 'S':
      case 'v':
      case 'h':
      default:
	break;
      }
      break;

    } /* switch */
  } /* while getopt */

  if (mode == GLOBAL)
    mjpeg_default_handler_verbosity(verbosity);
  if (optind != argc) {
      mjpeg_error_exit1("STREAMS ONLY MY FRIEND!");
  }
  return;
}







// ***************************************************************************
// ***************************************************************************
// ***************************************************************************
// ***************************************************************************
// MAIN
// ***************************************************************************
// ***************************************************************************
// ***************************************************************************
// ***************************************************************************

int main(int argc, char *argv[])
{
  int fd_in = 0;   /* stdin  */
  int fd_out = 1;  /* stdout */
  ysSource source;
  ysTarget target;
  ysScaling scaling;


  y4m_accept_extensions(1);

  /* parse stream-independent arguments */
  parse_args(argc, argv, source, target, scaling, GLOBAL);

  /* read source stream header */
  if (source.read_stream_header(fd_in) != Y4M_OK)
    mjpeg_error_exit1("Failed to read YUV4MPEG2 header!");
  mjpeg_info("Input Stream Header:");
  source.stream().log_info(mjpeg_loglev_t("info"), "<<< ");

  /* set target stream defaults from source stream */
  target.init_stream(source);

  /* parse stream-dependent arguments (source, target parameters) */
  parse_args(argc, argv, source, target, scaling, SOURCE);
  parse_args(argc, argv, source, target, scaling, DEST);

  /* apply heuristics and finalize parameters */
  source.check_parameters();
  target.check_parameters(source);
  scaling.check_parameters(source, target);

  /* log results to user */
  source.describe_parameters();
  scaling.describe_parameters();
  target.describe_parameters();

  /* set up target stream */
  target.stream().write_stream_header(fd_out);
  mjpeg_info("Output Stream Header:");
  target.stream().log_info(mjpeg_loglev_t("info"), ">>> ");

  /* do some scaling */
  scaling.create_scalers(source, target);
  scaling.process_stream(fd_in, fd_out, source, target);

  return 0;
}



