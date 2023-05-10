#include "Log.h"

#include <cstdio>
#include <cstring>
#include <mutex>


namespace rir
{

	static void log_function(int level, const char* text)
	{
		if (level == LOG_INFO_LEVEL)
			printf("INFO: %s\n", text);
		else if (level == LOG_WARNING_LEVEL)
			printf("WARNING: %s\n", text);
		else if (level == LOG_ERROR_LEVEL)
			printf("ERROR: %s\n", text);
	}

	static void null_log_function(int, const char*)
	{}

	static log_print_function _log = log_function;



	void set_log_function(log_print_function function)
	{
		_log = function;
	}

	void disable_log()
	{
		_log = null_log_function;
	}

	log_print_function log_function()
	{
		return _log;
	}

	void reset_log_function()
	{
		_log = log_function;
	}

	void logInfo(const char* text)
	{
		if (_log)
			_log(LOG_INFO_LEVEL, text);
	}

	void logWarning(const char* text)
	{
		if (_log)
			_log(LOG_WARNING_LEVEL, text);
	}

	static std::string _last_log_error;
	static std::mutex _last_log_mutex;

	void logError(const char* text)
	{
		if (_log)
			_log(LOG_ERROR_LEVEL, text);

		std::lock_guard<std::mutex> lock(_last_log_mutex);
		_last_log_error = std::string(text);
	}

	int getLastErrorLog(char* text, int* len)
	{
		if (*len < (int)_last_log_error.size())
		{
			std::lock_guard<std::mutex> lock(_last_log_mutex);
			*len = (int)_last_log_error.size();
			return -1;
		}

		std::lock_guard<std::mutex> lock(_last_log_mutex);
		*len = (int)_last_log_error.size();
		memcpy(text, _last_log_error.data(), _last_log_error.size());
		return 0;
	}


}