#ifndef LOG_H
#define LOG_H

#include "rir_config.h"

#define LOG_INFO_LEVEL 0
#define LOG_WARNING_LEVEL 1
#define LOG_ERROR_LEVEL 2

/** @file

Defines logging functions used within librir
*/

namespace rir
{

	/**Logging function type*/
	typedef void (*log_print_function)(int, const char *);

	/**Set the log function for librir*/
	TOOLS_EXPORT void set_log_function(log_print_function function);
	/**Disable logging*/
	TOOLS_EXPORT void disable_log();
	/**Returns the current log function*/
	TOOLS_EXPORT log_print_function log_function();
	/**Reset log function to the standard one which uses printf*/
	TOOLS_EXPORT void reset_log_function();

	/**Log an information*/
	TOOLS_EXPORT void logInfo(const char *text);
	/**Log a warning*/
	TOOLS_EXPORT void logWarning(const char *text);
	/**Log an error*/
	TOOLS_EXPORT void logError(const char *text);

	/**Retrieve last logged error.
	Returns -1 if given len is too low (len will be set to the right value).
	Returns 0 on success.*/
	TOOLS_EXPORT int getLastErrorLog(char *text, int *len);

}

#define RIR_LOG_INFO(...)                               \
	{                                                   \
		int size = std::snprintf(NULL, 0, __VA_ARGS__); \
		char *data = new char[size];                    \
		sprintf(data, __VA_ARGS__);                     \
		rir::logInfo(data);                             \
		delete[] data;                                  \
	}

#define RIR_LOG_WARNING(...)                            \
	{                                                   \
		int size = std::snprintf(NULL, 0, __VA_ARGS__); \
		char *data = new char[size];                    \
		sprintf(data, __VA_ARGS__);                     \
		rir::logWarning(data);                          \
		delete[] data;                                  \
	}

#define RIR_LOG_ERROR(...)                              \
	{                                                   \
		int size = std::snprintf(NULL, 0, __VA_ARGS__); \
		char *data = new char[size];                    \
		sprintf(data, __VA_ARGS__);                     \
		rir::logError(data);                            \
		delete[] data;                                  \
	}

#endif
