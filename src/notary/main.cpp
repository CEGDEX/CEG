#include <utils/headers.h>
#include <common/general.h>
#include <common/storage.h>
#include <common/private_key.h>
#include <common/argument.h>
#include <common/daemon.h>
#include "notary/notary_mgr.h"
#include <notary/configure.h>
#include <notary/http_client.h>


void SaveWSPort();
void RunLoop();

int main(int argc, char *argv[]){

#ifdef WIN32
	_set_output_format(_TWO_DIGIT_EXPONENT);
#else
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	size_t stacksize = 0;
	int ret = pthread_attr_getstacksize(&attr, &stacksize);
	if (ret != 0) {
		printf("get stacksize error!:%d\n", (int)stacksize);
		return -1;
	}

	if (stacksize <= 2 * 1024 * 1024)
	{
		stacksize = 2 * 1024 * 1024;

		pthread_attr_t object_attr;
		pthread_attr_init(&object_attr);
		ret = pthread_attr_setstacksize(&object_attr, stacksize);
		if (ret != 0) {
			printf("set main stacksize error!:%d\n", (int)stacksize);
			return -1;
		}
	}
#endif

	utils::SetExceptionHandle();
	utils::Thread::SetCurrentThreadName("CEG-thread");

	utils::Daemon::InitInstance();
	utils::net::Initialize();
	utils::Timer::InitInstance();
	CEG::Configure::InitInstance();
	CEG::Storage::InitInstance();
	CEG::Global::InitInstance();
	CEG::SlowTimer::InitInstance();
	utils::Logger::InitInstance();
	CEG::NotaryMgr::InitInstance();

	CEG::Argument arg;
	if (arg.Parse(argc, argv)){
		return 1;
	}

	do {
		utils::ObjectExit object_exit;
		CEG::InstallSignal();

		srand((uint32_t)time(NULL));
		CEG::StatusModule::modules_status_ = new Json::Value;
#ifndef OS_MAC
		utils::Daemon &daemon = utils::Daemon::Instance();
		if (!CEG::g_enable_ || !daemon.Initialize((int32_t)1234))
		{
			LOG_STD_ERRNO("Failed to initialize daemon", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&utils::Daemon::Exit, &daemon));
#endif

		CEG::Configure &config = CEG::Configure::Instance();
		std::string config_path = CEG::General::CONFIG_FILE;
		if (!utils::File::IsAbsolute(config_path)){
			config_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), config_path.c_str());
		}

		if (!config.Load(config_path)){
			printf("%s\n", config_path.c_str());
			LOG_STD_ERRNO("Failed to load configuration", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}

		std::string log_path = config.logger_configure_.path_;
		if (!utils::File::IsAbsolute(log_path)){
			log_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), log_path.c_str());
		}
		const CEG::LoggerConfigure &logger_config = CEG::Configure::Instance().logger_configure_;
		utils::Logger &logger = utils::Logger::Instance();
		logger.SetCapacity(logger_config.time_capacity_, logger_config.size_capacity_);
		logger.SetExpireDays(logger_config.expire_days_);
		if (!CEG::g_enable_ || !logger.Initialize((utils::LogDest)(arg.log_dest_ >= 0 ? arg.log_dest_ : logger_config.dest_),
			(utils::LogLevel)logger_config.level_, log_path, true)){
			LOG_STD_ERR("Failed to initialize logger");
			break;
		}
		object_exit.Push(std::bind(&utils::Logger::Exit, &logger));
		LOG_INFO("Initialized daemon successfully");
		LOG_INFO("Loaded configure successfully");
		LOG_INFO("Initialized logger successfully");

		// end run command
		CEG::Storage &storage = CEG::Storage::Instance();
		CEG::Global &global = CEG::Global::Instance();
		if (!CEG::g_enable_ || !global.Initialize()){
			LOG_ERROR_ERRNO("Failed to initialize global variable", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&CEG::Global::Exit, &global));
		LOG_INFO("Initialized global module successfully");


		CEG::SlowTimer &slow_timer = CEG::SlowTimer::Instance();
		if (!CEG::g_enable_ || !slow_timer.Initialize(1)){
			LOG_ERROR_ERRNO("Failed to initialize slow timer", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&CEG::SlowTimer::Exit, &slow_timer));
		LOG_INFO("Initialized slow timer with " FMT_SIZE " successfully", utils::System::GetCpuCoreCount());

		CEG::NotaryMgr &cross_mgr = CEG::NotaryMgr::Instance();
		if (!cross_mgr.Initialize()) {
			LOG_ERROR("Initialize cross manager failed");
			break;
		}
		object_exit.Push(std::bind(&CEG::NotaryMgr::Exit, &cross_mgr));
		LOG_INFO("Initialize cross manager successful");

		CEG::g_ready_ = true;

		RunLoop();

		LOG_INFO("Process begins to quit...");
		delete CEG::StatusModule::modules_status_;

	} while (false);

	CEG::NotaryMgr::ExitInstance();
	CEG::SlowTimer::ExitInstance();
	CEG::Configure::ExitInstance();
	CEG::Global::ExitInstance();
	CEG::Storage::ExitInstance();
	utils::Logger::ExitInstance();
	utils::Daemon::ExitInstance();
	
	if (arg.console_ && !CEG::g_ready_) {
		printf("Initialized failed, please check log for detail\n");
	}
	printf("process exit\n");
}

void RunLoop(){
	int64_t check_module_interval = 5 * utils::MICRO_UNITS_PER_SEC;
	int64_t last_check_module = 0;
	while (CEG::g_enable_){
		int64_t current_time = utils::Timestamp::HighResolution();

		for (auto item : CEG::TimerNotify::notifys_){
			item->TimerWrapper(utils::Timestamp::HighResolution());
			if (item->IsExpire(30 * utils::MICRO_UNITS_PER_SEC)){
				LOG_WARN("The execution time(" FMT_I64 " us) for the timer(%s) is expired after 30s elapses", item->GetLastExecuteTime(), item->GetTimerName().c_str());
			}
		}

		utils::Timer::Instance().OnTimer(current_time);
		utils::Logger::Instance().CheckExpiredLog();

		if (current_time - last_check_module > check_module_interval){
			utils::WriteLockGuard guard(CEG::StatusModule::status_lock_);
			CEG::StatusModule::GetModulesStatus(*CEG::StatusModule::modules_status_);
			last_check_module = current_time;
		}

		utils::Sleep(1);
	}
}
