#pragma once

#include "rir_config.h"

#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#ifndef _MSC_VER
/**
truncate function already available on all unix systems
*/
extern "C"
{
	#include <unistd.h>
}
#endif

#undef close

	/** @file

	Defines utility functions used within librir
	*/

	namespace rir
	{

		/**Vector of strings*/
		typedef std::vector<std::string> StringList;
		/**Vector of timestamps, usually in nanoseconds*/
		typedef std::vector<std::int64_t> TimestampVector;
		/**String to wide string conversion*/
		std::wstring s2ws(const std::string &str);
		/**Wide string to string conversion*/
		std::string ws2s(const std::wstring &wstr);
		/**
		Retrieve any kind of object from a string.
		Returns T(0) on error.
		If ok is not NULL, it will be set to true on success, false otherwise.
		*/
		template <class T>
		T fromString(const std::string &str, bool *ok = NULL)
		{
			std::istringstream iss(str);
			T res;
			if (iss >> res)
			{
				if (ok)
					*ok = true;
				return res;
			}
			if (ok)
				*ok = false;
			return T(0);
		}

		/**
		Convert any kind of object to its string representation.
		*/
		template <class T>
		std::string toString(const T &value)
		{
			std::ostringstream oss;
			oss << value;
			return oss.str();
		}
		/**Returns number of non-overlapping occurrences of \a sub in \a str.*/
		TOOLS_EXPORT int count_substring(const std::string &str, const std::string &sub);
		/**Replace all occurrences of \a str in \a in by \a replace*/
		TOOLS_EXPORT int replace(std::string &in, const char *str, const char *replace);
		/**Split \a in based on \a match separator.*/
		TOOLS_EXPORT StringList split(const std::string &in, const char *match, bool keep_empty_strings = false);
		/**Split \a in based on \a match separator.*/
		TOOLS_EXPORT StringList split(const char *in, const char *match, bool keep_empty_strings = false);
		/**Join list of string with \a str separator*/
		TOOLS_EXPORT std::string join(const StringList &lst, const char *str);
		/** Find substring using case insensitive comparison. Returns std::string::npos if not found. */
		TOOLS_EXPORT size_t find_case_insensitive(size_t start, const char *str, size_t size, const char *sub_str, size_t sub_size);

		/**Returns file size (0 if it does not exist)*/
		TOOLS_EXPORT size_t file_size(const char *filename);
		/**Returns full file content in binary format.*/
		TOOLS_EXPORT std::string read_file(const char *filename, bool *ok = NULL);
		/**Returns true if given file exists*/
		TOOLS_EXPORT bool file_exists(const char *filename);
		/**Returns true if given directory exists*/
		TOOLS_EXPORT bool dir_exists(const char *dirname);
		/**Remove given file or directory (recursively)*/
		TOOLS_EXPORT bool rm_file_or_dir(const char *filename);
		/**Attempt to create directory, and create all parents if missing*/
		TOOLS_EXPORT bool make_path(const char *path);
		/**Rename file, returns 0 on success */
		TOOLS_EXPORT int rename_file(const char *old, const char *_new);

		/**
		Format given file path by:
		1 - replacing '\' by '/',
		2 - removing any trailing slash.
		*/
		TOOLS_EXPORT std::string format_file_path(const char *fname);
		/**
		Format given directory path by:
		1 - replacing '\' by '/',
		2 - adding a trailing slash if necessary.
		*/
		TOOLS_EXPORT std::string format_dir_path(const char *dname);
		/**Returns the temporay directory for west data*/
		// TOOLS_EXPORT std::string get_temp_dir();

		/**Sleep \a msecs milliseconds*/
		TOOLS_EXPORT void msleep(int msecs);

		/**Return the number of milliseconds elapsed since Epoch.*/
		TOOLS_EXPORT long long msecs_since_epoch();

		/**Check endianness*/
		inline unsigned is_little_endian()
		{
			const union
			{
				unsigned i;
				unsigned char c[4];
			} one = {1}; /* don't use static : performance detrimental  */
			return one.c[0];
		}
		/** Byte swap unsigned short*/
		inline uint16_t swap_uint16(uint16_t val)
		{
			return (val << 8) | (val >> 8);
		}
		/** Byte swap short*/
		inline int16_t swap_int16(int16_t val)
		{
			return (val << 8) | ((val >> 8) & 0xFF);
		}
		/** Byte swap unsigned int*/
		inline uint32_t swap_uint32(uint32_t val)
		{
			val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
			return (val << 16) | (val >> 16);
		}
		/** Byte swap int*/
		inline int32_t swap_int32(int32_t val)
		{
			val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
			return (val << 16) | ((val >> 16) & 0xFFFF);
		}
		/** Byte swap int 64 bits*/
		inline int64_t swap_int64(int64_t val)
		{
			val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
			val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
			return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
		}
		/** Byte swap unsigned int 64 bits*/
		inline uint64_t swap_uint64(uint64_t val)
		{
			val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
			val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
			return (val << 32) | (val >> 32);
		}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
		inline double swap_double(double X)
		{
			int64_t x = reinterpret_cast<int64_t &>(X); //*(int64_t*)(&X);
			x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
			x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
			x = (x & 0x00FF00FF00FF00FF) << 8 | (x & 0xFF00FF00FF00FF00) >> 8;
			return reinterpret_cast<double &>(x); //*(double*)(&x);
		}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

		/** Check machine endianness */
		inline unsigned isLittleEndian()
		{
			const union
			{
				uint32_t i;
				uint8_t c[4];
			} one = {1}; /* don't use static : performance detrimental  */
			return one.c[0];
		}

#ifdef _MSC_VER
		/**Truncate given file*/
		TOOLS_EXPORT int truncate(const char *fname, size_t off);
#else
	/**
	truncate function already available on all unix systems
	*/

#endif

		/**
		Very basic 2D array class.
		Values are stored in Row major order.
		*/
		template <class T>
		struct Array2D
		{
			std::vector<T> values;
			size_t width;
			size_t height;

			Array2D() noexcept
				: width(0), height(0)
			{
			}
			Array2D(Array2D &&other) noexcept
				: values(std::move(other.values)), width(0), height(0)
			{
				std::swap(width, other.width);
				std::swap(height, other.height);
			}
			Array2D(const Array2D &other)
				: values(other.values()), width(other.width), height(other.height)
			{
			}
			~Array2D() noexcept
			{
			}
			Array2D &operator=(Array2D &&other) noexcept
			{
				values = std::move(other.values);
				std::swap(width, other.width);
				std::swap(height, other.height);
				return *this;
			}
			Array2D &operator=(const Array2D &other) noexcept
			{
				values = other.values;
				width = other.width;
				height = other.height;
				return *this;
			}

			size_t size() const noexcept { return values.size(); }

			T &operator()(size_t y, size_t x) noexcept
			{
				return values[x + y * width];
			}
			const T &operator()(size_t y, size_t x) const noexcept
			{
				return values[x + y * width];
			}

			void resize(size_t new_size, const T *data = NULL)
			{
				values.resize(new_size);
				if (data)
					std::copy(data, data + new_size, values.begin());
			}

			void resize(size_t new_size, const T val)
			{
				values.resize(new_size, val);
			}
		};

		/**
		Straightforward view on a 2D array.
		Values are considered to be stored in Row major order.
		*/
		template <class T>
		struct Array2DView
		{
			T *values;
			size_t width;
			size_t height;

			Array2DView() : values(NULL), width(0), height(0) {}
			Array2DView(T *ptr, size_t w, size_t h)
				: values(ptr), width(w), height(h)
			{
			}
			Array2DView(Array2D<T> &ar)
				: values(ar.values.data()), width(ar.width), height(ar.height)
			{
			}
			size_t size() const noexcept { return width * height; }
			const T &operator()(size_t y, size_t x) const noexcept
			{
				return values[x + y * width];
			}
			T &operator()(size_t y, size_t x) noexcept
			{
				return values[x + y * width];
			}
		};
		/**
		Straightforward view on a 2D array.
		Values are considered to be stored in Row major order.
		*/
		template <class T>
		struct ConstArray2DView
		{
			const T *values;
			size_t width;
			size_t height;

			ConstArray2DView() : values(NULL), width(0), height(0) {}
			ConstArray2DView(const T *ptr, size_t w, size_t h)
				: values(ptr), width(w), height(h)
			{
			}
			ConstArray2DView(const Array2D<T> &ar)
				: values(ar.values.data()), width(ar.width), height(ar.height)
			{
			}
			ConstArray2DView(const Array2DView<T> &ar)
				: values(ar.values), width(ar.width), height(ar.height)
			{
			}

			size_t size() const noexcept { return width * height; }
			const T &operator()(size_t y, size_t x) const noexcept
			{
				return values[x + y * width];
			}
		};

		/**
		Simple class to read file/string containing ascii float/double values.
		This class is much faster than standard stl streams, which are painfully slow when reading huge ascii matrices.
		*/
		class TOOLS_EXPORT FileFloatStream
		{
			FILE *file;
			const char *string;
			size_t len;
			size_t pos;
			char buff[100];

		public:
			/**Construct from file*/
			FileFloatStream(const char *filename, const char *format = "r");
			/**Construct from string*/
			FileFloatStream(const char *string, size_t len);
			~FileFloatStream();

			/**Open given file*/
			bool open(const char *filename, const char *format = "r");
			/**Open given string*/
			bool open(const char *string, size_t len);
			/**Close stream. Automatically called in the destructor, and before any call to #Open().*/
			void close();

			/**Returns true if the stream is open on a file or string*/
			bool isOpen() const;
			/**Returns true if we reach the end of file/string*/
			bool atEnd() const { return pos >= len; }
			/**Returns the file/string total size*/
			size_t size() const;
			/**Returns the current read position*/
			size_t tell() const;
			/**Seek to given position from the beginning of file/string*/
			bool seek(size_t p);

			/**Read all remaining data from current position*/
			std::string readAll();
			/**Read next line*/
			std::string readLine();

			/**Reads the next float or double value and returns it.
			If not NULL, ok is set to true on success, false otherwise.*/
			template <class T>
			T readFloat(bool *ok = NULL)
			{
				if (!isOpen() || pos >= len)
				{
					if (ok)
						*ok = false;
					return (T)0;
				}

				size_t saved_pos = pos;

				// seek to actual position
				if (file)
					fseek(file, (long)pos, SEEK_SET);

				// skip empty characters
				char first;
				while (true)
				{
					first = file ? getc(file) : string[pos];
					++pos;
					if ((!(first == ' ' || first == '\t' || first == '\n' || first == '\r')) || pos == len)
					{
						break;
					}
				}
				buff[0] = first;

				{
					// read next 32 bytes
					size_t max_read = len - pos;
					if (max_read > 32)
						max_read = 32;
					if (file)
						fread(buff + 1, max_read, 1, file);
					else
						memcpy(buff + 1, string + pos, max_read);
					char *start = buff;
					char *end = buff + 1 + max_read;
					*end = 0;
					T res = (T)strtod(start, &end);
					if (start == end)
					{
						if (ok)
							*ok = false;
						pos = saved_pos;
						return (T)0;
					}
					pos += (end - start) - 1;
					if (ok)
						*ok = true;
					return res;
				}

				// read file until word break
				size_t max_read = len - pos;
				if (max_read > 98)
					max_read = 98;
				size_t i = 1;
				for (; i <= max_read; ++i)
				{
					char c = file ? getc(file) : string[pos + i - 1];
					if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
					{
						break;
					}
					buff[i] = c;
				}
				buff[i] = 0;

				char *start = buff;
				char *end = buff + i;
				T res;
				if (std::is_same<T, double>::value)
					res = (T)strtod(start, &end);
				else if (std::is_same<T, float>::value)
					res = (T)strtof(start, &end);
				else
				{
					if (ok)
						*ok = false;
					pos = saved_pos;
					return (T)0;
				}
				if (end == start)
				{
					if (ok)
						*ok = false;
					pos = saved_pos;
					return (T)0;
				}
				pos += (end - start) - 1;
				if (ok)
					*ok = true;
				return res;
			}
		};

		/**
		Read ascii matrix from a FileFloatStream object
		*/
		template <class T>
		Array2D<T> readFileFast(FileFloatStream &fin, std::string *error = NULL)
		{
			static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value, "wrong data type (should be float or double)");
			if (!fin.isOpen())
			{
				if (error)
					*error = "Cannot read stream: not open";
				return Array2D<T>();
			}
			bool ok;

			std::int64_t start = fin.tell();

			// read first value
			bool is_ascii = true;
			/*double val =*/fin.readFloat<double>(&ok);

			if (!ok)
			{
				is_ascii = false;
			}

			if (!is_ascii)
			{
				start = fin.tell() + 256;
				// the file probably has a WEST header
				// fin.open(filename,"rb");
				fin.seek(start);
				/*double val =*/fin.readFloat<double>(&ok);
				if (!ok)
				{
					// sometimes, the header seems to have 256 + 5 bytes
					start += 5;
					// fin.open(filename, "rb");
					fin.seek(start);
					/*val =*/fin.readFloat<double>(&ok);
					if (!ok)
					{
						if (error)
							*error = "Cannot interpret file format";
						return Array2D<T>();
					}
				}
			}

			fin.seek(start);

			// read full file
			std::string file = fin.readAll();

			// count line ending
			std::string eol = "\n";
			if (file.find("\r\n") != std::string::npos)
				eol = "\r\n";
			int eol_count = count_substring(file, eol);

			Array2D<T> res;
			res.width = 0;
			res.height = 0;

			// check if we need to skip comment line
			int skip_line = 0;
			{
				FileFloatStream tmp(file.c_str(), file.size());
				while (true)
				{
					std::string line = tmp.readLine();
					if (line[0] == ('C') || line[0] == ('#') || line[0] == ('%') || line.substr(0, 2) == ("//"))
						++skip_line;
					else
						break;
				}
			}

			{
				FileFloatStream s(file.c_str(), file.size());
				// skip comments
				for (int i = 0; i < skip_line; ++i)
				{
					std::string line = s.readLine();
				}
				// skip empty lines
				size_t pos = s.tell();
				while (true)
				{
					std::string line = s.readLine();
					if (line.empty())
					{
						pos = s.tell();
						if (s.atEnd())
							break;
					}
					else
						break;
				}
				s.seek(pos);

				std::vector<T> tmp;
				// read values
				while (true)
				{
					// read a full line
					std::string line = s.readLine();

					// empy line: finished
					if (line.empty())
						break;

					// read all values in the line
					int width = 0;
					T value;

					FileFloatStream line_s(line.c_str(), line.size());
					while (true)
					{
						value = line_s.readFloat<double>(&ok);
						if (ok)
						{
							tmp.push_back(value);
							++width;
						}
						else
							break;
					}

					// empy line: finished
					if (width == 0)
						break;

					// line with a different width
					if (res.width && (int)res.width != width)
					{
						if (error)
							*error = "The file does not contain a matrix ";
						return Array2D<T>();
					}

					res.width = width;
					res.height++;
				}

				res.values = tmp;
			}

			return res;
		}

		/**
		Read ascii matrix from file
		*/
		template <class T>
		Array2D<T> readFileFast(const char *filename, std::string *error = NULL)
		{
			FileFloatStream fin(filename, "rb");
			// open file
			if (!fin.isOpen())
			{
				if (error)
					*error = "Cannot read file " + std::string(filename);
				return Array2D<T>();
			}
			return readFileFast<T>(fin, error);
		}

	}
