#include"BaseFFmpeg.h"

#include<iostream>

using namespace std::chrono_literals;

BaseFFmpeg::RESULT BaseFFmpeg::open(const char* url, byte type)
{
    this->clear();

	switch (type)
	{
	case in|video:
		init_decode(url);
		break;
	case in|audio:

		break;
	case out|video:

		break;
	case out|audio:

		break;
	default:
		break;
	}
	return SUCCESS;
}

BaseFFmpeg::RESULT BaseFFmpeg::init_decode(const char* url, const AVInputFormat* fmt, AVDictionary** options)
{
	avfctx_input = avformat_alloc_context();

	if (avformat_open_input(&avfctx_input, url, fmt, options)) return OPEN_ERROR;
	if (avformat_find_stream_info(avfctx_input, nullptr) < 0) return ARGS_ERROR;
	av_dump_format(avfctx_input, 0, url, false);

	int index = av_find_best_stream(avfctx_input, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (index != AVERROR_STREAM_NOT_FOUND) { AVStreamIndexToType[index] = AVMEDIA_TYPE_VIDEO; AVTypeToStreamIndex[AVMEDIA_TYPE_VIDEO] = index; }
	index = av_find_best_stream(avfctx_input, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (index != AVERROR_STREAM_NOT_FOUND) { AVStreamIndexToType[index] = AVMEDIA_TYPE_AUDIO; AVTypeToStreamIndex[AVMEDIA_TYPE_AUDIO] = index; }

	decode_ctx[AVMEDIA_TYPE_VIDEO] = avcodec_alloc_context3(nullptr);
	decode_ctx[AVMEDIA_TYPE_AUDIO] = avcodec_alloc_context3(nullptr);

	if (!decode_ctx[AVMEDIA_TYPE_VIDEO] || !decode_ctx[AVMEDIA_TYPE_AUDIO]) return ALLOC_ERROR;

	for (int i = 0; i != 6; i++)
	{
		if (AVStreamIndexToType[i] == AVMEDIA_TYPE_UNKNOWN)continue;
		if (avcodec_parameters_to_context(decode_ctx[AVStreamIndexToType[i]], avfctx_input->streams[i]->codecpar) < 0)return ARGS_ERROR;
		const AVCodec* codec = avcodec_find_decoder(decode_ctx[AVStreamIndexToType[i]]->codec_id);
		if (avcodec_open2(decode_ctx[AVStreamIndexToType[i]], codec, NULL))return OPEN_ERROR;
	}

	secBaseVideo = av_q2d(avfctx_input->streams[AVTypeToStreamIndex[AVMEDIA_TYPE_VIDEO]]->time_base);
	secBaseAudio = av_q2d(avfctx_input->streams[AVTypeToStreamIndex[AVMEDIA_TYPE_AUDIO]]->time_base);

	return SUCCESS;
}

BaseFFmpeg::RESULT BaseFFmpeg::init_encode(const char* url, const AVOutputFormat* fmt)
{
	//if(avformat_alloc_output_context2(&avfctx_output, fmt, url, nullptr)<0);

	//if (avio_open(&pFormatCtx->pb, outputPath, AVIO_FLAG_READ_WRITE) < 0) 
		//LOGE("Failed to open output file! \n");
		//return false;
	//}

	return RESULT();
}

BaseFFmpeg::RESULT BaseFFmpeg::init_swr()
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

BaseFFmpeg::RESULT BaseFFmpeg::sample_planner_to_packed(AVFrame* frame, uint8_t** data, int* linesize)
{
	*linesize = swr_convert(swr_ctx, data, *linesize, frame->data, frame->nb_samples);
	if (*linesize < 0)return ARGS_ERROR;
	*linesize *= frame->ch_layout.nb_channels * sample_bit_size[frame->format];
	return SUCCESS;
}

BaseFFmpeg::RESULT BaseFFmpeg::init_sws(AVFrame* work, const AVPixelFormat dstFormat, const int dstW, const int dstH)
{

	if (dstW == 0 || dstH == 0)
		sws_ctx = sws_getContext(work->width, work->height, (AVPixelFormat)work->format, work->width, work->height, dstFormat, SWS_FAST_BILINEAR, nullptr, nullptr, 0);
	else
		sws_ctx = sws_getContext(work->width, work->height, (AVPixelFormat)work->format, dstW, dstH, dstFormat, SWS_FAST_BILINEAR, nullptr, nullptr, 0);

	return SUCCESS;
}

BaseFFmpeg::RESULT BaseFFmpeg::sws_scale_420P(AVFrame*& data)
{
	AutoAVFramePtr& frame = avframe_work[AVMEDIA_TYPE_VIDEO].first;

	AVFrame* dst = av_frame_alloc();
	sws_scale_frame(sws_ctx, dst, data);

	dst->pts = data->pts;
	av_frame_unref(data);
	data = dst;
	return SUCCESS;
}

inline void BaseFFmpeg::insert_queue(AVMediaType index, AutoAVFramePtr&& avf) noexcept
{
	framedata_type* temp = nullptr;
    while (!(temp = FrameQueue[index]->rear()) && (local_thread & decode_thread))
		std::this_thread::sleep_for(1ms);
    if(temp==nullptr) return ;
	char* userdata = temp->second.release();
	if (insert_callback[index] != nullptr) 
		insert_callback[index](avf, userdata);
    while (!FrameQueue[index]->push({ avf.release(),std::unique_ptr<char[]>(userdata) }) && (local_thread & decode_thread))
		std::this_thread::sleep_for(1ms); 

	avf = av_frame_alloc();
}

bool BaseFFmpeg::flush_frame(AVMediaType index) noexcept
{

	framedata_type* temp = nullptr;
	while (!(temp = FrameQueue[index]->pop()))
		std::this_thread::sleep_for(1ms);

	avframe_work[index].first.reset(temp->first.release());
	avframe_work[index].second = temp->second.get();
	if (avframe_work[index].first == nullptr)return false;
	return true;
}

void BaseFFmpeg::seek_time(int64_t sec) noexcept
{
    stop(decode_thread);
    FrameQueue[AVMEDIA_TYPE_AUDIO]->clear();
    FrameQueue[AVMEDIA_TYPE_VIDEO]->clear();
    stop(read_thread);
    PacketQueue.clear();

    if (avformat_seek_file(avfctx_input, -1,  INT64_MIN, sec * AV_TIME_BASE, sec * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) < 0)
    {
        run(read_thread);
        run(decode_thread);
        return;
    }

    avcodec_flush_buffers(decode_ctx[AVMEDIA_TYPE_VIDEO]);
    //avcodec_flush_buffers(decode_ctx[AVMEDIA_TYPE_AUDIO]);

    run(read_thread);
    run(decode_thread);

    flush_frame(AVMEDIA_TYPE_VIDEO);
    flush_frame(AVMEDIA_TYPE_AUDIO);
}

void BaseFFmpeg::stop(char type) noexcept
{
    if(type & read_thread)
    {
        local_thread &= ~read_thread;
        wait_read_pause.acquire();
    }
    if(type & decode_thread)
    {
        local_thread &= ~decode_thread;
        wait_decode_pause.acquire();
    }

}
void BaseFFmpeg::run(char type) noexcept
{
    if(type & read_thread)
    {
        local_thread |= read_thread;
        run_read_thread.release();
    }
    if(type & decode_thread)
    {
        local_thread |= decode_thread;
;       run_decode_thread.release();
    }
}

void BaseFFmpeg::clear() noexcept
{
    local_thread |= delete_thread;
    local_thread &= ~read_thread;
    local_thread &= ~decode_thread;

    if(ThrRead.joinable())ThrRead.join();
    if(ThrDecode.joinable())ThrDecode.join();

    decode_ctx[AVMEDIA_TYPE_AUDIO].reset(nullptr);
    decode_ctx[AVMEDIA_TYPE_VIDEO].reset(nullptr);
    avfctx_input.reset(nullptr);
    local_thread &= ~delete_thread;
}

BaseFFmpeg::RESULT BaseFFmpeg::start_read_thread() noexcept
{
    local_thread = read_thread | decode_thread;
    ThrRead = std::jthread([&]()->void
		{
			AutoAVPacketPtr avp;
            int err = AVERROR(EAGAIN);
            while (err != AVERROR_EOF)
			{
				avp.reset(av_packet_alloc()); 
				err = av_read_frame(avfctx_input, avp);
                if (err < 0)
                {
					avp.reset(nullptr);
                    continue;
                }
                while(!PacketQueue.push(std::move(avp)))
                {
                    std::this_thread::sleep_for(1ms);
                    if (!(local_thread & read_thread))
                    {
                        if(local_thread & delete_thread) return;
                        wait_read_pause.release();
                        run_read_thread.acquire();
                        break;
                    }
                }
			}
    });
    return SUCCESS;
}

BaseFFmpeg::RESULT BaseFFmpeg::start_decode_thread() noexcept
{
    ThrDecode = std::jthread([&]()->void
		{
			int err = AVERROR(EAGAIN);
			AutoAVFramePtr avf = av_frame_alloc();
            while (true)
			{
                if (!(local_thread & decode_thread))
                {
                    if(local_thread & delete_thread) return;
                    wait_decode_pause.release();
                    run_decode_thread.acquire();
                }
				AutoAVPacketPtr* avp;
				while ((avp = PacketQueue.front()) == nullptr) std::this_thread::sleep_for(1ms);

				AVMediaType index = AVStreamIndexToType[(*avp)->stream_index];
				if (index == AVMEDIA_TYPE_VIDEO || index == AVMEDIA_TYPE_AUDIO)
				{
                    while ((err = avcodec_send_packet(decode_ctx[index], *avp)) == AVERROR(EAGAIN))
                        std::this_thread::sleep_for(1ms);
					PacketQueue.pop();
                    while (true)
					{
						err = avcodec_receive_frame(decode_ctx[index], avf);
                        if (err == 0) insert_queue(index, std::move(avf));
						else if (err == AVERROR(EAGAIN)) break;
                        else if (err == AVERROR_EOF)
                        {
                            insert_queue(index, nullptr);
                            return;
                        }
						else return;
					}
				}
				else { PacketQueue.pop(); }
			}
        });
		return SUCCESS;
}

BaseFFmpeg::RESULT BaseFFmpeg::start_encode_thread() noexcept
{

	return RESULT();
}

BaseFFmpeg::RESULT BaseFFmpeg::start_pull_steam_thread() noexcept
{

	return RESULT();
}
