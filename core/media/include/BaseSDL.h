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

	using AutoWindowPtr = ::common::AutoHandle<SDL_Window, ::common::Functor<SDL_DestroyWindow>>;
	using AutoRendererPtr = ::common::AutoHandle<SDL_Renderer, ::common::Functor<SDL_DestroyRenderer>>;
	using AutoTexturePtr = ::common::AutoHandle<SDL_Texture, ::common::Functor<SDL_DestroyTexture>>;

	//把AVSampleFormat格式转化为SDL_AudioFormat
	extern const SDL_AudioFormat map_audio_formot[13];
	//转化AVPixelFormat为SDL_PixelFormatEnum格式
	extern const std::map<AVPixelFormat, SDL_PixelFormatEnum> map_video_format;

	class DriveWindow
	{
	public:
		//ffmpeg层
		std::unique_ptr<PlayTool> play_tool;
		//是否需要重新设置窗口大小
		bool isChangeSize = false;
		//----------------------------------------------------AUDIO------------------------------------------------------------------
	 private:
		//音频缓存指针
		 uint8_t* audio_buffer = nullptr;
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
		unsigned char volume= static_cast<unsigned char>(SDL_MIX_MAXVOLUME);
		//音频识别符
		SDL_AudioDeviceID device_id = 0;
		//音频播放状态
		bool is_pause = false;
		//窗口信息
		SDL_Rect rect{ 0 };
		//视频宽高比
		float aspect_ratio;
		//-----------------------------------------------------VIDEO------------------------------------------------------------------
	//private:
		//视频句柄
		AutoWindowPtr SDL_win;
		//渲染器
		AutoRendererPtr SDL_renderer;
		//纹理
		AutoTexturePtr SDL_texture;
		//SDL材质选择
		SDL_PixelFormatEnum pixel_format;
		//是否需要转化帧格式
		bool isNeedToChangeFrame = false;
		//----------------------------------------------------------------------------------------------------------------------------
	public:
		DriveWindow(FFmpegLayer::PlayTool* rely);

		~DriveWindow();

		//处理画面帧回调封装
		void convert_video_frame(AVFrame*& work, char*& buf) noexcept;
		//处理音頻帧回调封装
		void convert_audio_frame(AVFrame*& work, char*& buf) noexcept;

		//鼠标键盘事件回调函数
		void KeyMouseCallEvent() noexcept;

		//初始化音频播放环境
		void InitAudio(SDL_AudioCallback callback);
		//初始化视频环境
		void InitVideo(void* windows, int width, int height);

		//默认音頻回调函数
		static void SDLCALL default_callback(void* userdata, Uint8* stream, int len);

	public:
		//初始化播放环境
		void InitPlayer(int width, int height, void* win_handle = nullptr, SDL_AudioCallback callback = default_callback);
		//开始播放
		void StartPlayer() noexcept;
		//重绘窗口大小
		void ReSize(int width, int height) noexcept;
		//暂停播放
		void PausePlayer() noexcept;
		//恢复播放
		void ResumePlayer() noexcept;
		//触发暂停或播放
		void togglePause() noexcept;
	};
};
