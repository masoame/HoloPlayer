#include"BaseFFmpeg.h"
#include<iostream>
#include<syncstream>
namespace FFmpegLayer
{
    //sample_bit_size[AVSampleFormat(音频采样格式)] == 采样点的大小
    constexpr const char sample_bit_size[13]{ 1,2,4,4,8,1,2,4,4,8,8,8,-1 };
    //planner转化为对应的packed(AV_SAMPLE_FMT_NONE为错误格式)
    constexpr const AVSampleFormat map_palnner_to_packad[13]
    {
        AV_SAMPLE_FMT_NONE,
        AV_SAMPLE_FMT_NONE,
        AV_SAMPLE_FMT_NONE,
        AV_SAMPLE_FMT_NONE,
        AV_SAMPLE_FMT_NONE,
        AV_SAMPLE_FMT_U8,
        AV_SAMPLE_FMT_S16,
        AV_SAMPLE_FMT_S32,
        AV_SAMPLE_FMT_FLT,
        AV_SAMPLE_FMT_DBL,
        AV_SAMPLE_FMT_NONE,
        AV_SAMPLE_FMT_S64,
        AV_SAMPLE_FMT_NONE
    };

    RESULT PlayTool::open(const char* srcUrl, const char* dstUrl, unsigned char type)
    {
        this->close();
        if (type & in)
        {
            avfctx_input = avformat_alloc_context();
            if (avformat_open_input(&avfctx_input, srcUrl, nullptr, nullptr)) return OPEN_ERROR;
            if (avformat_find_stream_info(avfctx_input, nullptr) < 0) return ARGS_ERROR;

            av_dump_format(avfctx_input, 0, srcUrl, false);

            if (type & video)
                init_decode(AVMEDIA_TYPE_VIDEO);
            if (type & audio)
                init_decode(AVMEDIA_TYPE_AUDIO);

        }
        if (type & out)
        {
            avfctx_output = avformat_alloc_context();
            if (avformat_alloc_output_context2(&avfctx_output, nullptr, dstUrl, nullptr) < 0) {};
        }
        return SUCCESS;
    }

    RESULT PlayTool::init_decode(AVMediaType type)
    {

        int index = av_find_best_stream(avfctx_input, type, -1, -1, NULL, 0);
        if (index != AVERROR_STREAM_NOT_FOUND) { AVStreamIndexToType[index] = type; AVTypeToStreamIndex[type] = index; }
        decode_ctx[type] = avcodec_alloc_context3(nullptr);
        if (decode_ctx[type] == nullptr)return ALLOC_ERROR;

        if (avcodec_parameters_to_context(decode_ctx[type], avfctx_input->streams[index]->codecpar) < 0)return ARGS_ERROR;
        const AVCodec* codec = avcodec_find_decoder(decode_ctx[type]->codec_id);
        if (avcodec_open2(decode_ctx[type], codec, NULL))return OPEN_ERROR;

        secBaseTime[type] = av_q2d(avfctx_input->streams[AVTypeToStreamIndex[type]]->time_base);

        return SUCCESS;
    }


    RESULT PlayTool::init_swr()
    {
        AutoAVCodecContextPtr& audio_ctx = decode_ctx[AVMEDIA_TYPE_AUDIO];

        AVSampleFormat* format;
        avcodec_get_supported_config(audio_ctx, nullptr, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0,(const void**)&format, nullptr);

        if (audio_ctx == nullptr)return ARGS_ERROR;
        if (*format == AV_SAMPLE_FMT_NONE || map_palnner_to_packad[*format] == AV_SAMPLE_FMT_NONE)return UNNEED_SWR;

        swr_ctx = swr_alloc();
        if (!swr_ctx)return ALLOC_ERROR;

        AVChannelLayout out_ch_layout;
        out_ch_layout.nb_channels = audio_ctx->ch_layout.nb_channels;
        out_ch_layout.order = audio_ctx->ch_layout.order;
        out_ch_layout.u.mask = ~((~0) << audio_ctx->ch_layout.nb_channels);
        out_ch_layout.opaque = nullptr;
        if (swr_alloc_set_opts2(&swr_ctx, &out_ch_layout, map_palnner_to_packad[*format], audio_ctx->sample_rate, &audio_ctx->ch_layout, *format, audio_ctx->sample_rate, 0, nullptr))return UNKONW_ERROR;
        if (swr_init(swr_ctx))return INIT_ERROR;
        return SUCCESS;
    }

    RESULT PlayTool::sample_planner_to_packed(AVFrame* frame, uint8_t** data, int* linesize)
    {
        *linesize = swr_convert(swr_ctx, data, *linesize, frame->data, frame->nb_samples);
        if (*linesize < 0)return ARGS_ERROR;
        *linesize *= frame->ch_layout.nb_channels * sample_bit_size[frame->format];
        return SUCCESS;
    }

    RESULT PlayTool::init_sws(AVFrame* work, const AVPixelFormat dstFormat, const int dstW, const int dstH)
    {

        if (dstW == 0 || dstH == 0)
            sws_ctx = sws_getContext(work->width, work->height, (AVPixelFormat)work->format, work->width, work->height, dstFormat, SWS_FAST_BILINEAR, nullptr, nullptr, 0);
        else
            sws_ctx = sws_getContext(work->width, work->height, (AVPixelFormat)work->format, dstW, dstH, dstFormat, SWS_FAST_BILINEAR, nullptr, nullptr, 0);

        return SUCCESS;
    }

    inline void PlayTool::insert_queue(AVMediaType index, AutoAVFramePtr&& avf) noexcept
    {
        char* userdata = nullptr;
        if (insert_callback[index] != nullptr)
            insert_callback[index](avf, userdata);

        FrameQueue[index].emplace(std::move(avf), userdata);

        avf = av_frame_alloc();
    }

    RESULT PlayTool::sws_scale_420P(AVFrame*& data)
    {

        AVFrame* dst = av_frame_alloc();

        if(sws_scale_frame(sws_ctx, dst, data)!=0)return RESULT::UNKONW_ERROR;

        dst->pts = data->pts;
        av_frame_unref(data);
        data = dst;
        return SUCCESS;
    }

    void PlayTool::seek_time(int64_t sec) noexcept
    {
        std::lock_guard lock(decode_mutex);

        PacketQueue.lock();
        FrameQueue[AVMEDIA_TYPE_AUDIO].lock();
        FrameQueue[AVMEDIA_TYPE_VIDEO].lock();

        PacketQueue.clear();
        FrameQueue[AVMEDIA_TYPE_AUDIO].clear();
        FrameQueue[AVMEDIA_TYPE_VIDEO].clear();

        avcodec_flush_buffers(decode_ctx[AVMEDIA_TYPE_VIDEO]);
        avcodec_flush_buffers(decode_ctx[AVMEDIA_TYPE_AUDIO]);

        if (avformat_seek_file(avfctx_input, -1, INT64_MIN, sec * AV_TIME_BASE, sec * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) < 0) {
            std::osyncstream{ std::cout } << "seek failed" << std::endl;
        }
        
        this->avframe_work[AVMEDIA_TYPE_VIDEO].first->pts = 0;

        PacketQueue.unlock();
        FrameQueue[AVMEDIA_TYPE_AUDIO].unlock();
        FrameQueue[AVMEDIA_TYPE_VIDEO].unlock();

        PacketQueue._cv_could_push.notify_one();
        FrameQueue[AVMEDIA_TYPE_AUDIO]._cv_could_push.notify_one();
        FrameQueue[AVMEDIA_TYPE_VIDEO]._cv_could_push.notify_one();

    }

    void PlayTool::close() noexcept
    {
        if (ThrRead.joinable()) ThrRead.request_stop();
        if (ThrDecode.joinable()) ThrDecode.request_stop();
        if (ThrPlay.joinable()) ThrPlay.request_stop();

        PacketQueue._is_closed = true;
        FrameQueue[AVMEDIA_TYPE_AUDIO]._is_closed = true;
        FrameQueue[AVMEDIA_TYPE_VIDEO]._is_closed = true;

        PacketQueue._cv_could_push.notify_all();
        PacketQueue._cv_could_pop.notify_all();
        FrameQueue[AVMEDIA_TYPE_AUDIO]._cv_could_push.notify_all();
        FrameQueue[AVMEDIA_TYPE_AUDIO]._cv_could_pop.notify_all();
        FrameQueue[AVMEDIA_TYPE_VIDEO]._cv_could_push.notify_all();
        FrameQueue[AVMEDIA_TYPE_VIDEO]._cv_could_pop.notify_all();

        if (ThrRead.joinable()) ThrRead.join();
        if (ThrDecode.joinable()) ThrDecode.join();
        if (ThrPlay.joinable()) ThrPlay.join();

        PacketQueue._is_closed = false;
        FrameQueue[AVMEDIA_TYPE_AUDIO]._is_closed = false;
        FrameQueue[AVMEDIA_TYPE_VIDEO]._is_closed = false;
    }


    RESULT PlayTool::start_read_thread() noexcept
    {
        ThrRead = std::jthread([this](std::stop_token st)->void
            {
                std::stop_callback end_read_callback(st, [this]()->void {
                    std::osyncstream{ std::cout } << "success exit read thread"  << std::endl;
                });

#if _DEBUG
                //std::once_flag _once_flag;
                ////线程结束时，打印使用RAII
                //std::unique_ptr<void()> a([] {

                //    });
#endif


                int err = AVERROR(EAGAIN);
                while (st.stop_requested() == false)
                {

                    AutoAVPacketPtr avp(av_packet_alloc());
                    {
                        std::unique_lock lock(this->decode_mutex);
                        err = av_read_frame(avfctx_input, avp);
                        if ((err < 0) || (err == AVERROR_EOF)) {
                            std::this_thread::sleep_for(1ms);
                            continue;
                        }
                    }
                    PacketQueue.push(std::move(avp));
                }
            });
        std::osyncstream{ std::cout } << "create read thread id: " << ThrRead.get_id() << std::endl;
        return SUCCESS;
    }

    RESULT PlayTool::start_decode_thread() noexcept
    {
        ThrDecode = std::jthread([&](std::stop_token st)->void
            {
               std::stop_callback end_read_callback(st, [this]()->void {
                   std::osyncstream{ std::cout } << "success exit decode thread" << std::endl;
               });

                int err = AVERROR(EAGAIN);
                AutoAVFramePtr avf = av_frame_alloc();

                while (st.stop_requested() == false)
                {
                    auto _avp_optioal = PacketQueue.pop();

                    if (_avp_optioal.has_value() == false) { 
                        std::this_thread::sleep_for(1ms);
                        continue; 
                    }
                    auto& avp = _avp_optioal.value();

                    if (avp == nullptr) {
                        std::this_thread::sleep_for(1ms);
                        continue;
                    }

                    AVMediaType index = AVStreamIndexToType[avp->stream_index];
                    if (index == AVMEDIA_TYPE_VIDEO || index == AVMEDIA_TYPE_AUDIO)
                    {
                        {
                            std::unique_lock lock(this->decode_mutex);
                            while ((err = avcodec_send_packet(decode_ctx[index], avp)) == AVERROR(EAGAIN))
                                std::this_thread::sleep_for(1ms);
                        }


                        while (st.stop_requested() == false)
                        {
                            err = avcodec_receive_frame(decode_ctx[index], avf);
                            if (err == 0)insert_queue(index,std::move(avf));
                            else if (err == AVERROR(EAGAIN)) break;
                            else if (err == AVERROR_EOF) continue;
                            else return;
                        }
                    }
                    else {}
                }
            });
        std::osyncstream{ std::cout } << "create decode thread id: " << ThrDecode.get_id() << std::endl;
        return SUCCESS;
    }

    RESULT PlayTool::start_encode_thread() noexcept
    {

        return RESULT();
    }

    RESULT PlayTool::start_pull_steam_thread() noexcept
    {

        return RESULT();
    }
}





