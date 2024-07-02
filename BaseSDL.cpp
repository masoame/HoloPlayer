#include "BaseSDL.h"
#include<iostream>
namespace BaseSDL
{
	using namespace Global_AudioRunning;
	using namespace Global_VideoRunning;

	const SDL_AudioFormat map_audio_formot[13]
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

	BaseFFmpeg* target = nullptr;
    std::jthread Thr_Player;

	namespace Global_AudioRunning
	{
		Uint8* audio_buf = nullptr;
		Uint8* audio_pos = nullptr;
		int audio_buflen = 0;
        bool is_planner = false;
        char volume = SDL_MIX_MAXVOLUME;
	};

	namespace Global_VideoRunning
	{
		AutoWindowPtr SDL_win;
		AutoRendererPtr SDL_renderer;
		AutoTexturePtr SDL_texture;
		SDL_Rect rect{0};
		SDL_PixelFormatEnum last_format = SDL_PIXELFORMAT_IYUV;

        bool is_pause = true;
        std::binary_semaphore wait_show_pause{0};
        std::binary_semaphore run_show_thread{0};
	};

    void InitPlayer(BaseFFmpeg& rely, const char* WindowName, SDL_AudioCallback callback)
	{
        is_pause=true;
		target = &rely;
		InitAudio(callback);
		InitVideo(WindowName);
		target->insert_callback[AVMEDIA_TYPE_VIDEO] = convert_video_frame;
		target->insert_callback[AVMEDIA_TYPE_AUDIO] = convert_audio_frame;
		target->start_read_thread();
		target->start_decode_thread();
	}

    void StartPlayer() noexcept
	{
        close();
        target->local_thread |= BaseFFmpeg::playing_thread;
        Thr_Player = std::jthread([&]()->void
			{
                std::cout << "create player thread id: " << std::this_thread::get_id() << std::endl;


				auto& video_ptr = target->avframe_work[AVMEDIA_TYPE_VIDEO];
				auto& audio_ptr = target->avframe_work[AVMEDIA_TYPE_AUDIO];
				AVFrame*& frame = video_ptr.first;

                while (audio_ptr.first == nullptr) std::this_thread::sleep_for(1ms);
				target->flush_frame(AVMEDIA_TYPE_VIDEO);

                while (target->local_thread & BaseFFmpeg::playing_thread)
				{
					int ret = 0;


					switch (last_format) {
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
							return ;
						}
						break;
					default:
                        if (frame->linesize[0] < 0)
							ret = SDL_UpdateTexture(SDL_texture, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
                        else
							ret = SDL_UpdateTexture(SDL_texture, NULL, frame->data[0], frame->linesize[0]);
						break;
					}
					if (SDL_RenderCopy(SDL_renderer, SDL_texture, NULL, &rect))
						return;

                    SDL_RenderPresent(SDL_renderer);
                    target->flush_frame(AVMEDIA_TYPE_VIDEO);
                    while ((target->local_thread & BaseFFmpeg::playing_thread) && (frame->pts * target->secBaseVideo) >= (audio_ptr.first->pts * target->secBaseAudio))
                    {
                        if(is_pause)
                        {
                            wait_show_pause.release();
                            run_show_thread.acquire();
                        }
                        std::this_thread::sleep_for(1ms);
                    }
				}
                std::cout << "exit player thread id: " << std::this_thread::get_id() << std::endl;
            });
	}

    void convert_video_frame(AVFrame*& work, char*& buf) noexcept
	{
		if (work == nullptr) return;

		auto tempformat = map_video_format.find(static_cast<AVPixelFormat>(work->format));

		if (target->sws_ctx == nullptr && work->format != AV_PIX_FMT_NONE && tempformat == map_video_format.end())
			target->init_sws(work);
		
		if (tempformat == map_video_format.end())
			target->sws_scale_420P(work);

		last_format = map_video_format.find(static_cast<AVPixelFormat>(work->format))->second;
	}

    void convert_audio_frame(AVFrame*& work, char*& buf) noexcept
	{
		if (work == nullptr)return;
		if (buf == nullptr)buf = new char[work->linesize[0]];
		if (is_planner)
			target->sample_planner_to_packed(work, reinterpret_cast<uint8_t**>(&buf), &work->linesize[0]);
	}
	void KeyMouseCallEvent() noexcept
	{
		SDL_Event windowEvent;
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
                        BaseSDL::Global_VideoRunning::is_pause?BaseSDL::run():BaseSDL::stop();
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

    void SDLCALL default_callback(void* userdata, Uint8* stream, int len)noexcept
	{
		auto& audio_frame = target->avframe_work[AVMEDIA_TYPE_AUDIO];
		//清空流
		SDL_memset(stream, 0, len);
		if (audio_buflen == 0)
		{
			if (target->flush_frame(AVMEDIA_TYPE_AUDIO))
			{
				if (is_planner)audio_buf = reinterpret_cast<uint8_t*>(audio_frame.second);
				else audio_buf = audio_frame.first->data[0];

				audio_pos = audio_buf;
				audio_buflen = audio_frame.first->linesize[0];
			}
			else {
				SDL_CloseAudio(); return;
			}
		}

		len = audio_buflen > len ? len : audio_buflen;
        SDL_MixAudio(stream, audio_pos, len, volume);
		audio_pos += len;
		audio_buflen -= len;

		return;
	}


    void InitAudio(SDL_AudioCallback callback)
	{
        SDL_CloseAudio();
		auto& audio_ctx = target->decode_ctx[AVMEDIA_TYPE_AUDIO];
		AVSampleFormat format = *audio_ctx->codec->sample_fmts;

		if (SDL_Init(SDL_INIT_AUDIO))throw "SDL_init error";

		if (av_sample_fmt_is_planar(format))
		{
			if (target->init_swr() != BaseFFmpeg::SUCCESS) throw "init_swr() failed";
			is_planner = true;
		}
        else is_planner = false;
		if (map_audio_formot[format] == -1) throw "audio format is not suport!!!\n";

		SDL_AudioSpec sdl_audio{ 0 };
		sdl_audio.format = map_audio_formot[format];
		sdl_audio.channels = audio_ctx->ch_layout.nb_channels;
		sdl_audio.samples = audio_ctx->frame_size / audio_ctx->ch_layout.nb_channels;
		sdl_audio.silence = 0;
		sdl_audio.freq = audio_ctx->sample_rate;
		sdl_audio.callback = callback;

		if (SDL_OpenAudio(&sdl_audio, nullptr)) throw "SDL_OpenAudio failed!!!\n";
	}

    void InitVideo(const char* title)
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

		SDL_texture = SDL_CreateTexture(SDL_renderer, last_format, SDL_TEXTUREACCESS_STREAMING, video_ctx->width, video_ctx->height);
		if (SDL_texture == nullptr) throw "texture create failed";

		rect.x = 0;
		rect.y = 0;
		rect.w = video_ctx->width;
		rect.h = video_ctx->height;
	}


    void stop() noexcept
    {
        if(is_pause==false)
        {
            SDL_PauseAudio(1);
            is_pause=true;
            wait_show_pause.acquire();
        }
    }

    void run() noexcept
    {
        if(is_pause==true)
        {
            is_pause=false;
            run_show_thread.release();
            SDL_PauseAudio(0);
        }

    }


    extern void close() noexcept
    {
        if(Thr_Player.joinable())
        {
            stop();
            target->local_thread &= ~BaseFFmpeg::playing_thread;
            run();
            Thr_Player.join();
        }
    }

    void Destroy() noexcept
    {
        close();
        SDL_CloseAudio();
        SDL_texture.reset(nullptr);
        SDL_renderer.reset(nullptr);
        SDL_win.reset(nullptr);
        delete target;
    }

}

