#pragma once

#include<optional>
#include<vector>
#include<string>

namespace ParseJson
{
	extern std::optional<std::vector<std::string>> ParseBilibili(std::string&& bilibili_html_str);
	extern void TestMain();
}