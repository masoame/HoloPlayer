#pragma once
extern"C"
{
#include<libavformat/avformat.h>
#include<libavcodec/avcodec.h>
#include<libswscale/swscale.h>
#include<libavfilter/avfilter.h>
#include<libavutil/avutil.h>
#include<libswresample/swresample.h>
#include<libavutil/channel_layout.h>
}
#include"common.hpp"
#include"Cirucular_Queue.hpp"

#include<semaphore>
#include<thread>


namespace BaseFFmpeg
{
	using namespace std::chrono_literals;

	//管理内存的智能指针
	using AutoAVPacketPtr = AutoPtr<AVPacket, Functor<av_packet_free>, true>;
	using AutoAVCodecContextPtr = AutoPtr<AVCodecContext, Functor<avcodec_free_context>, true>;
	using AutoAVFormatContextPtr = AutoPtr<AVFormatContext, Functor<avformat_free_context>, false>;
	using AutoSwsContextPtr = AutoPtr<SwsContext, Functor<sws_freeContext>, false>;
	using AutoSwrContextPtr = AutoPtr<SwrContext, Functor<swr_free>, true>;
	using AutoAVFramePtr = AutoPtr<AVFrame, Functor<av_frame_free>, true>;

	//函数回调类型
	using insert_callback_type = void (*)(AVFrame*&, char*& buf) noexcept;

	using framedata_type = std::pair<AutoAVFramePtr, std::unique_ptr<char[]>>;
	using auto_framedata_type = std::pair<AutoAVFramePtr, char*>;
	//错误枚举
	enum RESULT
	{
		SUCCESS, UNKONW_ERROR, ARGS_ERROR,
		ALLOC_ERROR, OPEN_ERROR, INIT_ERROR,
		UNNEED_SWR
	};

	//流类型
	enum ios :char
	{
		in = 0x1,
		out = 0x2,
		io = in | out,
		video = 0x4,
		audio = 0x8
	};

	//线程类型
	enum thread_type : char
	{
		playing_thread = 0x01,
		read_thread = 0x02,
		decode_thread = 0x04,
		delete_thread = 0x08
	};

	//输入或输出帧格式
	typedef struct MediaType
	{
		int w, h;
		AVPixelFormat video_type;
		AVSampleFormat audio_type;
	}IOMediaType;

	extern const char sample_bit_size[13];
	extern const AVSampleFormat map_palnner_to_packad[13];

	//播放工具类
	class PlayTool
	{
	public:

		explicit PlayTool() {
			FrameQueue[AVMEDIA_TYPE_VIDEO].reset(new Circular_Queue<framedata_type>);
			FrameQueue[AVMEDIA_TYPE_AUDIO].reset(new Circular_Queue<framedata_type,5>);
		};
		~PlayTool() { clear(); };

		//打开流
        RESULT open(const char* srcUrl, const char* dstUrl = nullptr, unsigned char type = in | video | audio);
		//初始化各种编解码器
		RESULT init_decode(AVMediaType type);
		//初始化编码器
		RESULT init_encode(AVMediaType type);

		//初始化音频重采样(planner到packed格式转化)
		RESULT init_swr();
		//转化函数(planner到packed格式转化)
		RESULT sample_planner_to_packed(AVFrame* frame, uint8_t** data, int* linesize);
		//帧格式转化
		RESULT init_sws(AVFrame* work, const AVPixelFormat dstFormat = AV_PIX_FMT_YUV420P, const int dstW = 0, const int dstH = 0);
		//转化为
		RESULT sws_scale_420P(AVFrame*& data);
		//本地音视频解码
		RESULT start_read_thread() noexcept;
		RESULT start_decode_thread() noexcept;
		//本地音视频编码
		RESULT start_encode_thread() noexcept;
		//音视频拉流解码线程
		RESULT start_pull_steam_thread() noexcept;

		//插入队列
		void insert_queue(AVMediaType index, AutoAVFramePtr&& avf) noexcept;
		//从队列中取出并刷新工作指针的指向
		bool flush_frame(AVMediaType index) noexcept;
		//定位到对应的时间
		void seek_time(int64_t usec)noexcept;
		//暂停线程
		void stop(const char type) noexcept;
		//恢复线程
		void run(const char type) noexcept;
		//结束线程
		void clear(const char type) noexcept;
		//关闭所有相关功能
		void clear() noexcept;

	public:
		
		//视频帧,音频帧，字幕帧队列[AVMediaType]
		std::unique_ptr<Circular_Queue_API<framedata_type>> FrameQueue[3];
		//读取到包管理队列
		Circular_Queue<AutoAVPacketPtr, 8> PacketQueue;

		//本地线程状态
		uint8_t local_thread;
		//输入输出格式指针
		AutoAVFormatContextPtr avfctx_input, avfctx_output;
		//解码上下文指针
		AutoAVCodecContextPtr decode_ctx[3];
		//编码上下文指针
		AutoAVCodecContextPtr encode_ctx[3];

		//图像帧转化上下文
		AutoSwsContextPtr sws_ctx;
		//音频解码上下文
		AutoSwrContextPtr swr_ctx;

		//各个工作帧
		auto_framedata_type avframe_work[6];

		//insert_callback[AVMediaType(帧格式)] == 处理函数指针
		insert_callback_type insert_callback[6]{ 0 };

		// AVStreamIndexToType[流的索引] == 流类型
		AVMediaType AVStreamIndexToType[6]{ AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN,AVMEDIA_TYPE_UNKNOWN };

		// AVTypeToStreamIndex[流类型] == 流的索引
		int AVTypeToStreamIndex[6]{ -1,-1,-1,-1,-1,-1 };

		//各个流的时间基
		double secBaseTime[3];

		//读取Packet线程
		std::jthread ThrRead;
		std::binary_semaphore wait_read_pause{ 0 };
		std::binary_semaphore run_read_thread{ 0 };

		//解码Packet线程
		std::jthread ThrDecode;
		std::binary_semaphore wait_decode_pause{ 0 };
		std::binary_semaphore run_decode_thread{ 0 };

		// 播放线程
		std::jthread ThrPlay;
		std::binary_semaphore wait_play_pause{ 0 };
		std::binary_semaphore run_play_thread{ 0 };
	};
}




