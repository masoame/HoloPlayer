#include "BaseSDL.h"
#include<iostream>
#include<syncstream>
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

	DriveWindow::DriveWindow(FFmpegLayer::PlayTool* rely)
	{
		play_tool.reset(rely);
	}

	DriveWindow::~DriveWindow()
	{

		SDL_CloseAudioDevice(device_id);
		//if (play_tool != nullptr) play_tool->clear();
		play_tool = nullptr;
		SDL_texture.reset(nullptr);
		SDL_renderer.reset(nullptr);
		SDL_win.reset(nullptr);

	}

	void DriveWindow::InitPlayer(int width, int height, void* win_handle, SDL_AudioCallback callback)
	{
		if (play_tool == nullptr)return;
		InitAudio(callback);
		InitVideo(win_handle, width, height);
		play_tool->insert_callback[AVMEDIA_TYPE_VIDEO] = std::bind(&DriveWindow::convert_video_frame, this, std::placeholders::_1, std::placeholders::_2);
		play_tool->insert_callback[AVMEDIA_TYPE_AUDIO] = std::bind(&DriveWindow::convert_audio_frame, this, std::placeholders::_1, std::placeholders::_2);
		play_tool->start_read_thread();
		play_tool->start_decode_thread();
	}

    void DriveWindow::StartPlayer() noexcept
	{
		if (play_tool == nullptr) return;
		SDL_PauseAudioDevice(device_id, 0);
		play_tool->ThrPlay = std::jthread([&](std::stop_token st)->void
			{
				//退出执行
				std::stop_callback scb(st, [this]()
					{
						std::osyncstream{ std::cout } << "exit player thread" << std::endl;
					});

				//工作元素引用
				auto& work_data = play_tool->avframe_work[AVMEDIA_TYPE_VIDEO];
				auto& frame = play_tool->avframe_work[AVMEDIA_TYPE_VIDEO].first;
				auto& audio_ptr = play_tool->avframe_work[AVMEDIA_TYPE_AUDIO];

				//时间基准
				auto& secBaseVideo = play_tool->secBaseTime[AVMEDIA_TYPE_VIDEO];
				auto& secBaseAudio = play_tool->secBaseTime[AVMEDIA_TYPE_AUDIO];

				//画面帧队列
				auto& video_queue = play_tool->FrameQueue[AVMEDIA_TYPE_VIDEO];
				auto& audio_queue = play_tool->FrameQueue[AVMEDIA_TYPE_AUDIO];

				//如果音频队列为空，则等待音频队列填充
                while (audio_ptr.first == nullptr) std::this_thread::sleep_for(1ms);

				//循环读取帧刷新到窗口句柄上
                while (st.stop_requested() == false)
				{
					//读取视频帧
					auto frame_data = video_queue.pop();
					if (frame_data.has_value() == false) { 
						std::this_thread::sleep_for(1ms); 
						continue; 
					}
					//放到工作队列
					work_data = std::move(frame_data.value());
					if (frame == nullptr) continue;

					//如果画面尺寸改变，则重新创建渲染器和纹理
					if (isChangeSize == true || SDL_renderer == nullptr || SDL_texture == nullptr)
					{
						SDL_renderer.reset();
						SDL_renderer = SDL_CreateRenderer(SDL_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
						SDL_texture.reset();
						SDL_texture = SDL_CreateTexture(SDL_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
						if(SDL_texture==nullptr)std::cout << SDL_GetError() << std::endl;
						isChangeSize = false;
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

					SDL_RenderPresent(SDL_renderer);

					//如果视频时间在音频时间之前，则等待音频时间播放完毕
					for (double sec = (frame->pts * secBaseVideo) - (audio_ptr.first->pts * secBaseAudio); sec >= 0 && st.stop_requested() == false;)
                    {
						if (video_queue.full() && audio_queue.empty()) break;
						std::this_thread::sleep_for(1ms);
						sec = (frame->pts * secBaseVideo) - (audio_ptr.first->pts * secBaseAudio);
                    }
				}
            });
		std::osyncstream{ std::cout } << "create player thread id: " << play_tool->ThrPlay.get_id() << std::endl;
	}

	void DriveWindow::ReSize(int width, int height) noexcept
	{
		int scr_width = width;
		int scr_height = height;
		int _width, _height, x, y;

		_height = scr_height;
		_width = lrint(height * aspect_ratio) & ~1;
		if (_width > scr_width)
		{
			_width = scr_width;
			_height = lrint(_width / aspect_ratio) & ~1;
		}
		x = (scr_width - _width) / 2;
		y = (scr_height - _height) / 2;

		rect.x = x;
		rect.y = y;
		rect.w = _width;
		rect.h = _height;
		isChangeSize = true;
	}

	void DriveWindow::PausePlayer() noexcept
	{
		if (play_tool == nullptr) return;
		SDL_PauseAudioDevice(device_id, 1);
		this->is_pause = true;
	}

	void DriveWindow::ResumePlayer() noexcept
	{
		if (play_tool == nullptr) return;
		SDL_PauseAudioDevice(device_id, 0);
		this->is_pause = false;
	}

	void DriveWindow::togglePause() noexcept
	{
		is_pause ? ResumePlayer() : PausePlayer();
	}

	void DriveWindow::convert_video_frame(AVFrame*& work, [[maybe_unused]] char*& buf) noexcept
	{
		if (work == nullptr) return;

		if (SDL_texture == nullptr)
		{
			auto format = map_video_format.find(static_cast<AVPixelFormat>(work->format));
			if (format == map_video_format.end() || format->first == AV_PIX_FMT_NONE)
			{
				isNeedToChangeFrame = true;
				play_tool->init_sws(work);
				pixel_format = SDL_PIXELFORMAT_IYUV;
			}
			else
			{
				pixel_format = format->second;
				isNeedToChangeFrame = false;
			}
		}
		if (isNeedToChangeFrame) play_tool->sws_scale_420P(work);

	}

    void DriveWindow::convert_audio_frame(AVFrame*& work, char*& buf) noexcept
	{
		if (work == nullptr)return;
		if (buf == nullptr)buf = new char[work->linesize[0]];
		if (is_planner)
			play_tool->sample_planner_to_packed(work, reinterpret_cast<uint8_t**>(&buf), &work->linesize[0]);
	}

	void DriveWindow::KeyMouseCallEvent() noexcept
	{
		SDL_Event windowEvent;

		//auto secBaseVideo = play_tool->secBaseTime[AVMEDIA_TYPE_VIDEO];
		auto secBaseAudio = play_tool->secBaseTime[AVMEDIA_TYPE_AUDIO];

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
						if (play_tool == nullptr) break;
						//if (play_tool->local_thread & playing_thread){
						//	SDL_PauseAudioDevice(device_id, 1);
						//	//play_tool->stop(playing_thread);
						//}
						else{
							SDL_PauseAudioDevice(device_id, 0);
							//play_tool->run(playing_thread);
						}
                        break;

					case SDLK_LEFT:

						play_tool->seek_time(static_cast<int64_t>(play_tool->avframe_work[AVMEDIA_TYPE_AUDIO].first->pts * secBaseAudio - 5));

						break;
					case SDLK_RIGHT:

						play_tool->seek_time(static_cast<int64_t>(play_tool->avframe_work[AVMEDIA_TYPE_AUDIO].first->pts * secBaseAudio + 5));

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

    void SDLCALL DriveWindow::default_callback(void* userdata, Uint8* stream, int len)
	{
		auto _this = static_cast<DriveWindow*>(userdata);
		auto& audio_frame = _this->play_tool->avframe_work[AVMEDIA_TYPE_AUDIO];

		//清空流
		if (_this->audio_buflen == 0)
		{
			auto tmp_audio_frame = _this->play_tool->FrameQueue[AVMEDIA_TYPE_AUDIO].pop();
			if(tmp_audio_frame.has_value() == false) return;

			audio_frame = std::move(tmp_audio_frame.value());

			if (_this->is_planner)
				_this->audio_buffer = reinterpret_cast<uint8_t*>(audio_frame.second.get());
            else 
				_this->audio_buffer = audio_frame.first->data[0];

			_this->audio_pos = _this->audio_buffer;
			_this->audio_buflen = audio_frame.first->linesize[0];
		}

		len = _this->audio_buflen > len ? len : _this->audio_buflen;
		SDL_memset(stream, 0, len);
		SDL_MixAudioFormat(stream, _this->audio_pos, _this->sdl_audio.format, len, _this->volume);
		_this->audio_pos += len;
		_this->audio_buflen -= len;

		return;
	}

    void DriveWindow::InitAudio(SDL_AudioCallback callback)
	{
		SDL_CloseAudioDevice(device_id);
		auto& audio_ctx = play_tool->decode_ctx[AVMEDIA_TYPE_AUDIO];
		AVSampleFormat* format;

		avcodec_get_supported_config(audio_ctx, nullptr, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, (const void**)&format, nullptr);

		if (SDL_Init(SDL_INIT_AUDIO))throw "SDL_init error";

		if (av_sample_fmt_is_planar(*format))
		{
			if (play_tool->init_swr() != FFmpegLayer::SUCCESS) throw "init_swr() failed";
			is_planner = true;
		}
        else is_planner = false;
		if (map_audio_formot[*format] == -1) throw "audio format is not suport!!!\n";

		memset(&sdl_audio, 0, sizeof(sdl_audio));

		sdl_audio.format = map_audio_formot[*format];
		sdl_audio.channels = static_cast<Uint8>(audio_ctx->ch_layout.nb_channels);
		sdl_audio.samples = static_cast<Uint16>(audio_ctx->frame_size / audio_ctx->ch_layout.nb_channels);
		sdl_audio.silence = 0;
		sdl_audio.freq = audio_ctx->sample_rate;
		sdl_audio.userdata = this;
		sdl_audio.callback = callback;

		device_id = SDL_OpenAudioDevice(nullptr, 0, &sdl_audio, nullptr, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

		if (device_id == 0) throw SDL_GetError();
	}

    void DriveWindow::InitVideo(void* win_handle,int width,int height)
	{
        SDL_texture.reset(nullptr);
        SDL_renderer.reset(nullptr);
        SDL_win.reset(nullptr);

		//抗锯齿
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

		auto& video_ctx = play_tool->decode_ctx[AVMEDIA_TYPE_VIDEO];

		if (SDL_Init(SDL_INIT_VIDEO)) throw "SDL_init error";

		SDL_win.reset();

		if(win_handle)
			SDL_win =SDL_CreateWindowFrom(win_handle);
		else
			SDL_win = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, video_ctx->width, video_ctx->height, SDL_WINDOW_SHOWN);

		if (SDL_win == nullptr) throw "windows create error";

		aspect_ratio = static_cast<float>(video_ctx->width) / video_ctx->height;
		ReSize(width, height);
	}
}

