#pragma once
#include<optional>
#include<vector>
#include<string>
#include<map>
#include<nlohmann/json.hpp>
#include<set>
namespace ParseJson
{
	namespace BiliBili
	{
		enum class VideoQuality
		{
			_bilibili_default = 0,
			_bilibili_360p = 16,
			_bilibili_480p = 32,
			_bilibili_720p = 64,
			_bilibili_1080p = 80,
			_bilibili_1080p_plus = 112,
		};

		struct Data {
			::nlohmann::json all_data;
			std::string select_quality;
			std::string select_video_codecs;
			std::string select_audio_codecs;
		};

		using DataMap = std::map<std::string, Data>;

		extern std::optional<std::pair<std::string, ::nlohmann::json>> ParseHTML(std::string_view bilibili_html_str) noexcept;

		extern std::list<std::string> GetSuportedQualities(const ::nlohmann::json& bilibili_json) noexcept;
		extern std::list<std::string> GetSuportedVideoCodecs(const ::nlohmann::json& bilibili_json, std::string quality) noexcept;
		extern std::set<std::string> GetSuportedAudioCodecs(const ::nlohmann::json& bilibili_json) noexcept;

		extern VideoQuality QualitiesToId(const ::nlohmann::json& bilibili_json,std::string_view quality) noexcept;

		extern std::pair<std::string, std::string> GetDownloadUrl(Data& data_map) noexcept;

	}
}
