#pragma once

#include"BaseFFmpeg.h"
extern"C"
{
#define SDL_MAIN_HANDLED
#include<SDL.h>
#undef main
}
#include<map>

namespace SDLLayer
{
	using namespace FFmpegLayer;
	using namespace std::chrono_literals;

	using AutoWindowPtr = AutoPtr<SDL_Window, Functor<SDL_DestroyWindow>, false>;
	using AutoRendererPtr = AutoPtr<SDL_Renderer, Functor<SDL_DestroyRenderer>, false>;
	using AutoTexturePtr = AutoPtr<SDL_Texture, Functor<SDL_DestroyTexture>, false>;

	//把AVSampleFormat格式转化为SDL_AudioFormat
	extern const SDL_AudioFormat map_audio_formot[13];
	//转化AVPixelFormat为SDL_PixelFormatEnum格式
	extern const std::map<AVPixelFormat, SDL_PixelFormatEnum> map_video_format;

	
	class DriveFullWindow
	{
	public:
		PlayTool* target = nullptr;
		//----------------------------------------------------AUDIO------------------------------------------------------------------
	 private:
		//音频缓存指针
		 Uint8* audio_buf = nullptr;
		//音频工作指向
		Uint8* audio_pos = nullptr;
		//采样到数据大小 =（采样个数) nb_sample * (采样点大小) bit_size
		int audio_buflen = 0;
		//原始帧是否为planner格式
		bool is_planner = false;
		//音频格式
		SDL_AudioSpec sdl_audio{ 0 };
	public:
		//音量大小值
		char volume= static_cast<char>(SDL_MIX_MAXVOLUME);
		//音频识别符
		SDL_AudioDeviceID device_id = 0;
		//-----------------------------------------------------VIDEO------------------------------------------------------------------
	private:
		//视频句柄
		AutoWindowPtr SDL_win;
		//渲染器
		AutoRendererPtr SDL_renderer;
		//纹理
		AutoTexturePtr SDL_texture;
		//窗口信息
		SDL_Rect rect{ 0 };
		//SDL材质选择
		SDL_PixelFormatEnum last_format = SDL_PIXELFORMAT_IYUV;
		//----------------------------------------------------------------------------------------------------------------------------
	public:
		DriveFullWindow(FFmpegLayer::PlayTool& rely);

		~DriveFullWindow();

		//处理画面帧回调封装
		void convert_video_frame(AVFrame*& work, char*& buf) noexcept;
		//处理音頻帧回调封装
		void convert_audio_frame(AVFrame*& work, char*& buf) noexcept;

		//鼠标键盘事件回调函数
		void KeyMouseCallEvent() noexcept;

		//初始化音频播放环境
		void InitAudio(SDL_AudioCallback callback);
		//初始化视频环境
		void InitVideo(const char* title);

		//默认音頻回调函数
		static void SDLCALL default_callback(void* userdata, Uint8* stream, int len);

		//初始化播放环境
		void InitPlayer(const char* WindowName, SDL_AudioCallback callback = default_callback);
		//开始播放
		void StartPlayer() noexcept;
	};
};
