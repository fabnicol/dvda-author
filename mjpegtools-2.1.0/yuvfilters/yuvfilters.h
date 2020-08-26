/* -*- mode:C -*- */
/*
 *  Copyright (C) 2001 Kawamata/Hitoshi <hitoshi.kawamata@nifty.ne.jp>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __YUVFILTERS_H__
#define __YUVFILTERS_H__

#include <stddef.h>
#ifdef MJPEGTOOLS
# include <string.h>
# include <errno.h>
# include <mjpeg_logging.h>
#endif
#include <yuv4mpeg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MJPEGTOOLS
# define perror(MSG) mjpeg_error("%s: %s", MSG, strerror(errno))
# define WERROR(MSG) mjpeg_error(MSG)
# define WERRORL(MSG) mjpeg_error(MSG)
# define WWARN(MSG) mjpeg_warn(MSG)
#else
# define WERROR(MSG) write(2, MSG "\n", sizeof MSG)
# define WERRORL(MSG) do { write(2, MSG, strlen(MSG)); write(2, "\n", 1); } while (0)
# define WWARN(MSG) WERROR("warning: " MSG)
#endif

typedef struct {
  y4m_frame_info_t fi;
  uint8_t data[0];
} YfFrame_t;

#define CWDIV(C) (((C)==Y4M_CHROMA_444)?1:((C)==Y4M_CHROMA_411)?4:2)
#define CHDIV(C) \
(((C)==Y4M_CHROMA_444||(C)==Y4M_CHROMA_422||(C)==Y4M_CHROMA_411)?1:2)
#define DATABYTES(C,W,H) \
(((W)*(H))+(((C)==Y4M_CHROMA_MONO)?0:(((W)/CWDIV(C))*((H)/CHDIV(C))*2)))
#define FRAMEBYTES(C,W,H) (sizeof ((YfFrame_t *)0)->fi + DATABYTES(C,W,H))

struct YfTaskClass_tag;

typedef struct YfTaskCore_tag {
  /* private: filter may not touch */
  const struct YfTaskClass_tag *method;
  struct YfTaskCore_tag *handle_outgoing;
  /* protected: filter must set */
  y4m_stream_info_t si;
  int width, height, fpscode;
} YfTaskCore_t;

typedef struct YfTaskClass_tag {
  const char *(*usage)(void);
  YfTaskCore_t *(*init)(int argc, char **argv, const YfTaskCore_t *h0);
  void (*fini)(YfTaskCore_t *handle);
  int (*frame)(YfTaskCore_t *handle, const YfTaskCore_t *h0, const YfFrame_t *frame0);
} YfTaskClass_t;


#define DECLARE_YFTASKCLASS(name) \
extern const YfTaskClass_t name

#define DEFINE_YFTASKCLASS(sclass, prefix, name) \
sclass const char *prefix##_usage(void); \
sclass YfTaskCore_t *prefix##_init(int argc, char **argv, const YfTaskCore_t *h0); \
sclass void prefix##_fini(YfTaskCore_t *handle); \
sclass int prefix##_frame(YfTaskCore_t *handle, const YfTaskCore_t *h0, const YfFrame_t * frame0); \
const YfTaskClass_t name = { prefix##_usage, prefix##_init, prefix##_fini, prefix##_frame, }

#define DEFINE_STD_YFTASKCLASS(name) DEFINE_YFTASKCLASS(static,do,name)

extern int verbose;

extern YfTaskCore_t *YfAllocateTask(const YfTaskClass_t *filter, size_t size, const YfTaskCore_t *h0);
extern void YfFreeTask(YfTaskCore_t *handle);
extern YfFrame_t *YfInitFrame(YfFrame_t *frame, const YfTaskCore_t *h0);
extern void YfFiniFrame(YfFrame_t *frame);
extern int YfPutFrame(const YfTaskCore_t *handle, const YfFrame_t *frame);
extern YfTaskCore_t *YfAddNewTask(const YfTaskClass_t *filter,
				  int argc, char **argv, const YfTaskCore_t *h0);
#ifdef __cplusplus
}
#endif

#endif
