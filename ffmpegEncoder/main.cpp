/*
 * CopyRight:	none
 * FileName:	testffmpeg.cpp
 * Desc:
 * Author:		Conscien
 * Date:		2023/5/8
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/5/8
 * */

#include <stdio.h>
#include "h265_cup_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif

static void encode(int inputFrameId, AVCodecContext *enc_ctx, const AVFrame *frame, AVPacket *pkt, FILE *outfile) {
    int ret;

    /* send the frame to the encoder */
    //if(frame)
    //    printf("Send frame %lld\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error during encoding\n");
            exit(1);
        }

        //write to file
        printf("input frame id: %d Write packet %lld (size = %d)\n", inputFrameId, pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);

        av_packet_unref(pkt);
    }
}

#define INPUT_FORMAT_YUV420P
#define CODEC_H265

#if 0

int main() {
    const char *in_file_name = "../ds_480x272.yuv";
    const char *out_file_name = "ds.hevc";

    // 输入输出文件
    FILE *in_file, *out_file;
    in_file = fopen(in_file_name, "rb");
    if (!in_file) {
        fprintf(stderr, "Can not open file %s\n", in_file_name);
        return 0;
    }
    out_file = fopen(out_file_name, "wb");
    if (!out_file) {
        fprintf(stderr, "Can not open file %s\n", out_file_name);
        return 0;
    }

    int size = 480 * 272 * 3 / 2;
    Horizon::H265CpuEncoder encoder;
    uint8_t *pInput = new uint8_t[size];
    uint8_t *pDst = new uint8_t[size];
    memset(pInput, 0, size);
    memset(pDst, 0, size);


    while (!feof(in_file)) {

        memset(pInput, 0, size);
        if (fread(pInput, size, 1, in_file) != 1) {
            break;
        }

        int len = 0;
        memset(pDst, 0, size);
        int ret = encoder.Encode(pInput, size, pDst, len);
        if (ret == 0 && len > 0) {
            fwrite(pDst, 1, len, out_file);
        }

    }

    // flush encode
    int len = 0;
    int ret = encoder.Encode(nullptr, 0, pDst, len);
    if (ret == 0 && len > 0) {
        fwrite(pDst, 1, len, out_file);
    }

    delete[] pInput;
    pInput = nullptr;

    delete[] pDst;
    pDst = nullptr;

    fclose(in_file);
    fclose(out_file);
}

#else

int main() {

    // 输入视频文件信息
    int in_width = 480;
    int in_height = 272;
    int in_fps = 25;

    std::cout << "No1 : " << AVERROR(EAGAIN) << std::endl;
    std::cout << "No2 : " << AVERROR_EOF << std::endl;

#ifdef INPUT_FORMAT_YUV420P
    const char *in_file_name = "../ds_480x272.yuv";
    AVPixelFormat in_pix_fmt = AV_PIX_FMT_YUV420P;
#else
    const char *in_file_name = "../files/Titanic_640x272_rgb24.rgb";
    AVPixelFormat in_pix_fmt = AV_PIX_FMT_RGB24;
#endif

    // 输出编码流文件信息
    int out_width = 480;
    int out_height = 272;
    int out_fps = 25;

#ifdef CODEC_H265
    const char *out_file_name = "ds.hevc";
    AVCodecID codec_id = AV_CODEC_ID_HEVC; // AV_CODEC_ID_HEVC
#else
    const char *out_file_name = "Titanic_out.h264";
    AVCodecID codec_id = AV_CODEC_ID_H264;
#endif

    // 输入输出文件
    FILE *in_file, *out_file;
    in_file = fopen(in_file_name, "rb");
    if (!in_file) {
        fprintf(stderr, "Can not open file %s\n", in_file_name);
        return 0;
    }
    out_file = fopen(out_file_name, "wb");
    if (!out_file) {
        fprintf(stderr, "Can not open file %s\n", out_file_name);
        return 0;
    }

    // video encoder
    const AVCodec *encodec = avcodec_find_encoder(codec_id);
//    const AVCodec *encodec = avcodec_find_encoder_by_name("hevc_videotoolbox"); mac硬件加速

    if (!encodec) {
        av_log(nullptr, AV_LOG_ERROR, "Codec '%s' not found\n", avcodec_get_name(codec_id));
        return 0;
    }

    // video encoder contex
    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encodec);
    if (!encoder_ctx) {
        av_log(nullptr, AV_LOG_ERROR, "Could not allocate video codec context\n");
        return 0;
    }

    // encoder parameters
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_ctx->bit_rate = 1000000; // 1Mbps
    encoder_ctx->width = out_width;
    encoder_ctx->height = out_height;
    encoder_ctx->framerate = AVRational{out_fps, 1};
    encoder_ctx->time_base = AVRational{1, out_fps};
    encoder_ctx->gop_size = 5;
//    encoder_ctx->gop_size = 250;
    encoder_ctx->has_b_frames = 0;
    encoder_ctx->max_b_frames = 0;   // B帧数
//    encoder_ctx->max_b_frames = 3;   // B帧数

    if (encoder_ctx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(encoder_ctx->priv_data, "preset", "slow", 0);
    } else if (encoder_ctx->codec_id == AV_CODEC_ID_HEVC) {
        av_opt_set(encoder_ctx->priv_data, "preset", "slow", 0);
    }

    // open condec
    int ret = avcodec_open2(encoder_ctx, encodec, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not open codec\n");
        avcodec_free_context(&encoder_ctx);
        return 0;
    }

    // allocate variables

    // 编码器实际输入（参数和编码器保持一致）
    AVFrame *frame = av_frame_alloc();

    // 供编码器使用，需要设定width、height、format，否则会出现警告
    frame->width = encoder_ctx->width;
    frame->height = encoder_ctx->height;
    frame->format = encoder_ctx->pix_fmt;

#ifdef INPUT_FORMAT_YUV420P
    // Y,U,V三个通道内存不连续，1字节对齐
    //av_frame_get_buffer(frame, 1);

    //  Y,U,V内存连续，1字节对齐
    av_image_alloc(frame->data, frame->linesize,
                   encoder_ctx->width, encoder_ctx->height,
                   encoder_ctx->pix_fmt, 1);
#endif

    AVFrame *frame_tmp;
    SwsContext *img_convert_ctx;

    bool needSws = in_width != out_width || in_height != out_height;

    if (needSws) {
        av_frame_get_buffer(frame, 1);   // 由ffmpeg决定内存布局

        // 临时
        frame_tmp = av_frame_alloc();
        av_image_alloc(frame_tmp->data, frame_tmp->linesize,
                       in_width, in_height, in_pix_fmt, 1);

        img_convert_ctx = sws_getContext(in_width, in_height, in_pix_fmt,
                                         frame->width, frame->height, (AVPixelFormat) frame->format,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    AVPacket *pkt = av_packet_alloc();

    // start encoder
    int64_t frame_cnt = 0;
    while (!feof(in_file)) {

#ifdef INPUT_FORMAT_YUV420P
        // Y,U,V三个通道内存不连续，单通道内存1字节对齐连续
        //if(fread(frame->data[0], in_width*in_height, 1, in_file) != 1)
        //    break;
        //if(fread(frame->data[1], in_width*in_height / 4, 1, in_file) != 1)
        //    break;
        //if(fread(frame->data[2], in_width*in_height / 4, 1, in_file) != 1)
        //    break;

        //Y, U, V内存连续（此时可同样使用上述方式）
        //if(fread(frame->data[0], in_width*in_height * 3 / 2, 1, in_file) != 1)
        //    break;


        if (needSws) {
            if (fread(frame_tmp->data[0], in_width * in_height * 3 / 2, 1, in_file) != 1)
                break;
        } else {
            if (fread(frame->data[0], in_width * in_height * 3 / 2, 1, in_file) != 1)
                break;
        }
#else
        // 读RGB数据，转换
        if(fread(frame_tmp->data[0], in_width*in_height * 3, 1, in_file) != 1)
            break;
#endif

        if (needSws) {
            ret = sws_scale(img_convert_ctx,
                            frame_tmp->data, frame_tmp->linesize, 0, in_height,
                            frame->data, frame->linesize);
        }

        frame->pts = frame_cnt++; //必须，否则会有警告，输出视频码率极低，马赛克严重

        encode(frame_cnt - 1, encoder_ctx, frame, pkt, out_file);

        int *pInt = new int;
        *pInt = frame_cnt;
    }

    // flush
    encode(frame_cnt - 1, encoder_ctx, nullptr, pkt, out_file);

    fclose(in_file);
    fclose(out_file);

    avcodec_free_context(&encoder_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    if (needSws) {
        av_frame_free(&frame_tmp);
        sws_freeContext(img_convert_ctx);
    }

}

#endif
