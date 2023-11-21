#include <string>
#include <mutex>
#include <map>
#include <fstream>

extern "C"
{
#include "zstd.h"
#include "blosc.h"
#include "unzip.h"
#include <string.h>
}
#include "Misc.h"
#include "tools.h"
#include "Log.h"
#include "FileAttributes.h"

using namespace rir;

static std::string tools_get_temp_dir()
{
	std::string dirname;

#ifdef _MSC_VER
	char *tmp = std::getenv("TEMP");
	if (!tmp)
		tmp = std::getenv("TMP");
	if (!tmp)
		tmp = std::getenv("TMPDIR");
	if (!tmp)
	{
#ifdef P_tmpdir
		tmp = (char *)P_tmpdir;
#endif
	}

	dirname = tmp ? tmp : "";
	if (dirname.empty())
		dirname = "./west_data/";
	else
	{
		dirname = format_dir_path(dirname.c_str());
		dirname += "west_data/";
	}
#else
	// unix system

	// try to use the global thermavip user
	/*if (dir_exists("/Home/thermavip/west_data"))
		dirname = "/Home/thermavip/west_data/";
	else*/
	{
		std::string home = getenv("HOME");
		if (home.empty())
		{

			// if not, use the user's home directory
			dirname = "~/.west_data/";
		}
		else
		{
			if (home[home.size() - 1] != '/')
				home += "/";
			home += ".west_data/";
			dirname = home;
		}
	}

#endif
	replace(dirname, "\\", "/");

	if (!dir_exists(dirname.c_str()))
		if (!make_path(dirname.c_str()))
			return std::string();

	/*#ifdef _WIN32
		// On Windows only, create (if necessary) the SIGNALS directory that bufferize signals read with ts_read_signals and ts_read_group, as well as list of views for each pulse
		std::string signals_path = dirname + "SIGNALS/";
		if (!dir_exists(signals_path.c_str()))
			make_path(signals_path.c_str());
	#endif*/

	return dirname;
}

class ToolsTmpDir
{
public:
	std::string dirname;
	std::string default_dirname;
	ToolsTmpDir()
	{

		dirname = tools_get_temp_dir().c_str(); // +toString(msecs_since_epoch()) + "/";

		printf("tmp dir: %s\n", dirname.c_str());
		fflush(stdout);
		// int s2 = sizeof(dirname);

		if (!dir_exists(dirname.c_str()))
			if (!make_path(dirname.c_str()))
				dirname = "./west_data/";

		default_dirname = dirname;

		// Create the folder SIGNALS if it does not exist
		std::string signals_path = dirname + "SIGNALS/";
		if (!dir_exists(signals_path.c_str()))
			make_path(signals_path.c_str());
	}
	~ToolsTmpDir()
	{
	}
};

static ToolsTmpDir &tmpDir()
{
	static ToolsTmpDir tmp;
	return tmp;
}

int set_temp_directory(const char *dirname)
{
	if (!dir_exists(dirname))
		if (!make_path(dirname))
			return -1;

	std::string dir = dirname;
	replace(dir, "\\", "/");
	if (dir.back() != '/')
		dir += "/";
	tmpDir().dirname = dir;

	// #ifdef _WIN32
	std::string signals_path = dir + "SIGNALS/";
	if (!dir_exists(signals_path.c_str()))
		make_path(signals_path.c_str());
	// #endif

	return 0;
}

void get_temp_directory(char *dirname)
{
	sprintf(dirname, "%s", tmpDir().dirname.c_str());
}

void get_default_temp_directory(char *dirname)
{
	sprintf(dirname, "%s", tmpDir().default_dirname.c_str());
}

void set_print_function(print_function function)
{
	set_log_function(function);
}

void disable_print()
{
	disable_log();
}

void reset_print_functions()
{
	reset_log_function();
}

int get_last_log_error(char *text, int *len)
{
	return getLastErrorLog(text, len);
}

static std::map<int, void *> &map_void_ptr()
{
	static std::map<int, void *> inst;
	return inst;
}
static std::mutex &map_void_mutex()
{
	static std::mutex inst;
	return inst;
}

int set_void_ptr(void *cam)
{
	std::lock_guard<std::mutex> guard(map_void_mutex());
	if (!cam)
		return -1;
	int i = 1;
	std::map<int, void *>::const_iterator it = map_void_ptr().begin();
	for (; it != map_void_ptr().end(); ++it, ++i)
	{
		if (it->first != i)
			break;
	}
	if (it == map_void_ptr().end())
		i = (int)map_void_ptr().size() + 1;

	map_void_ptr()[i] = cam;
	return i;
}
void *get_void_ptr(int index)
{
	std::lock_guard<std::mutex> guard(map_void_mutex());
	std::map<int, void *>::iterator it = map_void_ptr().find(index);
	if (it != map_void_ptr().end())
		return it->second;
	return NULL;
}
void rm_void_ptr(int index)
{
	std::lock_guard<std::mutex> guard(map_void_mutex());
	std::map<int, void *>::iterator it = map_void_ptr().find(index);
	if (it != map_void_ptr().end())
		map_void_ptr().erase(it);
}

int attrs_read_file_reader(void *file_reader)
{
	FileAttributes *attrs = new FileAttributes();
	if (!attrs->openReadOnly(file_reader))
	{
		delete attrs;
		return 0;
	}
	return set_void_ptr(attrs);
}
int attrs_open_file(const char *filename)
{
	FileAttributes *attrs = new FileAttributes();
	if (!attrs->open(filename))
	{
		delete attrs;
		return 0;
	}
	return set_void_ptr(attrs);
}
void attrs_close(int handle)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return;
	attrs->close();
	delete attrs;
	rm_void_ptr(handle);
}
void attrs_discard(int handle)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return;
	attrs->close();
	delete attrs;
	rm_void_ptr(handle);
}
int attrs_flush(int handle)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	attrs->flush();
	return 0;
}
int attrs_image_count(int handle)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	return (int)attrs->size();
}
int attrs_global_attribute_count(int handle)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	return (int)attrs->globalAttributes().size();
}
int attrs_global_attribute_name(int handle, int pos, char *name, int *len)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (pos >= (int)attrs->globalAttributes().size())
		return -1;
	std::map<std::string, std::string>::const_iterator it = attrs->globalAttributes().begin();
	std::advance(it, pos);
	if (*len < (int)it->first.size())
	{
		*len = (int)it->first.size();
		return -2;
	}
	*len = (int)it->first.size();
	memcpy(name, it->first.c_str(), it->first.size());
	return 0;
}
int attrs_global_attribute_value(int handle, int pos, char *value, int *len)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (pos >= (int)attrs->globalAttributes().size())
		return -1;
	std::map<std::string, std::string>::const_iterator it = attrs->globalAttributes().begin();
	std::advance(it, pos);
	if (*len < (int)it->second.size())
	{
		*len = (int)it->second.size();
		return -2;
	}
	*len = (int)it->second.size();
	memcpy(value, it->second.c_str(), it->second.size());
	return 0;
}

int attrs_frame_attribute_count(int handle, int frame)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (frame >= (int)attrs->size())
		return -1;
	return (int)attrs->attributes(frame).size();
}
int attrs_frame_attribute_name(int handle, int frame, int pos, char *name, int *len)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (frame >= (int)attrs->size())
		return -1;

	const std::map<std::string, std::string> &attributes = attrs->attributes(frame);
	if (pos >= (int)attributes.size())
		return -1;

	std::map<std::string, std::string>::const_iterator it = attributes.begin();
	std::advance(it, pos);
	if (*len < (int)it->first.size())
	{
		*len = (int)it->first.size();
		return -2;
	}
	*len = (int)it->first.size();
	memcpy(name, it->first.c_str(), it->first.size());
	return 0;
}

int attrs_frame_attribute_value(int handle, int frame, int pos, char *value, int *len)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (frame >= (int)attrs->size())
		return -1;

	const std::map<std::string, std::string> &attributes = attrs->attributes(frame);
	if (pos >= (int)attributes.size())
		return -1;

	std::map<std::string, std::string>::const_iterator it = attributes.begin();
	std::advance(it, pos);
	if (*len < (int)it->second.size())
	{
		*len = (int)it->second.size();
		return -2;
	}
	*len = (int)it->second.size();
	memcpy(value, it->second.c_str(), it->second.size());
	return 0;
}

int attrs_frame_timestamp(int handle, int frame, int64_t *time)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;

	if (frame < 0 || frame >= (int)attrs->size())
		return -1;

	*time = attrs->timestamp(frame);
	return 0;
}

int attrs_timestamps(int handle, int64_t *times)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;

	for (size_t i = 0; i < attrs->size(); ++i)
		times[i] = attrs->timestamp(i);
	return 0;
}

int attrs_set_times(int handle, int64_t *times, int size)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (size < 0)
		return -1;
	if (size != (int)attrs->size())
		attrs->resize(size);
	for (int i = 0; i < size; ++i)
		attrs->setTimestamp(i, times[i]);
	return 0;
}

int attrs_set_time(int handle, int pos, int64_t time)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (pos < 0 || pos >= (int)attrs->size())
		return -1;

	attrs->setTimestamp(pos, time);
	return 0;
}

int attrs_set_frame_attributes(int handle, int pos, char *keys, int *key_lens, char *values, int *value_lens, int count)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	if (pos < 0 || pos >= (int)attrs->size())
		return -1;

	std::map<std::string, std::string> attributes;
	char *k = keys;
	char *v = values;
	for (int i = 0; i < count; ++i)
	{
		std::string key(key_lens[i], 0);
		memcpy((char *)key.data(), k, key_lens[i]);
		k += key_lens[i];

		std::string value(value_lens[i], 0);
		memcpy((char *)value.data(), v, value_lens[i]);
		v += value_lens[i];

		attributes.insert(std::pair<std::string, std::string>(key, value));
	}
	attrs->setAttributes(pos, attributes);
	return 0;
}

int attrs_set_global_attributes(int handle, char *keys, int *key_lens, char *values, int *value_lens, int count)
{
	FileAttributes *attrs = (FileAttributes *)get_void_ptr(handle);
	if (!attrs)
		return -1;
	// printf("start attrs_set_global_attributes\n");;fflush(stdout);
	std::map<std::string, std::string> attributes;
	char *k = keys;
	char *v = values;
	for (int i = 0; i < count; ++i)
	{
		std::string key(key_lens[i], 0);
		memcpy((char *)key.data(), k, key_lens[i]);
		k += key_lens[i];

		std::string value(value_lens[i], 0);
		memcpy((char *)value.data(), v, value_lens[i]);
		v += value_lens[i];

		attributes.insert(std::pair<std::string, std::string>(key, value));
	}
	// printf("try to set global attributes\n");fflush(stdout);
	attrs->setGlobalAttributes(attributes);
	// printf("done\n");fflush(stdout);
	return 0;
}

int64_t zstd_compress_bound(int64_t srcSize)
{
	return ZSTD_compressBound(srcSize);
}
int64_t zstd_decompress_bound(char *src, int64_t srcSize)
{
	size_t ret = ZSTD_getDecompressedSize(src, srcSize);
	if (ZSTD_isError(ret))
		return -1;
	return ret;
}
int64_t zstd_compress(char *src, int64_t srcSize, char *dst, int64_t dstSize, int level)
{
	size_t ret = ZSTD_compress(dst, dstSize, src, srcSize, level);
	if (ZSTD_isError(ret))
		return -1;
	return ret;
}
int64_t zstd_decompress(char *src, int64_t srcSize, char *dst, int64_t dstSize)
{
	size_t ret = ZSTD_decompress(dst, dstSize, src, srcSize);
	if (ZSTD_isError(ret))
		return -1;
	return ret;
}

static int init_blosc()
{
	blosc_init();
	blosc_set_nthreads(1);
	blosc_set_compressor("zstd");
	return 0;
}
static int _init_blosc = init_blosc();

int64_t blosc_compress_zstd(char *src, int64_t srcSize, char *dst, int64_t dstSize, size_t typesize, int doshuffle, int level)
{
	return blosc_compress(level, doshuffle, typesize, srcSize, src, dst, dstSize);
}

int64_t blosc_decompress_zstd(char *src, int64_t, char *dst, int64_t dstSize)
{
	return blosc_decompress(src, dst, dstSize);
}

int unzip(const char *infile, const char *outfolder)
{
	int count = 0;
	if (!rir::dir_exists(outfolder))
		if (!make_path(outfolder))
			return -1;

	std::string fin = infile;
	rir::replace(fin, "\\", "/");
	if (!rir::file_exists(fin.c_str()))
		return -2;

	std::string fout = outfolder;
	rir::replace(fout, "\\", "/");
	if (fout.back() != '/')
		fout += "/";

	unzFile file = unzOpen(fin.c_str());
	if (!file)
		return false;

	unz_global_info global_info;
	unzGetGlobalInfo(file, &global_info);

	if (unzGoToFirstFile(file) == UNZ_OK)
	{
		unz_file_info f_info;
		char name[1000];
		memset(name, 0, sizeof(name));
		while (unzGetCurrentFileInfo(file, &f_info, name, sizeof(name), NULL, 0, NULL, 0) == UNZ_OK)
		{
			std::string filename(name);
			if (filename.back() == '/')
			{
				// create the directory
				if (!make_path((fout + filename).c_str()))
				{
					unzClose(file);
					return -1;
				}
			}
			else
			{
				// read the compressed file and write it to the output file

				if (unzOpenCurrentFile(file) != UNZ_OK)
				{
					unzClose(file);
					return -3;
				}

				std::string ar(f_info.uncompressed_size, 0);
				int copied = unzReadCurrentFile(file, (char *)ar.data(), ar.size());
				if (copied == ar.size())
				{
					std::ofstream out((fout + filename).c_str(), std::ios::out | std::ios::binary);
					if (out)
						out.write(ar.data(), ar.size());
					else
						return -4;
				}
				else
				{
					unzClose(file);
					return -3;
				}

				unzCloseCurrentFile(file);
				++count;
			}

			int next = unzGoToNextFile(file);
			if (next == UNZ_END_OF_LIST_OF_FILE)
				break;
			else if (next != UNZ_OK)
			{
				unzClose(file);
				return -3;
			}
		}
	}

	unzClose(file);
	return count;
}
