#pragma once
#include <string>
#include <thread>
#include <concepts>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

#define LOG_INFO(msg,detail) \
    Logger::LoggerCore::Instance().LogPush(Logger::CreateLogEntry(Logger::Level::info, (msg), __FILE__, __FUNCTION__, __LINE__, detail))

#define LOG_WARNING(msg,detail) \
    Logger::LoggerCore::Instance().LogPush(Logger::CreateLogEntry(Logger::Level::warning, (msg), __FILE__, __FUNCTION__, __LINE__,detail))

#define LOG_ERROR(msg) \
    Logger::LoggerCore::Instance().LogPush(Logger::CreateLogEntry(Logger::Level::error, (msg), __FILE__, __FUNCTION__, __LINE__, true))


//@brief	| 自作ログ名前空間 |
namespace Logger {

	//@brief	| ログレベル |
	//@param	info	通常ログ
	//@param	warning	注意ログ
	//@param	error	エラーログ
	enum class Level :uint8_t {
		info, warning, error
	};

	//@brief	=== ログレベル版文字列変換関数 ===
	//@return	対応する文字リテラル
	[[nodiscard]] constexpr std::string_view LevelToString(Level level) {
		switch (level)
		{
		case Level::info:
			return "[ Info ]";
		case Level::warning:
			return "[ Warning ]";
		case Level::error:
			return "[ Error ]";
		default:
			return "[ None ]";
		}
	};
	
	//@brief	| ログ構造体 |
	//@param	level		ログレベル
	//@param	message		ログ
	//@param	file		ログ出力ファイル
	//@param	function	ログ出力関数
	//@param	line		ログ出力行
	//@param	thread_id	スレッドID
	struct LogEntry {

		Level level;
		std::string message;
		std::string file;
		std::string function;
		int line;
		bool use_detail;
	};

	template<class>
	inline constexpr bool always_false = false;

	// ---------- traits 本体 ----------
	template<typename T, typename = void>
	struct LogStringTraits {
		static std::string Convert(const T& value) {
			if constexpr (requires(const T & v) {
				{ ToString(v) } -> std::convertible_to<std::string>;
			}) {
				return ToString(value);
			}
			else if constexpr (requires(const T & v) {
				{ std::to_string(v) } -> std::convertible_to<std::string>;
			}) {
				return std::to_string(value);
			}
			else {
				static_assert(always_false<T>, "ConvertToLogString failed");
			}
		}
	};

	// std::string はそのまま返す
	template<>
	struct LogStringTraits<std::string, void> {
		static std::string Convert(const std::string& value) {
			return value;
		}
	};

	// const char* も個別対応
	template<>
	struct LogStringTraits<const char*, void> {
		static std::string Convert(const char* value) {
			return value ? std::string(value) : std::string{ "null" };
		}
	};

	template<>
	struct LogStringTraits<char*, void> {
		static std::string Convert(const char* value) {
			return value ? std::string(value) : std::string{ "null" };
		}
	};

	template<typename T>
	[[nodiscard]] std::string ConvertToLogString(const T& value) {
		using U = std::remove_cvref_t<T>;
		return LogStringTraits<U>::Convert(value);
	}

	//@brief	=== ログ作成関数 ===
	//@return	作成されたログ
	template<typename T>
	[[nodiscard]] LogEntry CreateLogEntry(Level level, const T& value,
		const char* file, const char* function, int line, bool detail = false) {

		return LogEntry{ level,ConvertToLogString(value),ToString(file),ToString(function),line,detail };
	};

	//@brief	/ === ログ一時保存キュークラス === /
	class LogQueue final
	{
		std::queue<LogEntry> log_queue_{};
		std::mutex mutex_;
		std::condition_variable cv_;
	public:
		[[nodiscard]] bool Is_QueueEnpty() {
			std::lock_guard lock(mutex_);
			return log_queue_.empty();
		}

		//@brief	=== ログ追加関数 ===
		void PushQueue(LogEntry log) {
			{
				std::lock_guard lock(mutex_);
				log_queue_.push(std::move(log));
			}
			cv_.notify_one();
		}

		//@brief	=== ログ取得関数 ===
		//@return	キューの先頭ログ
		[[nodiscard]] LogEntry PopQueue(const std::atomic_bool& is_running) {
			std::unique_lock lock(mutex_);

			cv_.wait(lock, [&] {
				return !log_queue_.empty() || !is_running.load();
				});

			if (log_queue_.empty()) {
				return {};
			}

			LogEntry log = std::move(log_queue_.front());
			log_queue_.pop();
			return log;
		}

		void AllNotify() {
			cv_.notify_all();
		}
	};

	//@brief	/ === ログ出力基底クラス === /
	class OutputBase {
	public:
		OutputBase() = default;
		virtual ~OutputBase() = default;
		virtual void Output(const LogEntry& log) = 0;
	};


	template<typename T>
	concept DerivedFromOutputBase = std::is_base_of_v<OutputBase, T>;

	//@brief	/ === ログ中心クラス === /
	class LoggerCore final
	{
		LogQueue queue_{};
		std::thread log_thread_{};
		std::unique_ptr<OutputBase> output_{};

		std::atomic_bool is_running = false;

		void ThreadLoop() {
			while (true) {
				auto log = queue_.PopQueue(is_running);

				if (!is_running && log.message.empty()) {
					break;
				}
					
				if (output_ && !log.message.empty()) {
					output_->Output(log);
				}
			}
		}
	public:
		static LoggerCore& Instance() {
			static LoggerCore ins;
			return ins;
		}

		LoggerCore() = default;
		~LoggerCore() {
			is_running = false;
			queue_.AllNotify();
			if (log_thread_.joinable()){
				log_thread_.join();
			}
		}
		
		template<DerivedFromOutputBase T>
		void Initalize() {
			auto output = std::make_unique<T>();
			output_ = std::move(output);
			is_running = true;
			log_thread_ = std::thread([this] {ThreadLoop();});
		}

		void LogPush(const LogEntry& log) {
			if (is_running) {
				queue_.PushQueue(log);
			}	
		}
	};
};