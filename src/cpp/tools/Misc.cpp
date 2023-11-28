#include "Misc.h"

#include <fstream>
#include <string>
#include <iostream>
#include <clocale>
#include <locale>
#include <fstream>

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h> // errno, ENOENT, EEXIST
#if defined(_WIN32) || defined(_MSC_VER)
#include <direct.h> // _mkdir
#include <Windows.h>
#include <io.h>
#endif

#ifdef _MSC_VER
#include <conio.h>
#define access_fun _access
// Copied from linux libc sys/stat.h:
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#else
#include <unistd.h>
#include <ftw.h>
#define access_fun access
#endif

#ifndef _MSC_VER

#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#else

#define WIN32_LEAN_AND_MEAN
#include <stdint.h> // portable: uint64_t   MSVC: __int64

#endif

/*
#include <codecvt>

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

*/

namespace rir
{

	std::wstring s2ws(const std::string &str)
	{
		return std::wstring(str.begin(), str.end());
	}

	std::string ws2s(const std::wstring &wstr)
	{
		std::string res(wstr.size(), (char)0);
		std::transform(wstr.begin(), wstr.end(), res.begin(), [](wchar_t c)
					   { return static_cast<char>(c); });
		return res;
	}

	// returns count of non-overlapping occurrences of 'sub' in 'str'
	int count_substring(const std::string &str, const std::string &sub)
	{
		if (sub.length() == 0)
			return 0;
		int count = 0;
		for (size_t offset = str.find(sub); offset != std::string::npos;
			 offset = str.find(sub, offset + sub.length()))
		{
			++count;
		}
		return count;
	}

	int replace(std::string &in, const char *_from, const char *_to)
	{
		std::string &str = in;
		std::string from = _from;
		std::string to = _to;
		int res = 0;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
			++res;
		}
		return res;
	}

	StringList split(const std::string &in, const char *_match, bool keep_empty_strings)
	{
		StringList res;
		std::string match = _match;
		size_t start_pos = 0;
		size_t previous_pos = 0;
		while ((start_pos = in.find(match, start_pos)) != std::string::npos)
		{
			// str.replace(start_pos, from.length(), to);
			if (previous_pos != start_pos || keep_empty_strings)
			{
				res.push_back(in.substr(previous_pos, start_pos - previous_pos));
			}
			start_pos += match.length();
			previous_pos = start_pos;
		}

		if (previous_pos != in.size() || keep_empty_strings)
			res.push_back(in.substr(previous_pos));
		return res;
	}

	StringList split(const char *in, const char *match, bool keep_empty_strings)
	{
		return split(std::string(in), match, keep_empty_strings);
	}

	std::string join(const StringList &lst, const char *str)
	{
		std::string res;
		if (lst.empty())
			return res;

		res = lst.front();
		for (size_t i = 1; i < lst.size(); ++i)
			res += str + lst[i];
		return res;
	}

	static bool compare_case_insensitive(const char *str1, const char *str2, size_t size)
	{
		for (size_t i = 0; i < size; ++i)
			if (std::tolower(str1[i]) != std::tolower(str2[i]))
				return false;
		return true;
	}
	size_t find_case_insensitive(size_t start, const char *str, size_t size, const char *sub_str, size_t sub_size)
	{
		if (sub_size == 0)
			return std::string::npos;
		for (size_t i = start; i < size; ++i)
		{
			if (std::tolower(str[i]) == std::tolower(sub_str[0]))
			{
				// compare remaining
				if (size - i < sub_size)
					return std::string::npos;
				if (compare_case_insensitive(str + i + 1, sub_str + 1, sub_size - 1))
					return i;
			}
		}
		return std::string::npos;
	}

	bool file_exists(const char *filename)
	{
		if (!filename)
			return false;
		/*
		// Check for existence
		if ((access_fun(filename, 0)) != -1)
			return 1;
		else
			return 0;*/

		/*#ifdef _MSC_VER
			std::ifstream fin(filename);
			return !fin.fail();
		#else*/

		// on linux, opening a directory might return a valid file descriptor!
		FILE *f = fopen(filename, "r");
		if (!f)
			return false;
		else
		{
			bool res = fseek(f, 0, SEEK_END) == 0;
			fclose(f);
			return res && !dir_exists(filename);
		}
		// #endif
	}

	size_t file_size(const char *filename)
	{
		if (!file_exists(filename))
			return 0;
		std::ifstream fin(filename, std::ios::binary);
		if (!fin)
			return 0;
		fin.seekg(0, std::ios::end);
		return fin.tellg();
	}

	std::string read_file(const char *filename, bool *ok)
	{
		size_t fsize = file_size(filename);
		std::ifstream fin(filename, std::ios::binary);
		if (!fin)
		{
			if (ok)
				*ok = false;
			return std::string();
		}
		if (ok)
			*ok = true;
		std::string res(fsize, (char)0);
		fin.read((char *)res.data(), fsize);
		return res;
	}

	bool dir_exists(const char *dirname)
	{
		if (!dirname)
			return 0;

		struct stat sb;

		return (stat(dirname, &sb) == 0 && S_ISDIR(sb.st_mode));
	}

#ifndef _MSC_VER

	int unlink_cb(const char *fpath, const struct stat * /*sb*/, int /*typeflag*/, struct FTW * /*ftwbuf*/)
	{
		int rv = remove(fpath);

		if (rv)
			perror(fpath);

		return rv;
	}

	bool rm_file_or_dir(const char *path)
	{
		if (file_exists(path))
			return remove(path) == 0;
		else if (dir_exists(path))
			return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS) == 0;
		else
			return false;
	}

#else

	static std::wstring fromStr(const std::string &str)
	{
		std::wstring w(str.begin(), str.end());
		return w;
	}

	bool rm_file_or_dir(const char *path)
	{
		if (file_exists(path))
			return remove(path) == 0;
		else if (!dir_exists(path))
			return false;

		bool bSubdirectory = false;		 // Flag, indicating whether
										 // subdirectories have been found
		HANDLE hFile;					 // Handle to directory
		std::string strFilePath;		 // Filepath
		std::string strPattern;			 // Pattern
		WIN32_FIND_DATA FileInformation; // File information
		std::string refcstrRootDirectory = path;

		strPattern = refcstrRootDirectory + "\\*.*";
		hFile = ::FindFirstFile(s2ws(strPattern).c_str(), &FileInformation);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (FileInformation.cFileName[0] != '.')
				{
					strFilePath.erase();
					strFilePath = refcstrRootDirectory + "\\" + ws2s(FileInformation.cFileName);

					if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// Delete subdirectory
						bool iRC = rm_file_or_dir(strFilePath.c_str());
						if (!iRC)
							return iRC;
					}
					else
					{
						// Set file attributes
						if (::SetFileAttributes(s2ws(strFilePath).c_str(),
												FILE_ATTRIBUTE_NORMAL) == FALSE)
							return false;

						// Delete file
						if (::DeleteFile(s2ws(strFilePath).c_str()) == FALSE)
							return false; // ::GetLastError();
					}
				}
			} while (::FindNextFile(hFile, &FileInformation) == TRUE);

			// Close handle
			::FindClose(hFile);

			DWORD dwError = ::GetLastError();
			if (dwError != ERROR_NO_MORE_FILES)
				return false; // dwError;
			else
			{
				if (!bSubdirectory)
				{
					// Set directory attributes
					if (::SetFileAttributes(s2ws(refcstrRootDirectory).c_str(),
											FILE_ATTRIBUTE_NORMAL) == FALSE)
						return false; // ::GetLastError();

					// Delete directory
					if (::RemoveDirectory(s2ws(refcstrRootDirectory).c_str()) == FALSE)
						return false; // ::GetLastError();
				}
			}
		}

		return true;
	}

#endif

	static bool isDirExist(const std::string &path)
	{
#if defined(_WIN32)
		struct _stat info;
		if (_stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		return (info.st_mode & _S_IFDIR) != 0;
#else
		struct stat info;
		if (stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		return (info.st_mode & S_IFDIR) != 0;
#endif
	}

	bool make_path(const char *_path)
	{
		std::string path = _path;
#if defined(_WIN32)
		int ret = _mkdir(path.c_str());
#else
		mode_t mode = 0755;
		int ret = mkdir(path.c_str(), mode);
#endif
		if (ret == 0)
			return true;

		switch (errno)
		{
		case ENOENT:
			// parent didn't exist, try to create it
			{
				std::size_t pos = path.find_last_of('/');
				if (pos == std::string::npos)
#if defined(_WIN32)
					pos = path.find_last_of('\\');
				if (pos == std::string::npos)
#endif
					return false;
				if (!make_path(path.substr(0, pos).c_str()))
					return false;
			}
			// now, try to create again
#if defined(_WIN32)
			return 0 == _mkdir(path.c_str());
#else
			return 0 == mkdir(path.c_str(), mode);
#endif

		case EEXIST:
			// done!
			return isDirExist(path);

		default:
			return false;
		}
	}

	int rename_file(const char *old, const char *_new)
	{
#ifdef WIN32
		return rename(old, _new);
#else
		if (!file_exists(_new))
			// rename is a POSIX function, however it does not work when copying local file to another filesystem, while mv works (copy + delete)
			return std::system(("mv -f \"" + std::string(old) + "\" \"" + std::string(_new) + "\"").c_str());
		return -1;
#endif
	}

	std::string format_file_path(const char *fname)
	{
		std::string res = fname;
		replace(res, "\\", "/");
		while (res.size() > 0 && res[res.size() - 1] == '/')
			res = res.substr(0, res.size() - 1);
		return res;
	}

	std::string format_dir_path(const char *dname)
	{
		std::string res = dname;
		replace(res, "\\", "/");
		if (res[res.size() - 1] != '/')
			res += "/";
		return res;
	}

#ifndef _MSC_VER

	void msleep(int msecs)
	{
		usleep(msecs * 1000);
	}
	long long msecs_since_epoch()
	{
		struct timeval tp;
		gettimeofday(&tp, NULL);
		long long ms = tp.tv_sec * 1000LL + tp.tv_usec / 1000LL;
		return ms;
	}

#else

	/**Truncate given file*/
	int truncate(const char *fname, size_t off)
	{
		FILE *f = fopen(fname, "ab");
		if (!f)
			return -1;
		int r = _chsize_s(_fileno(f), off);
		fclose(f);
		return r;
	}

	void msleep(int msecs)
	{
		Sleep(msecs);
	}

	// MSVC defines this in winsock2.h!?
	typedef struct timeval_s
	{
		long tv_sec;
		long tv_usec;
	} timeval_s;

	int gettimeofday(struct timeval_s *tp, struct timezone *)
	{
		// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
		// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
		// until 00:00:00 January 1, 1970
		static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

		SYSTEMTIME system_time;
		FILETIME file_time;
		uint64_t time;

		GetSystemTime(&system_time);
		SystemTimeToFileTime(&system_time, &file_time);
		time = ((uint64_t)file_time.dwLowDateTime);
		time += ((uint64_t)file_time.dwHighDateTime) << 32;

		tp->tv_sec = (long)((time - EPOCH) / 10000000L);
		tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
		return 0;
	}

	long long msecs_since_epoch()
	{
		struct timeval_s tp;
		gettimeofday(&tp, NULL);
		long long ms = tp.tv_sec * 1000LL + tp.tv_usec / 1000LL;
		return ms;
	}

#endif

	FileFloatStream::FileFloatStream(const char *filename, const char *format)
		: file(NULL), string(NULL), len(0), pos(0)
	{
		file = fopen(filename, format);
		if (file)
		{
			fseek(file, 0, SEEK_END);
			len = ftell(file);
			fseek(file, 0, SEEK_SET);
		}
	}
	FileFloatStream::FileFloatStream(const char *str, size_t len)
		: file(NULL), string(NULL), len(len), pos(0)
	{
		if (len && str)
			this->string = str;
	}

	FileFloatStream::~FileFloatStream()
	{
		close();
	}

	void FileFloatStream::close()
	{
		if (file)
			fclose(file);
		file = (NULL);
		string = (NULL);
		len = (0);
		pos = (0);
	}

	bool FileFloatStream::open(const char *filename, const char *format)
	{
		close();
		file = fopen(filename, format);
		if (file)
		{
			fseek(file, 0, SEEK_END);
			len = ftell(file);
			fseek(file, 0, SEEK_SET);
			return true;
		}
		return false;
	}
	bool FileFloatStream::open(const char *str, size_t len)
	{
		close();
		if (len && str)
		{
			this->string = str;
			return true;
		}
		return false;
	}

	bool FileFloatStream::isOpen() const
	{
		return file || string;
	}
	size_t FileFloatStream::size() const
	{
		return len;
	}

	size_t FileFloatStream::tell() const
	{
		return pos;
	}

	bool FileFloatStream::seek(size_t p)
	{
		if (p >= len)
			return false;
		pos = p;
		return true;
	}

	std::string FileFloatStream::readAll()
	{
		if (pos >= len)
			return std::string();
		else if (file)
		{
			fseek(file, (long)pos, SEEK_SET);
			std::string res(len - pos, (char)0);
			size_t read = fread((char *)res.data(), 1, res.size(), file);
			if (read > 0)
			{
				pos += read;
				return res.substr(0, read);
			}
			return std::string();
		}
		else if (string)
		{
			std::string res(string + pos, len - pos);
			pos = len;
			return res;
		}
		else
			return std::string();
	}

	std::string FileFloatStream::readLine()
	{
		if (!isOpen() || pos >= len)
			return std::string();
		else
		{
			if (file)
				fseek(file, (long)pos, SEEK_SET);
			size_t max_read = len - pos;
			std::string res;
			size_t i = 0;
			for (; i < max_read; ++i)
			{
				int c = file ? getc(file) : string[pos + i];
				if (c == EOF)
				{
					--i;
					break;
				}
				if (c == '\n' || c == '\r')
				{
					if (c == '\r' && pos < len)
						++pos; // windows EOL
					break;
				}
				res.push_back((char)c);
			}
			pos += (i + 1);
			return res;
		}
	}

}
