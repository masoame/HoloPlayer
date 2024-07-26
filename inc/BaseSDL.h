#pragma once

#include"BaseFFmpeg.h"
extern"C"
{
#define SDL_MAIN_HANDLED
#include<SDL.h>
#undef main
}
#include<map>

namespace BaseSDL
{
	using namespace BaseFFmpeg;
	using namespace std::chrono_literals;

	using AutoWindowPtr = AutoPtr<SDL_Window, Functor<SDL_DestroyWindow>, false>;
	using AutoRendererPtr = AutoPtr<SDL_Renderer, Functor<SDL_DestroyRenderer>, false>;
	using AutoTexturePtr = AutoPtr<SDL_Texture, Functor<SDL_DestroyTexture>, false>;

	//把AVSampleFormat格式转化为SDL_AudioFormat
	extern const SDL_AudioFormat map_audio_formot[13];
	//转化AVPixelFormat为SDL_PixelFormatEnum格式
	extern const std::map<AVPixelFormat, SDL_PixelFormatEnum> map_video_format;

	//指向对应FFmpeg封装层的指针
	extern PlayTool* target;
    extern std::jthread Thr_Player;

    inline namespace Global_AudioRunning
	{
		//音频缓存指针
		extern Uint8* audio_buf;
		//音频工作指向
		extern Uint8* audio_pos;
		//采样到数据大小 =（采样个数) nb_sample * (采样点大小) bit_size
		extern int audio_buflen;
        //原始帧是否为planner格式
        extern bool is_planner;
        //音量大小值
        extern char volume;
	};

    inline namespace Global_VideoRunning
	{
		//视频句柄
		extern AutoWindowPtr SDL_win;
		//渲染器
		extern AutoRendererPtr SDL_renderer;
		//纹理
		extern AutoTexturePtr SDL_texture;
		//窗口信息
		extern SDL_Rect rect;
		//SDL材质选择
		extern SDL_PixelFormatEnum last_format;
	};

	extern void bindPlayTool(BaseFFmpeg::PlayTool& rely) noexcept;

	//处理画面帧回调封装
	extern void convert_video_frame(AVFrame*& work, char*& buf) noexcept;
	//处理音頻帧回调封装
	extern void convert_audio_frame(AVFrame*& work, char*& buf) noexcept;
	//鼠标键盘事件回调函数
	extern void KeyMouseCallEvent() noexcept;

	//初始化音频播放环境
	extern void InitAudio(SDL_AudioCallback callback);
	//初始化视频环境
	extern void InitVideo(const char* title);

	//默认音頻回调函数
	extern void SDLCALL default_callback(void* userdata, Uint8* stream, int len) noexcept;

	//初始化播放环境
	extern void InitPlayer(const char* WindowName, SDL_AudioCallback callback = default_callback);
	//开始播放
    extern void StartPlayer() noexcept;
    //销毁
    extern void Destroy() noexcept;
};
