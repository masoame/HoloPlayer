#include "BaseSDL.h"
#include<iostream>
namespace SDLLayer
{
	const constexpr SDL_AudioFormat map_audio_formot[13]
	{
		AUDIO_U8,
		AUDIO_S16SYS,
		AUDIO_S32SYS,
		AUDIO_F32SYS,
		static_cast<SDL_AudioFormat>(-1),
		AUDIO_U8,
		AUDIO_S16SYS,
		AUDIO_S32SYS,
		AUDIO_F32SYS,
		1,
		static_cast<SDL_AudioFormat>(-1),
		static_cast<SDL_AudioFormat>(-1),
		static_cast<SDL_AudioFormat>(-1)
	};

	const std::map<AVPixelFormat, SDL_PixelFormatEnum> map_video_format
	{
		{ AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
		{ AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
		{ AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
		{ AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
		{ AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
		{ AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
		{ AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
		{ AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
		{ AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
		{ AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
		{ AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
		{ AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
		{ AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
		{ AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
		{ AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
		{ AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
		{ AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
		{ AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
		{ AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
		{ AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN }
	};

	DriveFullWindow::DriveFullWindow(FFmpegLayer::PlayTool& rely)
	{
		target = &rely;
	}

	DriveFullWindow::~DriveFullWindow()
	{
		SDL_CloseAudioDevice(device_id);
		if (target != nullptr) target->clear();
		SDL_texture.reset(nullptr);
		SDL_renderer.reset(nullptr);
		SDL_win.reset(nullptr);
		target = nullptr;
	}

    void DriveFullWindow::InitPlayer(const char* WindowName, SDL_AudioCallback callback)
	{
		if (target == nullptr)return;
		InitAudio(callback);
		InitVideo(WindowName);
		target->insert_callback[AVMEDIA_TYPE_VIDEO] = std::bind(&DriveFullWindow::convert_video_frame, this, std::placeholders::_1, std::placeholders::_2);
		target->insert_callback[AVMEDIA_TYPE_AUDIO] = std::bind(&DriveFullWindow::convert_audio_frame, this, std::placeholders::_1, std::placeholders::_2);
		target->start_read_thread();
		target->start_decode_thread();
	}

    void DriveFullWindow::StartPlayer() noexcept
	{
		if (target == nullptr) return;
		target->clear(playing_thread);
		SDL_PauseAudioDevice(device_id, 0);
        target->local_thread |= playing_thread;
        target->ThrPlay = std::jthread([&]()->void
			{
                std::cout << "create player thread id: " << std::this_thread::get_id() << std::endl;
				std::unique_ptr < PlayTool, decltype([](void* _this)
					{
						auto __this = static_cast<PlayTool*>(_this);
						__this->local_thread &= ~FFmpegLayer::playing_thread;
						std::cout << "exit player thread id: " << std::this_thread::get_id() << std::endl;
					}) > end(target);

				auto& frame = target->avframe_work[AVMEDIA_TYPE_VIDEO].first;
				auto& audio_ptr = target->avframe_work[AVMEDIA_TYPE_AUDIO];
				auto& secBaseVideo = target->secBaseTime[AVMEDIA_TYPE_VIDEO];
				auto& secBaseAudio = target->secBaseTime[AVMEDIA_TYPE_AUDIO];

                while (audio_ptr.first == nullptr) std::this_thread::sleep_for(1ms);

                while (true)
				{
                    while (!target->flush_frame(AVMEDIA_TYPE_VIDEO))
                        if (target->local_thread & FFmpegLayer::playing_thread) std::this_thread::sleep_for(1ms);
                        else break;

					if (frame == nullptr) return;

                    if(!(target->local_thread & FFmpegLayer::playing_thread))
                    {
						if (target->local_thread & FFmpegLayer::delete_thread) return;
                        target->wait_play_pause.release();
						target->run_play_thread.acquire();
                    }
					int ret = 0;
					switch (pixel_format) {
					case SDL_PIXELFORMAT_IYUV:
						if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0) {
							ret = SDL_UpdateYUVTexture(SDL_texture, NULL, frame->data[0], frame->linesize[0],
								frame->data[1], frame->linesize[1],
								frame->data[2], frame->linesize[2]);
						}
						else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0) {
							ret = SDL_UpdateYUVTexture(SDL_texture, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0],
								frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[1],
								frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[2]);
						}
						else {
							av_log(NULL, AV_LOG_ERROR, "Mixed negative and positive linesizes are not supported.\n");
							return;
						}
						break;
					default:
                        if (frame->linesize[0] < 0)
							ret = SDL_UpdateTexture(SDL_texture, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
                        else
							ret = SDL_UpdateTexture(SDL_texture, NULL, frame->data[0], frame->linesize[0]);
						break;
					}

					if (ret == -1)std::cout << SDL_GetError() << std::endl;

					if (SDL_RenderCopy(SDL_renderer, SDL_texture, NULL, &rect)) return;

                    while ((frame->pts * secBaseVideo) >= (audio_ptr.first->pts * secBaseAudio))
                    {
                        if(target->FrameQueue[AVMEDIA_TYPE_VIDEO]->full() && target->FrameQueue[AVMEDIA_TYPE_AUDIO]->empty())
                            target->flush_frame(AVMEDIA_TYPE_VIDEO);
                        if(target->local_thread & FFmpegLayer::playing_thread) std::this_thread::sleep_for(1ms);
                        else break;
                    }
                    SDL_RenderPresent(SDL_renderer);
				}
            });
	}

	void DriveFullWindow::convert_video_frame(AVFrame*& work, char*& buf) noexcept
	{
		if (work == nullptr) return;

		if (SDL_texture == nullptr)
		{
			auto format = map_video_format.find(static_cast<AVPixelFormat>(work->format));
			if (format == map_video_format.end() || format->first == AV_PIX_FMT_NONE)
			{
				isNeedToChangeFrame = true;
				target->init_sws(work);
				pixel_format = SDL_PIXELFORMAT_IYUV;
			}
			else
			{
				pixel_format = format->second;
				isNeedToChangeFrame = false;
			}
			SDL_texture = SDL_CreateTexture(SDL_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, work->width, work->height);
		}
		if (isNeedToChangeFrame) target->sws_scale_420P(work);

	}

    void DriveFullWindow::convert_audio_frame(AVFrame*& work, char*& buf) noexcept
	{
		if (work == nullptr)return;
		if (buf == nullptr)buf = new char[work->linesize[0]];
		if (is_planner)
			target->sample_planner_to_packed(work, reinterpret_cast<uint8_t**>(&buf), &work->linesize[0]);
	}

	void DriveFullWindow::KeyMouseCallEvent() noexcept
	{
		SDL_Event windowEvent;

		auto& secBaseVideo = target->secBaseTime[AVMEDIA_TYPE_VIDEO];
		auto& secBaseAudio = target->secBaseTime[AVMEDIA_TYPE_AUDIO];

        while (true)
		{

			if (SDL_PollEvent(&windowEvent)) {

				switch (windowEvent.key.type)
				{
				case SDL_KEYDOWN:

					switch (windowEvent.key.keysym.sym)
					{
					case SDLK_ESCAPE:
						break;
					case SDLK_SPACE:
						if (target == nullptr) break;
						if (target->local_thread & playing_thread)
						{
							SDL_PauseAudioDevice(device_id, 1);
							target->stop(playing_thread);
						}
						else
						{
							SDL_PauseAudioDevice(device_id, 0);
							target->run(playing_thread);
						}
                        break;

					case SDLK_LEFT:

						target->seek_time(target->avframe_work[AVMEDIA_TYPE_AUDIO].first->pts * secBaseAudio - 5);

						break;
					case SDLK_RIGHT:

						target->seek_time(target->avframe_work[AVMEDIA_TYPE_AUDIO].first->pts * secBaseAudio + 5);

						break;

					default:
						break;
					}
					break;
				default:
					break;
				}
			}

            std::this_thread::sleep_for(2ms);
		}
	}

    void SDLCALL DriveFullWindow::default_callback(void* userdata, Uint8* stream, int len)
	{
		auto _this = static_cast<DriveFullWindow*>(userdata);
		auto& audio_frame = _this->target->avframe_work[AVMEDIA_TYPE_AUDIO];

		//清空流
		if (_this->audio_buflen == 0)
		{
			if (!_this->target->flush_frame(AVMEDIA_TYPE_AUDIO)) return;

            if(!(_this->target->local_thread & playing_thread)) return;

			if (_this->is_planner)
				_this->audio_buf = reinterpret_cast<uint8_t*>(audio_frame.second);
            else 
				_this->audio_buf = audio_frame.first->data[0];
			_this->audio_pos = _this->audio_buf;
			_this->audio_buflen = audio_frame.first->linesize[0];

		}

		len = _this->audio_buflen > len ? len : _this->audio_buflen;
		SDL_memset(stream, 0, len);
		SDL_MixAudioFormat(stream, _this->audio_pos, _this->sdl_audio.format, len, _this->volume);
		_this->audio_pos += len;
		_this->audio_buflen -= len;

		return;
	}

    void DriveFullWindow::InitAudio(SDL_AudioCallback callback)
	{
		SDL_CloseAudioDevice(device_id);
		auto& audio_ctx = target->decode_ctx[AVMEDIA_TYPE_AUDIO];
		AVSampleFormat format = *audio_ctx->codec->sample_fmts;

		if (SDL_Init(SDL_INIT_AUDIO))throw "SDL_init error";

		if (av_sample_fmt_is_planar(format))
		{
			if (target->init_swr() != FFmpegLayer::SUCCESS) throw "init_swr() failed";
			is_planner = true;
		}
        else is_planner = false;
		if (map_audio_formot[format] == -1) throw "audio format is not suport!!!\n";

		memset(&sdl_audio, 0, sizeof(sdl_audio));

		sdl_audio.format = map_audio_formot[format];
		sdl_audio.channels = audio_ctx->ch_layout.nb_channels;
		sdl_audio.samples = audio_ctx->frame_size / audio_ctx->ch_layout.nb_channels;
		sdl_audio.silence = 0;
		sdl_audio.freq = audio_ctx->sample_rate;
		sdl_audio.userdata = this;
		sdl_audio.callback = callback;

		device_id = SDL_OpenAudioDevice(nullptr, 0, &sdl_audio, nullptr, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

		if (device_id == 0) throw SDL_GetError();

	}

    void DriveFullWindow::InitVideo(const char* title)
	{
        SDL_texture.reset(nullptr);
        SDL_renderer.reset(nullptr);
        SDL_win.reset(nullptr);

		auto& video_ctx = target->decode_ctx[AVMEDIA_TYPE_VIDEO];

		if (SDL_Init(SDL_INIT_VIDEO)) throw "SDL_init error";

		SDL_win = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, video_ctx->width, video_ctx->height, SDL_WINDOW_SHOWN);
		if (SDL_win == nullptr) throw "windows create error";

		SDL_renderer = SDL_CreateRenderer(SDL_win, -1, SDL_RENDERER_PRESENTVSYNC);
		if (SDL_renderer == nullptr)throw "Renderer create failed";

		rect.w = target->decode_ctx[AVMEDIA_TYPE_VIDEO]->width;
		rect.h = target->decode_ctx[AVMEDIA_TYPE_VIDEO]->height;
	}
}

