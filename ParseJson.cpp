#include"ParseJson.h"

#include<fstream>
#include<iostream>
#include<nlohmann/json.hpp>
#include<Windows.h>
#include<regex>

namespace ParseJson
{
	using namespace nlohmann;
	using namespace nlohmann::literals;

	std::optional<std::vector<std::string>> ParseBilibili(std::string&& bilibili_html_str)
	{
		std::smatch videoResults, titleResults;
		std::regex getVideoResource("<script>window.__playinfo__=(.*?)</script>");
		std::regex getTitleResource("<title.*?>(.*?)</title>");

		if ((std::regex_search(bilibili_html_str, videoResults, getVideoResource) == false)) return {};
		if ((std::regex_search(bilibili_html_str, titleResults, getTitleResource) == false)) return {};

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


