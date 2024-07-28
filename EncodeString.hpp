#include<string>
#ifdef _WIN32
	#include<windows.h>
#else
	#include<codecvt>
#endif // _Win32
template<UINT CodePage>
inline std::wstring MultiStringToWideString(const std::string& str)
{
	std::wstring result(MultiByteToWideChar(CodePage, 0, str.c_str(), -1, nullptr, 0),L'\0');
	MultiByteToWideChar(CodePage, 0, str.c_str(), -1, result.data(), result.size());
	return result;
}

template<UINT CodePage>
inline std::string WideStringToMultiString(const std::wstring& str)
{
	std::string result(WideCharToMultiByte(CodePage, 0, str.c_str(), -1, nullptr, 0, NULL, NULL),'\0');
	WideCharToMultiByte(CodePage, 0, str.c_str(), -1, result.data(), result.size(), NULL, NULL);
	return result;
}
constexpr auto& UTF8ToUTF16 = MultiStringToWideString<CP_UTF8>;
constexpr auto& ANSIToUTF16 = MultiStringToWideString<CP_ACP>;
constexpr auto& GBKToUTF16 = MultiStringToWideString<936>;

constexpr auto& UTF16ToUTF8 = WideStringToMultiString<CP_UTF8>;
constexpr auto& UTF16ToANSI = WideStringToMultiString<CP_ACP>;
constexpr auto& UTF16ToGBK = WideStringToMultiString<936>;




