#include"Logger/Logger.hpp"
#include"Logger/DirectX_Logger.hpp"

int main() {
	
	Logger::LoggerCore::Instance().Initalize<Logger::OutputDebugWindow>();
	HRESULT hr = E_FAIL;
	int i = 10;
	LOG_INFO(hr,false);
	LOG_INFO(hr,false);
	LOG_INFO(hr,false);
	return 0;
}