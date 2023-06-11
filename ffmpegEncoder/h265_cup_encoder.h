/*
 * CopyRight:	none
 * FileName:	h265_cup_encoder.h
 * Desc:
 * Author:		Conscien
 * Date:		2023/5/9
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/5/9
 * */

#ifndef FFMEPGENCODER_H265_CUP_ENCODER_H
#define FFMEPGENCODER_H265_CUP_ENCODER_H

#include <iostream>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace HelloDemo {
class H265CpuEncoder {
public:
  H265CpuEncoder();

  ~H265CpuEncoder();

  bool Init(int width, int height);

  bool Destroy();

  int Encode(uint8_t *data, size_t data_size, uint8_t *dst_data, int &out_len);

private:
  const AVCodec *codec_;
  AVCodecContext *codec_ctx_;
  SwsContext *img_convert_ctx_;
  AVFrame *avFrame_;
  AVFrame *frame_tmp_;
  std::string yuv_path_;
  int64_t frame_id = 0;
  int width_ = 0;
  int height_ = 0;
};

} // namespace HelloDemo

#endif // FFMEPGENCODER_H265_CUP_ENCODER_H
