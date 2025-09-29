#pragma once

#include "rir_config.h"
#include "ReadFileChunk.h"
#include "Misc.h"


#include <map>
#include <string>

#define TABLE_TRAILER "H264ATTRIBUTES"

/** @file
 */

namespace rir
{

	/**
	Manage attributes for any kind of video file and read/write them from/to the file trailer.
	If given file does not have an attribute section, this class creates it.

	This class manages two types of attributes:
		-	Global attributes attached to the full video file
		-	Frame attributes attached to a single image

	Attributes are represented by a std::map<std::string,std::string>. String objects can contain binary content for the values, but the key should store ascii null terminated content.
	Attributes with a size > 1000 bytes will be compressed using zstd library.

	Note that this class add a binary content at the end of the video file. This works with MP4 video files as ffmpeg can ignore the trailer when reading back the video, but
	this is not guaranteed to work with other formats.
	*/
	class TOOLS_EXPORT FileAttributes : public BaseShared
	{
	public:
		FileAttributes();
		~FileAttributes();

		/// @brief Open file attribute object in read-only mode using a file descriptor as returned by createFileReader()
		/// @param file_access file descriptor
		/// @return true on success, false otherwise
		bool openReadOnly(const FileReaderPtr& file_access);
		/// @brief Open file attribute object using a file path
		/// @param filename local file name
		/// @return true on success, false otherwise
		bool open(const char *filename);
		/// @brief Write attributes to the file and close the file attribuyte object.
		void close();
		/// @brief Close the file attribute object without writting attributes to the file
		void discard();
		/// @brief Write any new attribute to the output file
		void flush();
		/// @brief Returns true if the file attribute object is open and pointing to a valid file
		bool isOpen() const;
		/// @brief Returns the full trailer size in bytes
		size_t tableSize() const;
		/// @brief Returns the number of frames in this attribute object
		size_t size() const;
		/// @brief Set the number of frames of this attribute object. Previous frame attributes are kept until new_size.
		void resize(size_t new_size);

		/// @brief Returns the file global attributes
		const std::map<std::string, std::string> &globalAttributes() const;
		/// @brief Set the file global attributes
		void setGlobalAttributes(const std::map<std::string, std::string> &attributes);
		/// @brief Add a global attribute
		void addGlobalAttribute(const std::string &key, const std::string &value);

		/// @brief Returns the timestamp for given frame. Note that resize() must have been called before.
		int64_t timestamp(size_t index) const;
		/// @brief Set the timestamp for given frame.
		void setTimestamp(size_t index, int64_t time);

		/// @brief Returns the frame attributes for given image position
		const std::map<std::string, std::string> &attributes(size_t index) const;
		/// @brief Set the frame attributes for given image position
		void setAttributes(size_t index, const std::map<std::string, std::string> &attributes);
		/// @brief Add a frame attribute for given image position
		void addAttribute(size_t index, const std::string &key, const std::string &value);

	private:
		void writeIfDirty();
		class PrivateData;
		PrivateData *m_data;
	};

}