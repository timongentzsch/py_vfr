#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class VideoReader
{
private:
    AVFormatContext *pFormatCtx = nullptr;
    int videoStream;
    AVCodecContext *pCodecCtx = nullptr;
    AVFrame *pFrame = nullptr;
    AVFrame *pFrameGray = nullptr;
    SwsContext *sws_ctx = nullptr;

public:
    VideoReader(const std::string &videoPath, bool greyscale = false)
    {
        // Open video file
        if (avformat_open_input(&pFormatCtx, videoPath.c_str(), nullptr, nullptr) != 0)
        {
            throw std::runtime_error("Couldn't open file.");
        }

        // Retrieve stream information
        if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
        {
            throw std::runtime_error("Couldn't find stream information.");
        }

        // Find the first video stream
        videoStream = -1;
        for (int i = 0; i < pFormatCtx->nb_streams; i++)
        {
            if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStream = i;
                break;
            }
        }
        if (videoStream == -1)
        {
            throw std::runtime_error("Didn't find a video stream.");
        }

        // Get a pointer to the codec context for the video stream
        pCodecCtx = avcodec_alloc_context3(nullptr);
        avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

        // Find the decoder for the video stream
        AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (pCodec == nullptr)
        {
            throw std::runtime_error("Codec not found.");
        }

        // Open codec
        if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
        {
            throw std::runtime_error("Could not open codec.");
        }

        pFrame = av_frame_alloc();

        if (greyscale)
        {
            pFrameGray = av_frame_alloc();
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_GRAY8, pCodecCtx->width, pCodecCtx->height, 1);
            uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
            av_image_fill_arrays(pFrameGray->data, pFrameGray->linesize, buffer, AV_PIX_FMT_GRAY8, pCodecCtx->width, pCodecCtx->height, 1);
        }
    }

    ~VideoReader()
    {
        // Clean up
        av_free(pFrame);
        if (pFrameGray)
            av_free(pFrameGray);
        avcodec_close(pCodecCtx);
        avformat_close_input(&pFormatCtx);
    }

    AVFrame *getFrame(double timestamp)
    {
        AVPacket packet;
        while (av_read_frame(pFormatCtx, &packet) >= 0)
        {
            if (packet.stream_index == videoStream)
            {
                if (avcodec_send_packet(pCodecCtx, &packet) == 0)
                {
                    while (avcodec_receive_frame(pCodecCtx, pFrame) == 0)
                    {
                        if ((packet.dts * av_q2d(pFormatCtx->streams[videoStream]->time_base)) >= timestamp)
                        {
                            if (pFrameGray)
                            {
                                sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_GRAY8, SWS_BILINEAR, nullptr, nullptr, nullptr);
                                sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameGray->data, pFrameGray->linesize);
                                return pFrameGray;
                            }
                            else
                            {
                                return pFrame;
                            }
                        }
                    }
                }
            }
            av_packet_unref(&packet);
        }
        return nullptr;
    }

    nlohmann::json getInfo()
    {
        nlohmann::json info;
        info["length"] = (double)pFormatCtx->duration / AV_TIME_BASE;
        info["frame_rate"] = av_q2d(pFormatCtx->streams[videoStream]->avg_frame_rate);
        // Add more info if needed
        return info;
    }
};

int main()
{
    VideoReader reader("sample.mp4", true);
    AVFrame *frame = reader.getFrame(5.0); // Get frame at 5 seconds
    nlohmann::json info = reader.getInfo();
    std::cout << info << std::endl;

    return 0;
}
