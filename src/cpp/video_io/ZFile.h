#ifndef Z_FILE_H
#define Z_FILE_H

#include "rir_config.h"
#include "ReadFileChunk.h"

/** @file

C interface to manage video files compressed with zstd
*/

/**
Open in read-only mode given compressed BIN file and return an opaque handle on the file.
*/
IO_EXPORT void *z_open_file_read(const rir::FileReaderPtr &file_reader);
/**
Open in read-only mode given compressed BIN file stored in memory and return an opaque handle on the file.
*/
IO_EXPORT void *z_open_memory_read(void *mem, uint64_t size);
/**
Open in write-only mode given compressed BIN file and return an opaque handle on the file.
*/
IO_EXPORT void *z_open_file_write(const char *filename, int width, int height, int rate, int method, int clevel = 2);
/**
Open in read-only mode given compressed BIN file stored in memory and return an opaque handle on the file.
*/
IO_EXPORT void *z_open_memory_write(void *mem, uint64_t size, int width, int height, int rate, int method, int clevel = 2);
/**
Close a compressed BIN file and return its size (the size is only valid for write-only handle)
*/
IO_EXPORT uint64_t z_close_file(void *file);
/**
Return the number of images stored in given file.
*/
IO_EXPORT int z_image_count(void *file);
/**
Return the image dimension of given file.
*/
IO_EXPORT int z_image_size(void *file, int *width, int *height);
/**
Write an image in a write-only handle with given \a timestamp.
*/
IO_EXPORT int z_write_image(void *file, const unsigned short *img, int64_t timestamp);
/**
Read the image at position \a pos from given handle.
If \a timestamp is not NULL, the image timestamp in nanoseconds will be stored inside.
*/
IO_EXPORT int z_read_image(void *file, int pos, unsigned short *img, int64_t *timestamp = NULL);
/**
For read-only handle, return the a pointer to the timestamp array.
This array has a size given by #z_image_count and contains timestamps in nanoseconds.
*/
IO_EXPORT int64_t *z_get_timestamps(void *file);

#endif