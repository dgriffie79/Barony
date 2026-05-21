#pragma once

#include <string>
#include <map>

struct Language
{
	static const char* get(const int line);
	static std::map<int, std::string> entries;
	static std::map<int, std::string> tmpEntries;
	static void reset();
	static int loadLanguage(char const* const lang, bool forceLoadBaseDirectory);
	static int reloadLanguage();
	static std::string languageCode;
};
