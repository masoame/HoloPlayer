#include"BaseFFmpeg.h"
#include<iostream>

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
        this->clear();

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

    RESULT PlayTool::init_encode(AVMediaType type)
    {

        return RESULT();
    }

    RESULT PlayTool::init_swr()
    {
        AutoAVCodecContextPtr& audio_ctx = decode_ctx[AVMEDIA_TYPE_AUDIO];

        AVSampleFormat format = *audio_ctx->codec->sample_fmts;

        if (audio_ctx == nullptr)return ARGS_ERROR;
        if (format == AV_SAMPLE_FMT_NONE || map_palnner_to_packad[format] == AV_SAMPLE_FMT_NONE)return UNNEED_SWR;

        swr_ctx = swr_alloc();
        if (!swr_ctx)return ALLOC_ERROR;

        AVChannelLayout out_ch_layout;
        out_ch_layout.nb_channels = audio_ctx->ch_layout.nb_channels;
        out_ch_layout.order = audio_ctx->ch_layout.order;
        out_ch_layout.u.mask = ~((~0) << audio_ctx->ch_layout.nb_channels);
        out_ch_layout.opaque = nullptr;
        if (swr_alloc_set_opts2(&swr_ctx, &out_ch_layout, map_palnner_to_packad[format], audio_ctx->sample_rate, &audio_ctx->ch_layout, (AVSampleFormat)format, audio_ctx->sample_rate, 0, nullptr))return UNKONW_ERROR;
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

    RESULT PlayTool::sws_scale_420P(AVFrame*& data)
    {

        AVFrame* dst = av_frame_alloc();

        int ret = sws_scale_frame(sws_ctx, dst, data);

        dst->pts = data->pts;
        av_frame_unref(data);
        data = dst;
        return SUCCESS;
    }

    inline void PlayTool::insert_queue(AVMediaType index, AutoAVFramePtr&& avf) noexcept
    {
        char* userdata = nullptr;
        if (insert_callback[index] != nullptr)
            insert_callback[index](avf, userdata);

        while (!FrameQueue[index].try_emplace(std::move(avf),userdata) && (local_thread & decode_thread))
            std::this_thread::sleep_for(1ms);

        avf = av_frame_alloc();
    }

    bool PlayTool::flush_frame(AVMediaType index) noexcept
    {

        std::unique_ptr<framedata_type> temp = nullptr;
        if (!(temp = FrameQueue[index].try_pop())) return false;

        avframe_work[index].first.reset(temp->first.release());
        avframe_work[index].second.reset(temp->second.release());
        return true;
    }

    void PlayTool::seek_time(int64_t sec) noexcept
    {
        stop(read_thread);
        stop(decode_thread);

        PacketQueue.clear();
        FrameQueue[AVMEDIA_TYPE_AUDIO].clear();
        FrameQueue[AVMEDIA_TYPE_VIDEO].clear();

        avcodec_flush_buffers(decode_ctx[AVMEDIA_TYPE_VIDEO]);
        avcodec_flush_buffers(decode_ctx[AVMEDIA_TYPE_AUDIO]);

        if (avformat_seek_file(avfctx_input, -1, INT64_MIN, sec * AV_TIME_BASE, sec * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) < 0)
        {
            if (!ThrRead.joinable()) start_read_thread();
            else run(read_thread);
            if (!ThrDecode.joinable()) start_decode_thread();
            else run(decode_thread);
            return;
        }
        if (!ThrRead.joinable()) start_read_thread();
        else run(read_thread);

        if (!ThrDecode.joinable()) start_decode_thread();
        else run(decode_thread);

        while (!flush_frame(AVMEDIA_TYPE_AUDIO)) std::this_thread::sleep_for(1ms);
    }

    void PlayTool::stop(const char type) noexcept
    {
        if (ThrRead.joinable() && (type & read_thread) && (local_thread & read_thread))
        {
            local_thread &= ~read_thread;
            wait_read_pause.acquire();
        }
        if (ThrRead.joinable() && (type & decode_thread) && (local_thread & decode_thread))
        {
            local_thread &= ~decode_thread;
            wait_decode_pause.acquire();
        }
        if (ThrPlay.joinable() && (type & playing_thread) && (local_thread & playing_thread))
        {
            local_thread &= ~playing_thread;
            wait_play_pause.acquire();
        }
    }
    void PlayTool::run(const char type) noexcept
    {
        if (ThrRead.joinable() && (type & read_thread) && !(local_thread & read_thread))
        {
            local_thread |= read_thread;
            run_read_thread.release();
        }
        if (ThrRead.joinable() && (type & decode_thread) && !(local_thread & decode_thread))
        {
            local_thread |= decode_thread;
            run_decode_thread.release();
        }
        if (ThrPlay.joinable() && (type & playing_thread) && !(local_thread & playing_thread))
        {
            local_thread |= playing_thread;
            run_play_thread.release();
        }
    }

    void PlayTool::clear(const char type) noexcept
    {
        local_thread |= delete_thread;
        if (ThrPlay.joinable() && type & playing_thread)
        {
            local_thread &= ~playing_thread;
            if (ThrPlay.joinable())ThrPlay.join();
        }
        if (ThrDecode.joinable() && type & decode_thread)
        {
            local_thread &= ~decode_thread;
            if (ThrDecode.joinable())ThrDecode.join();
        }
        if (ThrRead.joinable() && type & read_thread)
        {
            local_thread &= ~read_thread;
            if (ThrRead.joinable())ThrRead.join();
        }
        local_thread &= ~delete_thread;
    }

    void PlayTool::clear() noexcept
    {
        clear(read_thread | decode_thread | playing_thread);

        decode_ctx[AVMEDIA_TYPE_AUDIO].reset(nullptr);
        decode_ctx[AVMEDIA_TYPE_VIDEO].reset(nullptr);
        avfctx_input.reset(nullptr);

        PacketQueue.clear();
        FrameQueue[AVMEDIA_TYPE_AUDIO].clear();
        FrameQueue[AVMEDIA_TYPE_VIDEO].clear();

        for (auto& temp : AVStreamIndexToType)
        {
            temp = AVMEDIA_TYPE_UNKNOWN;
        }
        for (auto& temp : AVTypeToStreamIndex)
        {
            temp = -1;
        }
        local_thread &= ~delete_thread;
    }

    RESULT PlayTool::start_read_thread() noexcept
    {
        local_thread |= read_thread;
        ThrRead = std::jthread([this]()->void
            {
                std::cout << "create read thread id: " << std::this_thread::get_id() << std::endl;
                std::unique_ptr < PlayTool, decltype([](void* _this) ->void
                    {
                        auto __this = static_cast<PlayTool*>(_this);
                        __this->local_thread &= ~read_thread;
                        __this->PacketQueue.try_push(nullptr);
                        std::cout << "exit read thread id: " << std::this_thread::get_id() << std::endl;
                    }) > end(this);

                AutoAVPacketPtr avp;
                int err = AVERROR(EAGAIN);
                while (true)
                {
                    if (!(local_thread & read_thread))
                    {
                        if (local_thread & delete_thread) return;
                        wait_read_pause.release();
                        run_read_thread.acquire();
                    }
                    avp.reset(av_packet_alloc());
                    err = av_read_frame(avfctx_input, avp);
                    if ((err < 0) || (err == AVERROR_EOF))
                    {
                        std::this_thread::sleep_for(1ms);
                        continue;
                    }
                    while (!PacketQueue.try_push(std::move(avp)))
                    {
                        if (!(local_thread & read_thread)) break;
                        else std::this_thread::sleep_for(1ms);
                    }
                }
            });
        return SUCCESS;
    }

    RESULT PlayTool::start_decode_thread() noexcept
    {
        local_thread |= decode_thread;
        ThrDecode = std::jthread([&]()->void
            {
                std::cout << "create decode thread id: " << std::this_thread::get_id() << std::endl;
                std::unique_ptr <PlayTool, decltype([](void* _this) ->void
                    {
                        auto __this = static_cast<PlayTool*>(_this);
                        __this->local_thread &= ~decode_thread;
                        __this->insert_queue(AVMEDIA_TYPE_VIDEO, nullptr);
                        std::cout << "exit decode thread id: " << std::this_thread::get_id() << std::endl;
                    }) > end(this);

                int err = AVERROR(EAGAIN);
                AutoAVFramePtr avf = av_frame_alloc();
                AutoAVPacketPtr avp;

                while (true)
                {
                    if (!(local_thread & decode_thread))
                    {
                        if (local_thread & delete_thread) return;
                        wait_decode_pause.release();
                        run_decode_thread.acquire();
                    }

                    while ((avp = PacketQueue.try_pop()) == nullptr)
                        if (local_thread & decode_thread) std::this_thread::sleep_for(1ms);
                        else break;

                    if (avp == nullptr)continue;

                    if (avp.get() == nullptr) return;

                    AVMediaType index = AVStreamIndexToType[avp->stream_index];
                    if (index == AVMEDIA_TYPE_VIDEO || index == AVMEDIA_TYPE_AUDIO)
                    {
                        while ((err = avcodec_send_packet(decode_ctx[index], avp)) == AVERROR(EAGAIN))
                            if (local_thread & decode_thread) std::this_thread::sleep_for(1ms);
                            else continue;

                        while (true)
                        {
                            err = avcodec_receive_frame(decode_ctx[index], avf);
                            if (err == 0) insert_queue(index, std::move(avf));
                            else if (err == AVERROR(EAGAIN)) break;
                            else if (err == AVERROR_EOF) return;
                            else return;
                        }
                    }
                    else {}
                }
            });
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





