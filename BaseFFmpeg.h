#pragma once

#include<semaphore>
#include<thread>
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
#include<windows.h>

class BaseFFmpeg
{
public:
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

	enum ios :char
	{
		in = 0x0,
		out = 0x1,
		video = 0x0,
		audio = 0x02
	};
	
    enum thread_type: char
	{
        playing_thread=0x01,
        read_thread=0x02,
        decode_thread=0x04,
        delete_thread=0x08
	};

	//sample_bit_size[AVSampleFormat(音频采样格式)] == 采样点的大小
	static inline constexpr const char sample_bit_size[13]{ 1,2,4,4,8,1,2,4,4,8,8,8,-1 };
	//planner转化为对应的packed(AV_SAMPLE_FMT_NONE为错误格式)
	static inline constexpr const AVSampleFormat map_palnner_to_packad[13]
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

	//构造函数
	explicit BaseFFmpeg() {
        FrameQueue[AVMEDIA_TYPE_VIDEO].reset(new Circular_Queue<framedata_type,4>);
        FrameQueue[AVMEDIA_TYPE_AUDIO].reset(new Circular_Queue<framedata_type,4>);
	};
	//析构
    ~BaseFFmpeg() { clear(); };

	//打开流
	RESULT open(const char* url, byte type = in | video);

	//初始化您编解码器
	RESULT init_decode(const char* url, const AVInputFormat* fmt = nullptr, AVDictionary** options = nullptr);
	//初始化编码器
	RESULT init_encode(const char* url, const AVOutputFormat* fmt = nullptr);

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

	void seek_time(int64_t usec)noexcept;


    //暂停线程
    void stop(char type) noexcept;
    //恢复线程
    void run(char type) noexcept;
	//关闭所有相关功能
    void clear() noexcept;
private:

	AutoSwrContextPtr swr_ctx;


	//队列用于存放帧数据

public:

	std::unique_ptr<Circular_Queue_API<framedata_type>> FrameQueue[4];
    Circular_Queue<AutoAVPacketPtr> PacketQueue;

    //本地线程状态（）
    uint8_t local_thread;
	//输入输出格式指针
	AutoAVFormatContextPtr avfctx_input, avfctx_output;
	//解码上下文指针
	AutoAVCodecContextPtr decode_ctx[4];
	//编码上下文指针
	AutoAVCodecContextPtr encode_ctx[4];
	//图像帧转化上下文
	AutoSwsContextPtr sws_ctx;
	//各个工作帧
    auto_framedata_type avframe_work[6];
	//insert_callback[AVMediaType(帧格式)] == 处理函数指针
	insert_callback_type insert_callback[6]{ 0 };
	// AVStreamIndexToType[流的索引] == 流类型
	AVMediaType AVStreamIndexToType[6]{ AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN ,AVMEDIA_TYPE_UNKNOWN,AVMEDIA_TYPE_UNKNOWN };
	// AVTypeToStreamIndex[流类型] == 流的索引
	int AVTypeToStreamIndex[6]{ -1,-1,-1,-1,-1,-1 };

	double secBaseVideo = 0 , secBaseAudio = 0;

    std::jthread ThrRead;
    std::binary_semaphore wait_read_pause{0};
    std::binary_semaphore run_read_thread{0};

    std::jthread ThrDecode;
    std::binary_semaphore wait_decode_pause{0};
    std::binary_semaphore run_decode_thread{0};
};
