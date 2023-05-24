#pragma once

#include "rir_config.h"

#include <cstdint>


/** @file 

Defines abstractions to read files from any kind of sources (local files, remote file, custom interface...)

*/


/**
Read \a buf_size bytes from \a file_reader starting to the current position (see #seekFile and #posFile) into \a buf.
Returns the number of bytes actually read.
*/
TOOLS_EXPORT int readFile(void* file_reader, void* buf, int buf_size);
TOOLS_EXPORT int readFile2(void* file_reader, uint8_t* outbuf, int buf_size);
/**
Seek file reader to given position. Returns -1 on error, or the new position.
*/
TOOLS_EXPORT int64_t seekFile(void* file_reader, int64_t pos, int whence);
/**
Return current file reader position.
*/
TOOLS_EXPORT int64_t posFile(void* file_reader);
/**
Get file size from file reader object.
*/
TOOLS_EXPORT int64_t fileSize(void * file_reader);


/**
Helper and test functions
*/





#ifndef AVSEEK_SIZE
#define  AVSEEK_SIZE   0x10000 /*AVSEEK_SIZE from FFMPEG*/
#endif
#define AVSEEK_SET SEEK_SET
#define AVSEEK_CUR SEEK_CUR
#define AVSEEK_END SEEK_END

/**
Read a chunk at given offset into output buffer
*/
typedef int64_t(*read_chunk)(void*, int64_t, uint8_t*);
/**
Get file infos from opaque structure:
 - file size in bytes,
 - chunk number,
 - chunk size in bytes
*/
typedef void(*file_infos)(void*, int64_t*, int64_t*, int64_t*);
/**
Destroy internal opaque object
*/
typedef void(*destroy_opaque)(void*);

/**
\brief Structure representing a chunk based file access.
A FileAccess can be used to create a file reader object that can be passed to the librir through the function
#open_camera_file_reader(). FileAccess is basically a virtual file descriptor that could represent
a physical file, or for instance a network ressource. You can use the function #createFileAccess()
to create a FileAccess object representing a file.

In this case, calling
\code
open_camera_file_reader(createFileReader(createFileAccess("myfile")),NULL);
\endcode

is the same as
\code
open_camera_file("myfile",NULL);
\endcode

A FileAccess object contains the following fields:
	- opaque: opaque resource (like a file descriptor) that will be passed to the functions infos, read and destroy.
	- infos: function to get file informations: its size, the number of chunk it contains and the chunk size.
	- read: read a full chunk into a destination buffer. This function should take care of the last chunk which is usually
	smaller.
	- destroy: destroy the opaque object.
*/
struct FileAccess
{
	void* opaque;
	file_infos infos;
	read_chunk read;
	destroy_opaque destroy;
};

/**
Create and return a FileAccess from a local file
*/
TOOLS_EXPORT FileAccess createFileAccess(const char* filename, int64_t chunk_size = 4096);


/**
Create a file reader object from a #FileAccess object.
This file reader can be passed to the librir function #open_camera_file_reader().
*/
TOOLS_EXPORT void* createFileReader(FileAccess access);
/**
Destroy a file reader object previously created with #createFileReader().
*/
TOOLS_EXPORT void destroyFileReader(void* reader);

