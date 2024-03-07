#pragma once

#include "rir_config.h"
#include <string>
#include <mutex>

namespace rir
{
	/**
	A thread safe file locking class for interprocess synchronization.
	Only works for Linux.
	On windows, simply wrapps a mutex.
	*/
	class TOOLS_EXPORT FileLock
	{
		int m_fd;
		std::string m_name;
		std::mutex m_mutex;

	public:
		explicit FileLock(const char *fname) : m_fd(-1), m_name(fname) {}
		explicit FileLock(const std::string &fname) : m_fd(-1), m_name(fname) {}
		~FileLock();

		void lock();
		void unlock();
		bool tryLock();
	};

	/**
	Lock guard for FileLock
	*/
	class TOOLS_EXPORT FileLockGuard
	{
		FileLock *m_lock;

	public:
		FileLockGuard(FileLock &lock);
		~FileLockGuard();
	};

}