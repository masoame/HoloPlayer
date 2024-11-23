#pragma once
#include<string>
#include<fstream>
#include<vector>
#include<curl/curl.h>
#include "common.hpp"

namespace SpiderVideo
{

     inline void __stdcall _curl_easy_cleanup(CURL* curl)
     {
     	curl_easy_cleanup(curl);
     }
     inline void __stdcall _curl_slist_free_all(curl_slist* list)
     {
     	curl_slist_free_all(list);
     }

    using AutoCURLPtr = ::common::AutoHandle<CURL, ::common::Functor<_curl_easy_cleanup>>;
    using AutoCurl_slistPtr = ::common::AutoHandle<curl_slist, ::common::Functor<_curl_slist_free_all>>;

	enum RequestType
	{
		saveFile = 0x01,
		saveBuffer = 0x02,
		multiThread = 0x04
	};

	struct SpiderTool
	{
		AutoCurl_slistPtr header = nullptr;
        std::string filePath;

		std::vector<char> buffer;

		size_t loadSize = 0;
		size_t size = 0;

		CURLcode RequestInit(std::string_view request_url);
		CURLcode SetOption(char type, std::string savepath = "", std::initializer_list<const char*> headerOption_list = {}, int connecttimeout = 5);
		CURLcode RequestSoure();
		CURLcode RequestOnlyHeader();


		inline CURLcode _curl_easy_setopt(CURLoption option, auto... args) {
			return ::curl_easy_setopt(this->_curl, option, args...);
		}

		template<typename... Args, size_t... Is>
		inline CURLcode _curl_easy_setopt(const std::tuple<Args...>& option_tuple,std::index_sequence<Is...>) {
			return _curl_easy_setopt(std::get<Is>(option_tuple)...);
		}

		template<typename... Args, typename Index = std::make_index_sequence<sizeof...(Args)>>
		inline CURLcode _curl_easy_setopt_ex(const std::tuple<Args...>& option_tuple) {
			return _curl_easy_setopt(option_tuple, Index{});
		}

		template<typename... Args>
		inline std::initializer_list<CURLcode> _curl_easy_setopt_list(Args&& ...option_tuple) {
			return { _curl_easy_setopt_ex(std::forward<Args>(option_tuple))... };
		}




		//inline bool _curl_easy_setopt_ex(auto... option_list) {
		//	for (auto option : option_list) {
		//		if (this->_curl_easy_setopt(option.first, option.second)!= CURLE_OK) return false;
		//	}
		//	return true;
		//}

	private:
		AutoCURLPtr _curl;
		std::string _temp_url;
		char _type;
        std::ofstream _writefile;

		static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
		static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
	};

}
