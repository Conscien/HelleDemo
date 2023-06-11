/*
 * CopyRight:	none
 * FileName:	h265_cup_encoder.cpp
 * Desc:		
 * Author:		Conscien
 * Date:		2023/5/9
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/5/9
 * */

#include "h265_cup_encoder.h"

namespace Horizon {

    H265CpuEncoder::H265CpuEncoder() { Init(480, 272); }

    H265CpuEncoder::~H265CpuEncoder() { Destroy(); }

    bool H265CpuEncoder::Init(int width, int height) {
        width_ = width;
        height_ = height;
        int ret = -1;
        codec_ = avcodec_find_encoder(AV_CODEC_ID_HEVC);
        if (!codec_)
        {
            std::cout << "Could not find encoder" << std::endl;
            return false;
        }

        codec_ctx_ = avcodec_alloc_context3(codec_);
        if (!codec_ctx_) {
            std::cout << "Could not avcodec_alloc_context3" << std::endl;
            return false;
        }
        int nFrameRate = 25;
//        int64_t nBitRate = (int64_t)((width_ * height_ * 3 / 2) * 0.8); // 计算码率
        int64_t nBitRate = 1000000; // 计算码率
//        codec_ctx_->codec_id = AV_CODEC_ID_HEVC;
        codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
//        codec_ctx_->profile = FF_PROFILE_HEVC_MAIN;
        codec_ctx_->width = width;
        codec_ctx_->height = height;
        codec_ctx_->bit_rate = nBitRate;
//        codec_ctx_->rc_buffer_size = static_cast<int>(nBitRate);
        codec_ctx_->framerate = AVRational{nFrameRate, 1};
        codec_ctx_->time_base = AVRational{1, nFrameRate};
        codec_ctx_->gop_size = nFrameRate; // 每秒1个关键帧
        codec_ctx_->max_b_frames = 0;
//        codec_ctx_->thread_count = 10;
//        codec_ctx_->qmin = 10;
//        codec_ctx_->qmax = 51;

//        codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

//        AVDictionary* pDict = nullptr;
        if (codec_ctx_->codec_id == AV_CODEC_ID_HEVC)
        {
            // Encoding speed: ultrafast, superfast, veryfast, faster, fast, medium,
            // slow, slower, veryslow, placebo.
//            av_dict_set(&pDict, "x265-params", "qp=20", 0);
//            av_dict_set(&pDict, "tune", "zero-latency", 0);
//            av_dict_set(&pDict, "preset", "medium", 0);
//            av_dict_set(&pDict, "tune", "zerolatency", 0);
            av_opt_set(codec_ctx_->priv_data, "preset", "slow", 0);
        }

        ret = avcodec_open2(codec_ctx_, codec_, nullptr);
        if (ret < 0)
        {
            std::cout << "Could not open codec error: " << ret << std::endl;
            avcodec_free_context(&codec_ctx_);
            return false;
        }
//        av_dict_free(&pDict);

        avFrame_ = av_frame_alloc();
        if (!avFrame_) {
            std::cout << "Alloc frame buffer failed" << std::endl;
            return false;
        }
        avFrame_->format = codec_ctx_->pix_fmt;
        avFrame_->width = codec_ctx_->width;
        avFrame_->height = codec_ctx_->height;
        ret = av_frame_get_buffer(avFrame_, 1);
        if (ret < 0) {
            std::cout << "Alloc frame buffer failed error: " << ret << std::endl;
            return false;
        }

        //  Y,U,V内存连续，1字节对齐
        av_image_alloc(avFrame_->data, avFrame_->linesize,
                       codec_ctx_->width, codec_ctx_->height,
                       codec_ctx_->pix_fmt, 1);

        // 临时
        frame_tmp_ = av_frame_alloc();
        av_image_alloc(frame_tmp_->data, frame_tmp_->linesize,
                       width_, height_, codec_ctx_->pix_fmt, 1);
        img_convert_ctx_ = sws_getContext(width_, height_, codec_ctx_->pix_fmt,
                                          avFrame_->width, avFrame_->height, (AVPixelFormat) avFrame_->format,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);

        yuv_path_ = "/home/liangxin/code/data/"; // 指定debug时保存yuv图像数据路径
        std::cout << "Conscien Init avencoder success" << std::endl;
        return true;
    }

    bool H265CpuEncoder::Destroy() {
        avcodec_close(codec_ctx_);
        avcodec_free_context(&codec_ctx_);
        av_frame_free(&avFrame_);
        return true;
    }

    int H265CpuEncoder::Encode(uint8_t *data, size_t data_size, uint8_t *dst_data, int &out_len) {
        int ret = -1;
        out_len = 0;
        ret = av_frame_make_writable(avFrame_);
        if (ret < 0)
        {
            std::cout << "av_frame_make_writable error" << std::endl;
            return ret;
        }

        if (data != nullptr && data_size != 0) {
            memcpy(frame_tmp_->data[0], data, data_size);
            ret = sws_scale(img_convert_ctx_,
                            frame_tmp_->data, frame_tmp_->linesize, 0, height_,
                            avFrame_->data, avFrame_->linesize);
        }


        // 填充avframe
//        int yFillSize = width_ * height_;
//        memcpy(avFrame_->data[0], data, yFillSize);
//        int yuvFillSize = yFillSize * 3 / 2;
//        int uFillIdx = 0;
//        int vFillIdx = 0;
//        for (size_t i = yFillSize; i < yuvFillSize;)
//        {
//            avFrame_->data[1][uFillIdx++] = *(data + i++);
//            avFrame_->data[2][vFillIdx++] = *(data + i++);
//        }

        AVPacket pkt;
        av_init_packet(&pkt);
        int got_output;
        avFrame_->pts = frame_id++;

//        /* 老式编码 encode the image */
//        ret = avcodec_encode_video2(codec_ctx_, &pkt, avFrame_, &got_output);
//        if (ret < 0) {
//            std::cout << "Error encoding frame ret: " << ret << std::endl;
//            return -1;
//        }
//        if (got_output) {
//            std::cout << "Succeed to encode frame id : " << frame_id-1 << " get size: " << pkt.size << std::endl;
//            memcpy(dst_data, pkt.data, pkt.size);
//            out_len = pkt.size;
//            av_packet_unref(&pkt);
//            return 0;
//        }

//        //Flush Encoder
//        for (got_output = 1; got_output; i++) {
//            ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
//            if (ret < 0) {
//                printf("Error encoding frame\n");
//                return -1;
//            }
//            if (got_output) {
//                printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",pkt.size);
//                fwrite(pkt.data, 1, pkt.size, fp_out);
//                av_free_packet(&pkt);
//            }
//        }

        if (data_size ==0 && data == nullptr) {
            // flush
            ret = avcodec_send_frame(codec_ctx_, nullptr);
        } else {
            ret = avcodec_send_frame(codec_ctx_, avFrame_);
        }
        if (ret < 0) {
            std::cout << "avcodec_send_frame failed frameId: "<< frame_id << std::endl;
            return ret;
        }

        int addLen = 0;
        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_ctx_, &pkt);
            if (ret == AVERROR(EAGAIN)) {
                std::cout << "need more input data now frame id: " << frame_id-1 << " ret: " << ret << std::endl;
                break;
            } else if (ret == AVERROR_EOF) {
                std::cout << "input data end now frame id: " << frame_id-1 << " ret: " << ret << std::endl;
                break;
            } else if (ret < 0) {
                std::cout << "avcodec_receive_packet failed frameId: " << frame_id-1 << std::endl;
                return -1;
            }

            memcpy(dst_data + addLen, pkt.data, pkt.size);
            addLen += pkt.size;
            std::cout << "encoder packet " << pkt.pts << +" success len: " << pkt.size << std::endl;
            av_packet_unref(&pkt);
        }

        out_len = addLen;
        std::cout << "out encode data id： " << frame_id-1 << +" out len: " << out_len << std::endl;
        return 0;
    }
}  // namespace Horizon
