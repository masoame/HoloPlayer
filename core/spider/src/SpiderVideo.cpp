#include"SpiderVideo.h"
#include<iostream>
#include<regex>
#include<string>

namespace SpiderVideo
{
    static std::regex getHeaderValue{ "Content-Length: (\\d*?)\r\n" ,std::regex::icase};

    size_t SpiderTool::header_callback(char* buffer, size_t size, size_t nitems, void* userdata)
    {
        SpiderTool* _userdata = static_cast<SpiderTool*>(userdata);
        std::smatch result;
        std::string str(buffer);
        if (std::regex_match(str, result, getHeaderValue)) { _userdata->loadSize = std::atoi(result[1].str().c_str()); _userdata->size = 0; }

        return size * nitems;
    }
    size_t SpiderTool::write_callback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t realsize = size * nmemb;
        if (userp == nullptr) return realsize;

        SpiderTool* _userdata = static_cast<SpiderTool*>(userp);
        char* _backcalldata = static_cast<char*>(contents);

        if (_userdata->_writefile.is_open())
            _userdata->_writefile.write(_backcalldata, realsize);
        if (_userdata->_type & saveBuffer)_userdata->buffer.insert(_userdata->buffer.end(), _backcalldata, _backcalldata + realsize);

        _userdata->size += realsize;

        return realsize;
    }

    CURLcode SpiderTool::RequestInit(std::string_view request_url)
    {
        if (request_url.empty()==true || request_url == "") return CURLE_FAILED_INIT;

        this->_curl.reset(curl_easy_init());
        if (this->_curl == nullptr) return CURLE_FAILED_INIT;


#ifdef _DEBUG
        CURLcode code = _curl_easy_setopt(CURLOPT_VERBOSE, true);
        if (code != CURLE_OK) return code;
#endif
        _curl_easy_setopt_list(
            std::forward_as_tuple(CURLOPT_URL, request_url.data()),
            std::forward_as_tuple(CURLOPT_PROXY, "127.0.0.1:10809"),
            std::forward_as_tuple(CURLOPT_SSL_VERIFYPEER, 0L),
            std::forward_as_tuple(CURLOPT_SSL_VERIFYHOST, 0L),
            std::forward_as_tuple(CURLOPT_CA_CACHE_TIMEOUT, 300L),
            std::forward_as_tuple(CURLOPT_FOLLOWLOCATION, 1L),
            std::forward_as_tuple(CURLOPT_ACCEPT_ENCODING, ""),
            std::forward_as_tuple(CURLOPT_HEADERFUNCTION, header_callback),
            std::forward_as_tuple(CURLOPT_HEADERDATA, this),
            std::forward_as_tuple(CURLOPT_WRITEFUNCTION, write_callback),
            std::forward_as_tuple(CURLOPT_WRITEDATA, this)
        );
        return CURLE_OK;
    }

    CURLcode SpiderTool::SetOption(char type, std::string savepath, std::initializer_list<const char*> headerOption_list,int connecttimeout )
    {
        this->_type = type;
        if (this->_type & saveFile)
        {
            this->filePath = savepath;
            if (!this->filePath.empty() && this->filePath != "")this->_writefile.open(savepath, ::std::ios::binary | ::std::ios::out) ;
            else return CURLE_FAILED_INIT;
        }

        CURLcode code = _curl_easy_setopt(CURLOPT_CONNECTTIMEOUT, connecttimeout);
        if (code != CURLE_OK) return code;

        this->header = nullptr;
        curl_slist*& list = this->header;
        for (auto& str : headerOption_list) if(str != nullptr) list = curl_slist_append(list, str);
        if(list !=nullptr) return _curl_easy_setopt(CURLOPT_HTTPHEADER, list);
        return CURLE_OK;
    }

    CURLcode SpiderTool::RequestSoure()
	{
        CURLcode code = curl_easy_perform(this->_curl);
        if (code != CURLE_OK) return code;
        if(this->_writefile.is_open())this->_writefile.close();
        if(this->_type & saveBuffer) this->buffer.emplace_back('\0');

        return CURLE_OK;
	}
    CURLcode SpiderTool::RequestOnlyHeader()
    {
        CURLcode code = _curl_easy_setopt(CURLOPT_HEADER, 1);
        if (code != CURLE_OK) return code;

        code = _curl_easy_setopt(CURLOPT_NOBODY, 1);
        if (code != CURLE_OK) return code;

        code = curl_easy_perform(this->_curl);
        if (code != CURLE_OK) return code;

        if (this->_writefile.is_open())this->_writefile.close();
        if (this->_type & saveBuffer) this->buffer.emplace_back('\0');
        return CURLE_OK;
    }

}

