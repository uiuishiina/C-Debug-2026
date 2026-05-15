
#include"Logger/Logger.hpp"
#include"Logger/DirectX_Logger.hpp"

int main() {
	Logger::Logger logger(Logger::OutputDebugWindow);

	LOG_INFO(logger, "init start");
	LOG_ERROR(logger, "failed to load");
	return 0;
}