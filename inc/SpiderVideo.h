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

		CURLcode RequestInit(const char* request_url);
		CURLcode SetOption(char type, std::string savepath = "", std::initializer_list<const char*> headerOption_list = {}, int connecttimeout = 5);
		CURLcode RequestSoure();
		CURLcode RequestOnlyHeader();

	private:
		AutoCURLPtr _curl;
		char _type;
        std::ofstream _writefile;

		static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
		static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
	};

}
