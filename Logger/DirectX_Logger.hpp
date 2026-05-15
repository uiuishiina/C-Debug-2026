#pragma once
//-------- anoter hpp
#include"Logger.hpp"
#include<Windows.h>
#include<sstream>

namespace Logger {

	// HRESULT 専用ラッパー
	struct HResultValue {
		HRESULT value;
	};

	// 作りやすくする補助関数
	[[nodiscard]] inline HResultValue AsHResult(HRESULT hr) {
		return HResultValue{ hr };
	}

	// HResultValue 専用の traits specialization
	template<>
	struct LogStringTraits<HResultValue, void> {
		static std::string Convert(const HResultValue& hrValue) {
			HRESULT hr = hrValue.value;

			char* msgBuf = nullptr;
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				static_cast<DWORD>(hr),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&msgBuf),
				0,
				nullptr
			);

			std::ostringstream oss;
			oss << "hr = 0x"
				<< std::hex << std::uppercase
				<< static_cast<unsigned long>(hr)
				<< std::dec
				<< " : "
				<< (msgBuf ? msgBuf : "Unknown error");

			if (msgBuf) {
				LocalFree(msgBuf);
			}

			return oss.str();
		}
	};

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
			std::string s = oss.str();
			std::wstring wstr(s.begin(), s.end());
			Print_Log(wstr.c_str());
		}
	};
}