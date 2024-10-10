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
		std::optional<std::pair<std::string, json>> ParseHTML(std::string_view bilibili_html_str) noexcept
		{
			std::cmatch videoResults, titleResults;
			std::regex getVideoResource("<script>window\\.__playinfo__=(.*?)</script>");
			std::regex getTitleResource("<title.*?>(.*?)</title>");

			if ((std::regex_search(bilibili_html_str.data(), videoResults, getVideoResource) == false)) return {};
			if ((std::regex_search(bilibili_html_str.data(), titleResults, getTitleResource) == false)) return {};

			try {

				auto _json = json::parse(videoResults[1].str());

				return { { titleResults[1].str(),_json["data"] } };
			}
			catch (...)
			{
				std::cout << "parse json failed please check json string" << std::endl;
				return std::nullopt;
			}
		}
		//获取网页支持的清晰度
		std::optional<std::pair<json,std::set<std::string>>> GetSuportedFormat(const json& bilibili_json) noexcept
		{
			try
			{
				std::set<std::string> codecs;
				
				for (auto _audio_data : bilibili_json["dash"]["audio"]) {
					std::string str = _audio_data["codecs"];
					codecs.insert(std::move(str));
				}

				return { { bilibili_json["support_formats"], codecs } };
			}
			catch (...)
			{
				std::cout << "parse json failed please check json string" << std::endl;
				return std::nullopt;
			}
		}
		std::optional<std::pair<std::string,std::string>> GetDownloadUrl(const json& bilibili_json, VideoQuality quality, std::string video_codecs, std::string audio_codecs) noexcept
		{
			std::string video_url, audio_url;

			for (auto _video_data : bilibili_json["dash"]["video"])
			{
				if(_video_data["id"] == quality && _video_data["codecs"] == video_codecs)
					video_url = _video_data["baseUrl"];
			}

			for (auto _audio_data : bilibili_json["dash"]["audio"])
			{
				if (_audio_data["codecs"] == audio_codecs)
					audio_url = _audio_data["baseUrl"];
			}

			if(video_url.empty() || audio_url.empty())
				return std::nullopt;

			return { {video_url,audio_url} };

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
			json _json = json::parse(videoResults[1].str());
			return { { titleResults[1].str(),_json["data"]["dash"]["video"][0]["baseUrl"] ,_json["data"]["dash"]["audio"][0]["baseUrl"]} };
		}
		catch (...)
		{
			std::cout << "parse json failed please check json string" << std::endl;
			return {};
		}
	}

}


