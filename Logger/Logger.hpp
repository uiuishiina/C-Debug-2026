#pragma once
#include <string>
#include <thread>
#include <concepts>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

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
		std::string_view file;
		std::string_view function;
		int line;
		bool use_detail;
	};

	[[nodiscard]] std::string ToString(const char* c) {
		return c ? std::string(c) : std::string{"null"};
	}

	//	自作ToString関数チェック
	template<typename T>
	concept HasToString = requires(T value) {
		{ ToString(value) } -> std::convertible_to<std::string>;
	};

	//　std::to_string関数チェック
	template<typename T>
	concept HasStdToString = requires(T value) {
		{ std::to_string(value) } -> std::convertible_to<std::string>;
	};

	template<class>
	inline constexpr bool always_false = false;

	//@brief	=== ログ変換関数 ===
	//@return	変換された文字列
	template<typename T>
	[[nodiscard]] std::string ConvertToLogString(const T& value) {
		if constexpr (HasToString<T>) {
			return ToString(value);
		}
		else if constexpr (HasStdToString<T>) {
			return std::to_string(value);
		}
		else {
			static_assert(always_false<T>, "ConvertToLogString failed");
		}
	};

	//@brief	=== ログ作成関数 ===
	//@return	作成されたログ
	template<typename T>
	[[nodiscard]] LogEntry CreateLogEntry(Level level, const T& value,
		const char* file, const char* function, int line, bool detail = false) {

		return LogEntry{ level,ConvertToLogString(value),file,function,line,detail };
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
		[[nodiscard]] LogEntry PopQueue(bool is_running) {
			
			std::unique_lock lock(mutex_);

			cv_.wait(lock, [&] {
					return !log_queue_.empty() || !is_running;;
				});

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

	//@brief	/ === ログ中心クラス === /
	class Logger final
	{
		LogQueue queue_{};
		std::thread log_thread_{};
		std::unique_ptr<OutputBase> output_{};

		std::atomic_bool is_running = true;

		void ThreadLoop() {
			while (true) {
				auto log = queue_.PopQueue(is_running);

				if (!is_running && log.message.empty()) {
					break;
				}
					
				if (output_) {
					output_->Output(log);
				}
			}
		}
	public:
		Logger(std::unique_ptr<OutputBase> output) : output_(std::move(output)) {
			log_thread_ = std::thread([this] {ThreadLoop(); 
				});
		}
		~Logger() {
			is_running = false;
			queue_.AllNotify();
			if (log_thread_.joinable()){
				log_thread_.join();
			}
		}

		void LogPush(LogEntry log) {
			queue_.PushQueue(std::move(log));
		}
	};
};