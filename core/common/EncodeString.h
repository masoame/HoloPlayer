#pragma once
#include<string>
#ifdef _WIN32
	#include<windows.h>
#else
	#include<codecvt>
#endif // _Win32
#include<functional>
inline std::wstring MultiStringToWideString(UINT CodePage,const std::string& str)
{
	std::wstring result(MultiByteToWideChar(CodePage, 0, str.c_str(), -1, nullptr, 0),L'\0');
	MultiByteToWideChar(CodePage, 0, str.c_str(), -1, result.data(), result.size());
	return result;
}

inline std::string WideStringToMultiString(UINT CodePage,const std::wstring& str)
{
	std::string result(WideCharToMultiByte(CodePage, 0, str.c_str(), -1, nullptr, 0, NULL, NULL),'\0');
	WideCharToMultiByte(CodePage, 0, str.c_str(), -1, result.data(), result.size(), NULL, NULL);
	return result;
}

constexpr static auto UTF8ToUTF16 = std::bind(MultiStringToWideString, CP_UTF8, std::placeholders::_1);
constexpr static auto ANSIToUTF16 = std::bind(MultiStringToWideString, CP_ACP, std::placeholders::_1);
constexpr static auto GBKToUTF16 = std::bind(MultiStringToWideString, 936, std::placeholders::_1);

constexpr static auto UTF16ToUTF8 = std::bind(WideStringToMultiString, CP_UTF8, std::placeholders::_1);
constexpr static auto UTF16ToANSI = std::bind(WideStringToMultiString, CP_ACP, std::placeholders::_1);
constexpr static auto UTF16ToGBK = std::bind(WideStringToMultiString, 936, std::placeholders::_1);



