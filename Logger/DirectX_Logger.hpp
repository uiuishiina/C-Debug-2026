#pragma once
#include"Logger.hpp"
#include<Windows.h>
#include<sstream>

#define LOG_INFO(logger, msg) \
    (logger).LogPush(Logger::CreateLogEntry(Logger::Level::info, (msg), __FILE__, __FUNCTION__, __LINE__))

#define LOG_WARNING(logger, msg) \
    (logger).LogPush(Logger::CreateLogEntry(Logger::Level::warning, (msg), __FILE__, __FUNCTION__, __LINE__))

#define LOG_ERROR(logger, msg) \
    (logger).LogPush(Logger::CreateLogEntry(Logger::Level::error, (msg), __FILE__, __FUNCTION__, __LINE__))



namespace Logger {

	class OutputDebugWindow final :public OutputBase {

		void Print_Log(const wchar_t* str) {
			OutputDebugStringW(str);
		}
	public:
		~OutputDebugWindow() = default;
		void Output(const LogEntry& log)override {

			std::ostringstream oss;
			if (log.level == Level::error) {
				oss << "========================================\n";
			}

			oss << LevelToString(log.level) << "  " << log.message;
			if (log.use_detail) {
				oss << " [ file ] " << log.file << " [ func ] " << log.function << " [ line ] " << log.line;
			}
			oss << "\n";

			if (log.level == Level::error) {
				oss << "========================================\n";
			}
			std::wstring wstr(oss.str().begin(), oss.str().end());
			Print_Log(wstr.c_str());
		}
	};

	[[nodiscard]] std::string ToString(HRESULT hr) {
		char* msgBuf = nullptr;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&msgBuf,
			0,
			NULL
		);
		std::ostringstream oss;
		oss << "hr = " << hr << " : " << (msgBuf ? msgBuf : "Unknown error");

		if (msgBuf) {
			LocalFree(msgBuf);
		}

		return oss.str();
	}
}