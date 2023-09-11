#ifndef TOOLS_H
#define TOOLS_H

/** @file
 
C interface for the main utility features defined in the tools library.
Most functions return an integer set to 0 on success, and a value < 0 on error.
*/


#include "rir_config.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
Log levels
*/
#define INFO_LEVEL		0
#define WARNING_LEVEL	1
#define ERROR_LEVEL		2

/**All function using a char * as output with an unspecified length must have at least a size of UNSPECIFIED_CHAR_LENGTH*/
#define UNSPECIFIED_CHAR_LENGTH 200 

/**Function type for logging purposes*/
typedef void (*print_function)(int, const char*);





/**
Logging functions
*/

/**
Set the log function to be used by LIBRIR.
This function must have the signature: void print_function(int,const char*), where the first argument is
the log level (INFO_LEVEL, WARNING_LEVEL or ERROR_LEVEL).
*/
TOOLS_EXPORT void set_print_function(print_function function);
/**
Disables logging
*/
TOOLS_EXPORT void disable_print();
/**
Resets the logging to its initial state
*/
TOOLS_EXPORT void reset_print_functions();

/**
Retrieves the last logged error.
The results is stored in \a text.
\a len must be set to \a text length. After a call to this function, it will be set to the output text length.
If \a len is too small to conatin the actual error text, this function returns -1 and  \a len is set to the right length.
*/
TOOLS_EXPORT int get_last_log_error(char* text, int* len);





/**
For compatibility with older thermavip versions, new versions should NOT use these 3 functions
*/

TOOLS_EXPORT void get_temp_directory(char* dirname);
TOOLS_EXPORT void get_default_temp_directory(char* dirname);
TOOLS_EXPORT int set_temp_directory(const char* dirname);




/**
Object handler functions
*/


/**
* Internally store \a cam pointer and returns an integer handle on it.
* This function is thread safe.
*/
TOOLS_EXPORT int set_void_ptr(void* cam);
/**
* Returns the void* object corresponding to given handle.
* This function is thread safe.
*/
TOOLS_EXPORT void* get_void_ptr(int index);
/**
* Remove given handle.
* This function is thread safe.
* Note that the corresponding void* object is not freed.
*/
TOOLS_EXPORT void rm_void_ptr(int index);







/**
File attributes related functions
*/


/**
Read file attributes based on a file read object.
The file reader object is NOT destroyed by this function.
Returns the file attribute object handle.
Returns 0 on error.
*/
TOOLS_EXPORT int attrs_read_file_reader(void* file_reader);
/**
Read file attributes.
Returns the file attribute object handle.
Returns 0 on error.
*/
TOOLS_EXPORT int attrs_open_file(const char* filename);
/**
Close attribute file.
Attributes are written to file at closing.
To avoid writting them, call attrs_discard instead.
*/
TOOLS_EXPORT void attrs_close(int handle);
TOOLS_EXPORT void attrs_discard(int handle);
TOOLS_EXPORT int attrs_flush(int handle);
/**
Returns the number of frames for this handle.
Returns -1 on error.
*/
TOOLS_EXPORT int attrs_image_count(int handle);
/**
Returns the number of global attributes for this handle.
Returns -1 on error.
*/
TOOLS_EXPORT int attrs_global_attribute_count(int handle);
/**
For given handle, read the attribute name at \a pos into \a name.
\a name is a binary array of size \a len.
Returns 0 on success, -1 on error, -2 if given len is too small.
\a len will be set to the right value.
*/
TOOLS_EXPORT int attrs_global_attribute_name(int handle, int pos, char* name, int* len);
/**
For given handle, read the attribute value at \a pos into \a value.
\a value is a binary array of size \a len.
Returns 0 on success, -1 on error, -2 if given len is too small.
\a len will be set to the right value.
*/
TOOLS_EXPORT int attrs_global_attribute_value(int handle, int pos, char* value, int* len);
/**
Returns the number of frame attributes for given frame number.
Returns -1 on error.
*/
TOOLS_EXPORT int attrs_frame_attribute_count(int handle, int frame);
/**
For given handle and frame number, read the frame attribute name at \a pos into \a name.
\a name is a binary array of size \a len.
Returns 0 on success, -1 on error, -2 if given len is too small.
\a len will be set to the right value.
*/
TOOLS_EXPORT int attrs_frame_attribute_name(int handle, int frame, int pos, char* name, int* len);
/**
For given handle and frame number, read the attribute value at \a pos into \a value.
\a value is a binary array of size \a len.
Returns 0 on success, -1 on error, -2 if given len is too small.
\a len will be set to the right value.
*/
TOOLS_EXPORT int attrs_frame_attribute_value(int handle, int frame, int pos, char* value, int* len);

/**
Read frame timestamp for given frame number
*/
TOOLS_EXPORT int attrs_frame_timestamp(int handle, int frame, int64_t* time);
/**
Read all frame timestamps
*/
TOOLS_EXPORT int attrs_timestamps(int handle, int64_t* time);
/**
Set timestamps and resize the timestamp vector if needed.
*/
TOOLS_EXPORT int attrs_set_times(int handle, int64_t* times, int size);
/**
Set the frame timestamp at given position
*/
TOOLS_EXPORT int attrs_set_time(int handle, int pos, int64_t time);
/**
Set frame attributes for given position.
*/
TOOLS_EXPORT int attrs_set_frame_attributes(int handle, int pos, char* keys, int* key_lens, char* values, int* value_lens, int count);
/**
Set global attributes.
*/
TOOLS_EXPORT int attrs_set_global_attributes(int handle, char* keys, int* key_lens, char* values, int* value_lens, int count);


/**
Zstd/blosc compression/decompression, for Python wrapper
*/

TOOLS_EXPORT int64_t zstd_compress_bound(int64_t srcSize);
TOOLS_EXPORT int64_t zstd_decompress_bound(char* src, int64_t srcSize);
TOOLS_EXPORT int64_t zstd_compress(char* src, int64_t srcSize, char* dst, int64_t dstSize, int level);
TOOLS_EXPORT int64_t zstd_decompress(char* src, int64_t srcSize, char* dst, int64_t dstSize);

#define RIR_BLOSC_NOSHUFFLE 0
#define RIR_BLOSC_SHUFFLE 1
#define RIR_BLOSC_BITSHUFFLE 2

TOOLS_EXPORT int64_t blosc_compress_zstd(char* src, int64_t srcSize, char* dst, int64_t dstSize, size_t typesize, int doshuffle, int level );
TOOLS_EXPORT int64_t blosc_decompress_zstd(char* src, int64_t srcSize, char* dst, int64_t dstSize);


TOOLS_EXPORT int unzip(const char* infile, const char* outfolder); 


#ifdef __cplusplus
}
#endif


#endif /*TOOLS_H*/