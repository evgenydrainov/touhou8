#pragma once

#include <string_view>
#include <charconv>

#define ArrayLength(a) (sizeof(a) / sizeof(*a))

#define TH_STRINGIZE(x) TH_STRINGIZE2(x)
#define TH_STRINGIZE2(x) #x
#define TH_LINE_STRING TH_STRINGIZE(__LINE__)

#define LOG(fmt, ...) \
	SDL_Log(fmt, __VA_ARGS__)

#define LOG_NO_LINE(fmt, ...) \
	SDL_LogMessage(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_INFO, fmt, __VA_ARGS__)

#define LOG_FILE(fmt, ...) \
	do { \
		char file[] = __FILE__; \
		std::string_view view(file, sizeof(file)); \
		size_t pos = view.rfind('\\') + 1; \
		SDL_Log("%s:" TH_LINE_STRING ": " fmt, file + pos, __VA_ARGS__); \
	} while (0)

#define LOG_FUNC(fmt, ...) \
	SDL_Log(__FUNCTION__ ": " fmt, __VA_ARGS__)

namespace th {

	inline std::string_view ReadWord(std::string_view str, size_t* cursor) {
		while (*cursor < str.size() && str[*cursor] == ' ') {
			(*cursor)++;
		}

		if (*cursor >= str.size()) {
			return {};
		}

		size_t pos = str.find(' ', *cursor);
		if (pos == std::string::npos) {
			std::string_view result = str.substr(*cursor);
			*cursor = str.size();
			return result;
		}

		std::string_view result = str.substr(*cursor, pos - *cursor);
		*cursor = pos + 1;
		return result;
	}

	inline int StrToInt(std::string_view str, int _default = 0) {
		if (str.empty()) return _default;

		int result = _default;
		std::from_chars(str.data(), str.data() + str.size(), result);
		return result;
	}

	inline float StrToFloat(std::string_view str, float _default = 0.0f) {
		if (str.empty()) return _default;

		float result = _default;
		std::from_chars(str.data(), str.data() + str.size(), result);
		return result;
	}

	inline double GetTime() {
		return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
	}

}
