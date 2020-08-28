#ifndef MLP_H_INCLUDED
#define MLP_H_INCLUDED

#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "mlplayout.h"

#include "structures.h"
#include "c_utils.h"
#include "auxiliary.h"


int decode_mlp_file(fileinfo_t* info, globalData* globals);
int encode_mlp_file(fileinfo_t* info, globalData* globals);

void transport_to_mlp(fileinfo_t* info, globalData* globals);
#endif // MLP_H_INCLUDED
