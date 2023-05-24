#include "FileLock.h"
#include "Misc.h"



#ifndef _WIN32 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>


/*! Try to get lock. Return its file descriptor or -1 if failed.
*
*  @param lockName Name of file used as lock (i.e. '/var/lock/myLock').
*  @return File descriptor of lock file, or -1 if failed.
*/
int tryGetLock(char const *lockName)
{
	mode_t m = umask(0);
	int fd = open(lockName, O_RDWR | O_CREAT, 0666);
	umask(m);
	if (fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) < 0)
	{
		close(fd);
		fd = -1;
	}
	return fd;
}

/*! Release the lock obtained with tryGetLock( lockName ).
*
*  @param fd File descriptor of lock returned by tryGetLock( lockName ).
*  @param lockName Name of file used as lock (i.e. '/var/lock/myLock').
*/
void releaseLock(int fd, char const *lockName)
{
	if (fd < 0)
		return;
	close(fd);
	remove(lockName);
}
#endif


namespace rir
{


FileLock::~FileLock()
{
}

void FileLock::lock()
{
#ifndef _WIN32
	while (!tryLock())
		msleep(5);
#else
	m_mutex.lock();
#endif
}
void FileLock::unlock()
{
#ifndef _WIN32
	releaseLock(m_fd, m_name.c_str());
#endif
	m_mutex.unlock();
}

bool FileLock::tryLock()
{
	if (!m_mutex.try_lock())
		return false;

#ifndef _WIN32
	m_fd = tryGetLock(m_name.c_str());
	if (m_fd < 0) {
		m_mutex.unlock();
		return false;
	}
#endif
	return true;
}




FileLockGuard::FileLockGuard(FileLock & lock)
	:m_lock(&lock)
{
	m_lock->lock();
}
FileLockGuard::~FileLockGuard()
{
	m_lock->unlock();
}

}