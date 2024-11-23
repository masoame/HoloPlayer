#include"ParseJson.h"

#include<iostream>
#include"ParseJson.h"
#include<set>
#include<Windows.h>
#include<regex>
#include<vector>
namespace ParseJson
{
	namespace BiliBili
	{


		//网页清晰度表
		std::map<std::string, std::pair<VideoQuality, std::vector<std::string>>> SupportedQualityMap;

		//解析Bilibili的HTML页面
		std::optional<std::pair<std::string, ::nlohmann::json>> ParseHTML(std::string_view bilibili_html_str) noexcept
		{
			std::cmatch videoResults, titleResults;
			std::regex getVideoResource("<script>window\\.__playinfo__=(.*?)</script>");
			std::regex getTitleResource("<title.*?>(.*?)</title>");

			if ((std::regex_search(bilibili_html_str.data(), videoResults, getVideoResource) == false)) return {};
			if ((std::regex_search(bilibili_html_str.data(), titleResults, getTitleResource) == false)) return {};

			try {

				auto _json = ::nlohmann::json::parse(videoResults[1].str());

				return { { titleResults[1].str(),_json["data"] } };
			}
			catch (...)
			{
				std::cout << "parse json failed please check json string" << std::endl;
				return std::nullopt;
			}
		}
		std::list<std::string> GetSuportedQualities(const::nlohmann::json& bilibili_json) noexcept
		{
			try
			{
				std::list<std::string> supported_qualities;

				for (auto& _audio_data : bilibili_json["support_formats"]) {
					supported_qualities.emplace_back( _audio_data["new_description"]);
				}
				return supported_qualities;
			}
			catch (...)
			{
				std::cout << "GetSuportedQualities failed please check json string" << std::endl;
				return {};
			}
		}
		std::list<std::string> GetSuportedVideoCodecs(const::nlohmann::json& bilibili_json, std::string quality) noexcept
		{
			try
			{
				std::list<std::string> supported_video_codecs;

				for (auto& _video_data : bilibili_json["support_formats"]) {
					if (_video_data["new_description"] == quality) {
						for (auto& _codec : _video_data["codecs"]){
							supported_video_codecs.emplace_back(_codec);
						}
						break;
					}
				}
				return supported_video_codecs;
			}catch (...)
			{
				std::cout << "GetSuportedVideoCodecs failed please check json string" << std::endl;
				return {};
			}
			
		}

		std::set<std::string> GetSuportedAudioCodecs(const::nlohmann::json& bilibili_json) noexcept
		{
			try
			{
				std::set<std::string> supported_audio_codecs;
				for (auto& _audio_data : bilibili_json["dash"]["audio"]) {
					supported_audio_codecs.insert(_audio_data["codecs"]);
				}
				return supported_audio_codecs;
			}
			catch (...)
			{
				std::cout << "GetSuportedAudioCodecs failed please check json string" << std::endl;
				return {};
			}
		}
		inline VideoQuality QualitiesToId(const::nlohmann::json& bilibili_json, std::string_view quality) noexcept
		{
			for (auto& _video_data : bilibili_json["support_formats"]) {
				if (static_cast<std::string>(_video_data["new_description"]) == quality) {
					return _video_data["quality"];
				}
			}
			return VideoQuality::_bilibili_default;
		}

		std::pair<std::string,std::string> GetDownloadUrl(Data& _data) noexcept
		{
			std::string video_url, audio_url;
			auto id = QualitiesToId(_data.all_data, _data.select_quality);
			for (auto& _video_data : _data.all_data["dash"]["video"])
			{
				//int _id = _video_data["id"];
				std::string _codecs = _video_data["codecs"];
				if(_video_data["id"] == id && _video_data["codecs"] == _data.select_video_codecs)
					video_url = _video_data["baseUrl"];
			}

			for (auto& _audio_data : _data.all_data["dash"]["audio"])
			{
				if (_audio_data["codecs"] == _data.select_audio_codecs)
					audio_url = _audio_data["baseUrl"];
			}

			if(video_url.empty() || audio_url.empty())
				return {};

			return  {video_url,audio_url} ;

		}
	}

	std::optional<std::vector<std::string>> ParseYouTube(std::string&& YouTube_html_str)
	{
		std::smatch videoResults, titleResults;
		std::regex getVideoResource("<script nonce=.*?> </script>");
		std::regex getTitleResource("<title.*?>(.*?)</title>");

		if ((std::regex_search(YouTube_html_str, videoResults, getVideoResource) == false)) return {};
		if ((std::regex_search(YouTube_html_str, titleResults, getTitleResource) == false)) return {};

		try {
			auto _json = ::nlohmann::json::parse(videoResults[1].str());
			return { { titleResults[1].str(),_json["data"]["dash"]["video"][0]["baseUrl"] ,_json["data"]["dash"]["audio"][0]["baseUrl"]} };
		}
		catch (...)
		{
			std::cout << "parse json failed please check json string" << std::endl;
			return {};
		}
	}

}


