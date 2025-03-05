#include "FileAttributes.h"
#include "Misc.h"
#include "ReadFileChunk.h"

#include <fstream>
#include <set>
#include <thread>
#include <mutex>

extern "C"
{
#include "zstd.h"
#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>

	static size_t linux_getpid()
	{
		return ::getpid();
	}
#endif
}

#define MIN_SIZE_FOR_COMRPESSION 1000

static inline char* mgetenv(const char* name)
{
#if defined( _MSC_VER) || defined(__MINGW32__)
	static thread_local char buf[4096];
	int len = (int)GetEnvironmentVariableA(name, buf, sizeof(buf) - 1);
	if (len == 0) {
		return nullptr;
	}
	else {
		if ((size_t)len < sizeof(buf) - 1) {
			buf[len] = 0;
			return buf;
		}
		return nullptr;
	}
#else
	return std::getenv(name);
#endif
}

namespace rir
{

	static bool is_compressed(std::uint64_t size)
	{
		return ((size) >> (sizeof(std::uint64_t) * 8 - 1)) != 0;
	}
	static size_t removeCompressFlag(std::uint64_t size)
	{
		return size & ~(1ull << (sizeof(std::uint64_t) * 8 - 1));
	}

	static std::string compress(const std::string &in, std::uint64_t &out_size)
	{
		if (in.size() < MIN_SIZE_FOR_COMRPESSION)
		{
			out_size = in.size();
			return in;
		}
		std::vector<char> out(ZSTD_compressBound(in.size()) + sizeof(std::uint64_t));
		std::uint64_t c = ZSTD_compress(out.data() + sizeof(std::uint64_t), out.size() - sizeof(std::uint64_t), in.c_str(), in.size(), 0);
		if (c < in.size())
		{
			out_size = c + 8;
			out_size |= 1ull << (sizeof(std::uint64_t) * 8 - 1);
			// copy uncompressed size to out
			std::uint64_t tmp = in.size();
			if (!is_little_endian())
				tmp = swap_uint64(tmp);
			memcpy(out.data(), &tmp, sizeof(tmp));
			return std::string(out.data(), out.data() + c + 8);
		}
		else
		{
			out_size = in.size();
			return in;
		}
	}

	static void write_size_t(std::uint64_t value, std::ostream &oss)
	{
		uint64_t s = value;
		if (!is_little_endian())
			s = swap_uint64(s);
		oss.write((char *)&s, sizeof(s));
	}
	static std::uint64_t read_size_t(std::istream &iss)
	{
		uint64_t v;
		iss.read((char *)&v, sizeof(v));
		if (!is_little_endian())
			v = swap_uint64(v);
		return v;
	}

	static void write_string(const std::string &str, std::ostream &oss)
	{
		std::uint64_t size;
		std::string tmp = compress(str, size);

		write_size_t(size, oss);
		oss.write(tmp.c_str(), tmp.size());
	}
	static std::string read_string(std::istream &iss)
	{
		std::uint64_t s = read_size_t(iss);
		bool compressed = is_compressed(s);
		s = removeCompressFlag(s);
		if (!compressed)
		{
			char *c = new char[s];
			iss.read(c, s);
			std::string res(c, c + s);
			delete[] c;
			return res;
		}
		else
		{
			// read uncompressed size
			std::uint64_t csize;
			iss.read((char *)&csize, sizeof(csize));
			if (!is_little_endian())
				csize = swap_uint64(csize);
			char *c = new char[s - sizeof(csize)];
			iss.read(c, s - sizeof(csize));
			std::string res(csize, (char)0);
			std::uint64_t r = ZSTD_decompress((void *)res.c_str(), res.size(), c, s - sizeof(csize));
			if (r != csize)
			{
				// error
				res.clear();
			}
			delete[] c;
			return res;
		}
	}

	static void write_map(const std::map<std::string, std::string> &m, std::ostream &oss)
	{
		write_size_t((std::uint64_t)m.size(), oss);
		for (std::map<std::string, std::string>::const_iterator it = m.begin(); it != m.end(); ++it)
		{
			write_string(it->first, oss);
			write_string(it->second, oss);
		}
	}
	static std::map<std::string, std::string> read_map(std::istream &iss)
	{
		std::map<std::string, std::string> res;
		std::uint64_t size = read_size_t(iss);
		for (size_t i = 0; i < size; ++i)
		{
			std::string key = read_string(iss);
			std::string value = read_string(iss);
			res[key] = value;
		}
		return res;
	}

	static std::string temp_dir()
	{
		std::string dirname;

#ifdef _MSC_VER
		char *tmp = mgetenv("TEMP");
		if (!tmp)
			tmp = mgetenv("TMP");
		if (!tmp)
			tmp = mgetenv("TMPDIR");
		if (!tmp)
		{
#ifdef P_tmpdir
			tmp = (char *)P_tmpdir;
#endif
		}

		dirname = tmp ? tmp : "/";
		if (dirname.back() != '/')
			dirname += "/";
#else
		// unix system

		char *env = getenv("HOME");
		std::string home; // = getenv("HOME");
		if (env)
			home = env;
		if (home.empty())
		{

			// if not, use the user's home directory
			dirname = "~/.tmp/";
		}
		else
		{
			if (home[home.size() - 1] != '/')
				home += "/";
			home += ".tmp/";
			dirname = home;
		}

#endif

		if (!dir_exists(dirname.c_str()))
			if (!make_path(dirname.c_str()))
				return std::string();
		return dirname;
	}

	class FileAttributes::PrivateData
	{
	public:
		std::vector<int64_t> timestamps;
		std::vector<std::map<std::string, std::string>> attributes;
		std::map<std::string, std::string> globalAttributes;
		std::string filename;
		size_t tableSize;
		size_t fileTableSize;
		PrivateData() : tableSize(0), fileTableSize(0) {}
	};

	FileAttributes::FileAttributes()
	{
		m_data = new PrivateData();
	}
	FileAttributes::~FileAttributes()
	{
		close();
		delete m_data;
	}

	static int64_t get_pid()
	{
		// static int64_t pid = -1;
		// if (pid < 0) {
#ifdef _MSC_VER
		return GetCurrentProcessId();
#else
		return linux_getpid();
#endif
		//}
		// return pid;
	}
	bool FileAttributes::openReadOnly(void *file_access)
	{
		close();
		size_t pos = posFile(file_access);
		size_t fsize = fileSize(file_access);
		if (seekFile(file_access, fsize - (16 + strlen(TABLE_TRAILER)), AVSEEK_SET) < 0)
			return 0;
		uint64_t frame_count, trailer_size;
		readFile(file_access, &frame_count, 8);
		readFile(file_access, &trailer_size, 8);
		if (!is_little_endian())
			trailer_size = swap_uint64(trailer_size);
		char trailer[64];
		memset(trailer, 0, sizeof(trailer));
		readFile(file_access, trailer, (int)strlen(TABLE_TRAILER));
		trailer[strlen(TABLE_TRAILER)] = 0;
		int ok = strcmp(trailer, TABLE_TRAILER);
		if (ok != 0)
		{
			seekFile(file_access, pos, AVSEEK_SET);
			return false;
		}

		// lock this part as we are working with tmp files, and that might create collisions between processes/threads

		// Use thread id for filename as it might (?) be unique between processes
		std::string file = temp_dir() + toString(get_pid()) + toString(std::this_thread::get_id()); //"tmp_file_attributes";//toString( msecs_since_epoch());
		std::ofstream off(file.c_str(), std::ios::binary);
		if (!off)
		{
			seekFile(file_access, pos, AVSEEK_SET);
			return false;
		}
		std::vector<char> buff(trailer_size);
		seekFile(file_access, fsize - trailer_size, AVSEEK_SET);
		if (readFile(file_access, buff.data(), (int)trailer_size) != (int)trailer_size)
		{
			seekFile(file_access, pos, AVSEEK_SET);
			off.close();
			remove(file.c_str());
			return false;
		}
		seekFile(file_access, pos, AVSEEK_SET);
		off.write(buff.data(), buff.size());
		if (!off)
		{
			off.close();
			remove(file.c_str());
			return false;
		}
		off.close();
		if (!open(file.c_str()))
		{
			remove(file.c_str());
			return false;
		}
		remove(file.c_str());
		return true;
	}

	bool FileAttributes::open(const char *filename)
	{
		close();

		m_data->filename = filename;
		size_t fsize = file_size(filename);

		if (fsize >= 16 + strlen(TABLE_TRAILER))
		{
			// Read current trailer
			std::ifstream iff(filename, std::ios::binary | std::ios::app);
			if (!iff)
				return false;

			iff.seekg(fsize - (16 + strlen(TABLE_TRAILER)));

			size_t frame_count = read_size_t(iff);
			size_t trailer_size = read_size_t(iff);
			char trailer[64];
			memset(trailer, 0, sizeof(trailer));
			iff.read(trailer, strlen(TABLE_TRAILER));
			trailer[strlen(TABLE_TRAILER)] = 0;
			int ok = strcmp(trailer, TABLE_TRAILER);

			if (ok == 0)
			{
				// Right format
				m_data->timestamps.resize(frame_count);
				m_data->attributes.resize(frame_count);

				// go to beginning of trailer
				iff.seekg(fsize - trailer_size);

				// read global attributes
				m_data->globalAttributes = read_map(iff);

				// read frame attributes
				for (size_t i = 0; i < m_data->attributes.size(); ++i)
					m_data->attributes[i] = read_map(iff);

				// read timestamps
				for (size_t i = 0; i < m_data->timestamps.size(); ++i)
					m_data->timestamps[i] = read_size_t(iff);

				m_data->fileTableSize = trailer_size;
				m_data->tableSize = trailer_size;
			}
			else
			{
				// no trailer, just return
			}
			return true;
		}
		else
		{
			// just create the file
			std::ofstream fout(filename);
			if (!fout)
				return false;
			return true;
		}
	}
	void FileAttributes::close()
	{
		writeIfDirty();
		m_data->tableSize = m_data->fileTableSize = 0;
		m_data->filename.clear();
		m_data->timestamps.clear();
		m_data->attributes.clear();
		m_data->globalAttributes.clear();
	}
	void FileAttributes::discard()
	{
		m_data->tableSize = m_data->fileTableSize = 0;
		m_data->filename.clear();
		m_data->timestamps.clear();
		m_data->attributes.clear();
		m_data->globalAttributes.clear();
	}
	void FileAttributes::flush()
	{
		writeIfDirty();
	}
	bool FileAttributes::isOpen() const
	{
		return m_data->filename.size() > 0;
	}
	size_t FileAttributes::tableSize() const
	{
		const_cast<FileAttributes *>(this)->writeIfDirty();
		return m_data->tableSize;
	}

	size_t FileAttributes::size() const
	{
		return m_data->timestamps.size();
	}
	void FileAttributes::resize(size_t size)
	{
		m_data->tableSize = 0;
		m_data->timestamps.resize(size);
		m_data->attributes.resize(size);
	}

	const std::map<std::string, std::string> &FileAttributes::globalAttributes() const
	{
		return m_data->globalAttributes;
	}
	void FileAttributes::setGlobalAttributes(const std::map<std::string, std::string> &attributes)
	{
		m_data->tableSize = 0;
		m_data->globalAttributes = attributes;
	}
	void FileAttributes::addGlobalAttribute(const std::string &key, const std::string &value)
	{
		m_data->tableSize = 0;
		m_data->globalAttributes[key] = value;
	}

	int64_t FileAttributes::timestamp(size_t index) const
	{
		return m_data->timestamps[index];
	}
	void FileAttributes::setTimestamp(size_t index, int64_t time)
	{
		m_data->tableSize = 0;
		m_data->timestamps[index] = time;
	}

	const std::map<std::string, std::string> &FileAttributes::attributes(size_t index) const
	{
		return m_data->attributes[index];
	}
	void FileAttributes::setAttributes(size_t index, const std::map<std::string, std::string> &attributes)
	{
		m_data->tableSize = 0;
		m_data->attributes[index] = attributes;
	}
	void FileAttributes::addAttribute(size_t index, const std::string &key, const std::string &value)
	{
		m_data->tableSize = 0;
		m_data->attributes[index][key] = value;
	}

	void FileAttributes::writeIfDirty()
	{
		if (m_data->tableSize != 0)
			return;
		if (m_data->filename.empty())
			return;

		// write full table
		std::ostringstream str;

		// write global attributes
		write_map(m_data->globalAttributes, str);

		// write frame attributes
		for (size_t i = 0; i < m_data->attributes.size(); ++i)
			write_map(m_data->attributes[i], str);

		// write timestamps
		for (size_t i = 0; i < m_data->timestamps.size(); ++i)
			write_size_t(m_data->timestamps[i], str);

		// write number of frames
		write_size_t(size(), str);

		// write global trailer size
		write_size_t(m_data->tableSize = str.str().size() + strlen(TABLE_TRAILER) + 8, str);

		// write table trailer
		str.write(TABLE_TRAILER, strlen(TABLE_TRAILER));

		size_t fsize = file_size(m_data->filename.c_str());

		// truncate file if necessary
		if (m_data->tableSize < m_data->fileTableSize)
		{
			if (truncate(m_data->filename.c_str(), fsize - m_data->fileTableSize + m_data->tableSize) != 0)
			{
				m_data->tableSize = 0;
				return;
			}
		}

		// add to file
		std::fstream fout(m_data->filename.c_str(), std::ios::in | std::ios::out | std::ios::binary);
		if (!fout)
		{
			m_data->tableSize = 0;
			return;
		}

		// seek to end
		fout.seekp(fsize - m_data->fileTableSize);

		// write trailer
		std::string trailer = str.str();
		fout.write(trailer.c_str(), trailer.size());
		fout.close();

		m_data->fileTableSize = m_data->tableSize;
	}

}
