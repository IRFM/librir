#ifndef IO_H
#define IO_H

/** @file

C interface for the main features defined in the tools library.
Most functions return an integer set to 0 on success, and a value < 0 on error.
*/

#include "rir_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define FILE_FORMAT_PCR 1
#define FILE_FORMAT_WEST 2
#define FILE_FORMAT_PCR_ENCAPSULATED 3
#define FILE_FORMAT_ZSTD_COMPRESSED 4
#define FILE_FORMAT_H264 5
#define FILE_FORMAT_HCC 6
#define FILE_FORMAT_OTHER 7

	/**
	Opens a camera video file and returns the camera descriptor on success, NULL otherwise.
	If \a file_format is not NULL, it will be set to the file type: FILE_FORMAT_PCR, FILE_FORMAT_WEST,
	FILE_FORMAT_PCR_ENCAPSULATED, FILE_FORMAT_ZSTD_COMPRESSED, FILE_FORMAT_H264 or 0 on error.
	*/
	IO_EXPORT int open_camera_file(const char *filename, int *file_format);

	/**
	 * Returns the video file format or -1 on error.
	 */
	IO_EXPORT int video_file_format(const char *filename);

	/**
	Opens a camera video file from a file reader (created with #createFileReader()) object and returns
	the camera descriptor on success, NULL otherwise.
	If \a file_format is not NULL, it will be set to the file type: FILE_FORMAT_PCR, FILE_FORMAT_WEST,
	FILE_FORMAT_PCR_ENCAPSULATED, FILE_FORMAT_ZSTD_COMPRESSED, FILE_FORMAT_H264 or 0 on error.
	The created camera object will take ownership of the file reader and will destroy it with #destroyFileReader()
	on closing.
	\sa createFileReader
	*/
	IO_EXPORT int open_camera_file_reader(void *file_reader, int *file_format);

	/**
	Opens a camera video file from a memory reader (created with #createMemoryReader()) object and returns
	the camera descriptor on success, NULL otherwise.
	If \a file_format is not NULL, it will be set to the file type: FILE_FORMAT_PCR, FILE_FORMAT_WEST,
	FILE_FORMAT_PCR_ENCAPSULATED, FILE_FORMAT_ZSTD_COMPRESSED, FILE_FORMAT_H264 or 0 on error.
	The created camera object will take ownership of the file reader and will destroy it with #destroyMemoryReader()
	on closing.
	\sa createMemoryReader
	*/
	IO_EXPORT int open_camera_from_memory(void *ptr, int64_t size, int *file_format);

	/**
	Closes a previously opened camera or video file.
	*/
	IO_EXPORT int close_camera(int camera);
	/**
	Returns the number of image for given camera descriptor, ot -1 on failure.
	*/
	IO_EXPORT int get_image_count(int camera);
	/**
	Reads in \a time the image time in nanoseconds (in WEST time base) at position \a pos for \a camera descriptor.
	Returns 0 on success, -1 otherwise.
	*/
	IO_EXPORT int get_image_time(int camera, int pos, int64_t *time);
	/**
	Reads the camera image width and height.
	Returns 0 on success, -1 otherwise.
	*/
	IO_EXPORT int get_image_size(int camera, int *width, int *height);

	/**
	Reads the camera filename.
	Returns 0 on success, -1 otherwise.
	*/
	IO_EXPORT int get_filename(int camera, char *filename);

	/**
	Returns the number of possible calibrations for the camera.
	Returns 0 on success, -1 on failure.
	Usually, an IR camera has 2 calibration: Digital Levels and Temperature.
	*/
	IO_EXPORT int supported_calibrations(int camera, int *count);
	/**
	Reads the calibration name (like "Digital Level" or "Temperature") fort given \a camera and \a calibration index.
	Returns 0 on success, -1 on failure.
	*/
	IO_EXPORT int calibration_name(int camera, int calibration, char *name);

	/**
	Reads an image from \a camera at position \a pos using given \a calibration.
	Usually, if \a calibration is 0, it reads the raw uncalibrated image. If \a calibration is 1, it reads the calibrated (temperature) image.
	The result is stored in \a pixels.
	Returns 0 on success, -1 otherwise.
	*/
	IO_EXPORT int load_image(int camera, int pos, int calibration, unsigned short *pixels);

	/**
	Apply given calibration to a DL image (inplace)
	*/
	IO_EXPORT int calibrate_inplace(int camera, unsigned short *img, int size, int calibration);

	/**
	Set the global scene emissivity (same for each pixel).
	Only works if support_emissivity() returns 1.
	*/
	IO_EXPORT int set_global_emissivity(int camera, float emi);
	/**
	Set the emissivity for each pixels.
	size should be equal to width*height.
	Returns -1 on failure, the number of copied values on success.
	*/
	IO_EXPORT int set_emissivity(int camera, float *emi, int size);
	/**
	Copy the \a size first pixel emissivities to \a emi.
	Returns -1 on failure, the number of copied values on success.
	*/
	IO_EXPORT int get_emissivity(int camera, float *emi, int size);
	/**
	Tells if given camera support custom emissivities (returns 1 or 0).
	*/
	IO_EXPORT int support_emissivity(int camera);

	/**
	Enable/disable bad pixels removal for given camera.
	*/
	IO_EXPORT int enable_bad_pixels(int cam, int enable);
	/**
	Returns 1 if bad pixels removal is enabled for given camera, 0 otherwise.
	*/
	IO_EXPORT int bad_pixels_enabled(int cam);

	/**
	 * Load motion correction file for given video.
	 * Returns -1 on error, 0 on success.
	 */
	IO_EXPORT int load_motion_correction_file(int cam, const char *filename);
	/**
	 * Enable/disable motion correction for given video.
	 */
	IO_EXPORT int enable_motion_correction(int cam, int enable);
	/**
	 * Returns 1 if motion correction is enabled on given video, 0 otherwise.
	 */
	IO_EXPORT int motion_correction_enabled(int cam);

	/**
	Calibrate image based on the calibration files used for given camera.
	*/
	IO_EXPORT int calibrate_image(int cam, unsigned short *img, float *out, int size, int calib);
	/**
	Calibrate image based on the calibration files used for given camera (in-place version).
	*/
	IO_EXPORT int calibrate_image_inplace(int cam, unsigned short *img, int size, int calib);
	/**
	Tells if the last image calibration for given camera saturated.
	*/
	IO_EXPORT int camera_saturate(int cam);

	/**
	Returns the calibration file names for give camera.
	File names are stored in dst with a '\n' separator.
	Returns 0 on success, -1 on error.
	If dstSize is too small to contain the file names, it is set to the required size and the function returns -2.

	This function only works for WEST IR cameras and returns:
	 - The lut file name,
	 - The optical temperature file name,
	 - The fut transmission file name,
	 - The hublot transmission file name,
	 - The mir transmission file name.
	*/
	IO_EXPORT int calibration_files(int camera, char *dst, int *dstSize);

	IO_EXPORT int flip_camera_calibration(int camera, int flip_rl, int flip_ud);

	/*Attribute management for video files */

	/**
	Returns the number of attributes for the last read image.
	Returns -1 on error.
	*/
	IO_EXPORT int get_attribute_count(int camera);
	/**
	Returns the pair key->value at given attribute index for the last read image.
	Returns 0 on success, -1 on error.
	\a key_len and \a value_len must be set to the actual size of \a key and \a value.
	If given length are not enough, returns -1 and \a key_len and \a value_len are set to the right values.
	*/
	IO_EXPORT int get_attribute(int camera, int index, char *key, int *key_len, char *value, int *value_len);
	/**
	Returns the number of attributes for the video.
	Returns -1 on error.
	*/
	IO_EXPORT int get_global_attribute_count(int camera);
	/**
	Returns the pair key->value for given global attribute index.
	Returns 0 on success, -1 on error.
	\a key_len and \a value_len must be set to the actual size of \a key and \a value.
	If given length are not enough, returns -1 and \a key_len and \a value_len are set to the right values.
	*/
	IO_EXPORT int get_global_attribute(int camera, int index, char *key, int *key_len, char *value, int *value_len);

	/*
	Saving in h264
	*/

	/**
	Open output video file with given width and height.
	In case of lossy compression, lossy_height  controls where the loss stops (in order to keep the last rows lossless).
	Returns 0 on error, file identifier on success.
	*/
	IO_EXPORT int h264_open_file(const char *filename, int width, int height, int lossy_height);
	/**
	Close h264 video saver
	*/
	IO_EXPORT void h264_close_file(int file);
	/**
	Set a compression parameter. Supported values:
		- compressionLevel: h264 compression level (0 for very fast, 8 for maximum compression)
		- lowValueError: for lossy compression, maximum error on temperature values < background
		- highValueError: for lossy compression, maximum error on temperature values >= background
		- autoUpdatePixelInterval: for lossy compression, force update each pixel every X frames
		- ketFrames: proportion of key frames between 0 (no key frame) and 1 (all frames are key frames)
	Returns 0 on success, -1 on error.
	*/
	IO_EXPORT int h264_set_parameter(int file, const char *param, const char *value);
	/**
	Set file global attributes.
	@param attribute_count number of attributes
	@param keys all the keys concatenated in one string
	@param key_lens array containing the key lengths
	@param values all the values concatenated in one string
	@param value_lens array containing the value lengths
	Returns -1 on error, 0 on success.
	Parameters can be set until the first call to h264_add_image_lossless or h264_add_image_lossy.
	*/
	IO_EXPORT int h264_set_global_attributes(int file, int attribute_count, char *keys, int *key_lens, char *values, int *value_lens);
	/**
	Add and compress (lossless) an image to the file.
	@param file file identifier (> 0)
	@param img image
	@param timestamps_ns image timestamp in nanoseconds
	@param attribute_count number of attributes
	@param keys all the keys concatenated in one string
	@param key_lens array containing the key lengths
	@param values all the values concatenated in one string
	@param value_lens array containing the value lengths
	*/
	IO_EXPORT int h264_add_image_lossless(int file, unsigned short *img, int64_t timestamps_ns, int attribute_count, char *keys, int *key_lens, char *values, int *value_lens);
	/**
	Add and compress (lossy) an image to the file.
	@param file file identifier (> 0)
	@param img_DL image in digital levels
	@param img_T corresponding image in temperature
	@param timestamps_ns image timestamp in nanoseconds
	@param attribute_count number of attributes
	@param keys all the keys concatenated in one string
	@param key_lens array containing the key lengths
	@param values all the values concatenated in one string
	@param value_lens array containing the value lengths
	*/
	IO_EXPORT int h264_add_image_lossy(int file, unsigned short *img_DL, int64_t timestamps_ns, int attribute_count, char *keys, int *key_lens, char *values, int *value_lens);

	/**
	Add losses to input image without writing it to file
	 */
	IO_EXPORT int h264_add_loss(int file, unsigned short *img);

	IO_EXPORT int h264_get_low_errors(int file, unsigned short *errors, int *size);
	IO_EXPORT int h264_get_high_errors(int file, unsigned short *errors, int *size);

	/**
	 * Returns the list of floating point tables given camera provides.
	 * The table names are written to dst with a '\n' separator.
	 * Returns 0 on success, -1 on error, -2 if dst_size is too small.
	 */
	IO_EXPORT int get_table_names(int cam, char *dst, int *dst_size);
	/**
	 * Returns table for given camera and table name.
	 * Returns 0 on success, -1 on error, -2 if dst_size is too small.
	 */
	IO_EXPORT int get_table(int cam, const char *name, float *dst, int *dst_size);

	/**
	ZSTD compressed video file, mainly for tests
	*/

	/**
	Open output video file with given image width and height, frame rate, comrpession method and compression level.
	method == 1 means ZSTD standard compression (clevel goes from 0 to 22),
	method == 2 means blosc+ZSTD standard compression (clevel goes from 1 to 10),
	method == 3 means blosc+ZSTD advanced compression (clevel goes from 1 to 10).
	Returns the video writer identifier on success, -1 on error.
	*/
	IO_EXPORT int open_video_write(const char *filename, int width, int height, int rate, int method, int clevel);
	/**
	Write an image to given writter.
	Returns 0 on success, -1 on error.
	*/
	IO_EXPORT int image_write(int writter, unsigned short *img, int64_t time);
	/**
	Close video writter. Returns the file size.
	*/
	IO_EXPORT int64_t close_video(int writter);

	IO_EXPORT int get_last_image_raw_value(int camera, int x, int y, unsigned short *value);

	/*Internal use only*/
	IO_EXPORT int correct_PCR_file(const char *filename, int width, int height, int freq);

	IO_EXPORT int change_hcc_external_blackbody_temperature(const char *filename, float temperature);

#ifdef __cplusplus
}
#endif

#endif /*IO_H*/
