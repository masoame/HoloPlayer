#pragma once
#include<optional>
#include<vector>
#include<string>
#include<map>
#include<nlohmann/json.hpp>
#include<set>
namespace ParseJson
{

	using namespace nlohmann;
	using namespace nlohmann::literals;

	namespace BiliBili
	{
		enum class VideoQuality
		{
			_bilibili_360p = 16,
			_bilibili_480p = 32,
			_bilibili_720p = 64,
			_bilibili_1080p = 80,
			_bilibili_1080p_plus = 112,
		};

		extern std::optional<std::pair<std::string, json>> ParseHTML(std::string_view bilibili_html_str) noexcept;

		extern std::optional <std::pair<json, std::set<std::string>>> GetSuportedFormat(const json& bilibili_json) noexcept;
		extern std::optional<std::pair<std::string, std::string>> GetDownloadUrl(const json& bilibili_json, VideoQuality quality, std::string video_codec, std::string audio_codec) noexcept;

	}
}
