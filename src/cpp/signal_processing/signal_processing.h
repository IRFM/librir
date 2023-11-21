#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

/** @file

C interface for the signal_processing library
*/

#include "rir_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
    Image translation, for python wrapper only.
    @param type data type using numpy style ('?', 'I', 'b',...)
    @param src input pixels
    @param dst output pixels
    @param w width
    @param h height
    @param dx x offset
    @param dy y offset
    @param background background value used if strategy == "background"
    @param strategy border strategy management, one of "noborder", "background", "wrap" and "nearest".
    Returns 0 on success, -1 on error.
    */
    SIGNAL_PROCESSING_EXPORT int translate(int type, void *src, void *dst, int w, int h, float dx, float dy, void *background, const char *strategy);
    /**
    Gaussian filter, for python wrapper only
    */
    SIGNAL_PROCESSING_EXPORT int gaussian_filter(float *src, float *dst, int w, int h, float sigma);

    /**
     * Returns the median pixel for input image.
     * \a percent specify the requested percent quantile (0.5 for median).
     */
    SIGNAL_PROCESSING_EXPORT int find_median_pixel(unsigned short *pixels, int size, float percent = 0.5);
    /**
     * Returns the median pixel for input masked image.
     * \a percent specify the requested percent quantile (0.5 for median).
     */
    SIGNAL_PROCESSING_EXPORT int find_median_pixel_mask(unsigned short *pixels, unsigned char *mask, int size, float percent = 0.5);

    /**
    Create a unique time vector from several ones.
    Returns a growing time vector containing all different time values given in \a time_vector, without redundant times.
    The output time serie boundary depends on the strategy \a s:
    - 0 means take the union of all input time series
    - 1 means take the intersection.
    Return -1 on error, 0 on success, -2 if output size is too small.
    \a output_size is set to the real output size.
    */
    SIGNAL_PROCESSING_EXPORT int extract_times(double *time_vector, int vector_count, int *vector_sizes, int s, double *output, int *output_size);

    /**
    Resample a time serie based on a new time vector.
    @param  sample_x: input time values
    @param  sample_y: input y values
    @param  size: input data size
    @param  times: new time vector
    @param  times_size: new time serie size
    @param  s: resampling strategy (combination of ResamplePadd0(2) and ResampleInterpolation(4))
    @param  padds: padding value if (s & ResamplePadd0)
    @param  output: output y values corresponding to the new time vector
    @param  output_size: size of the output vector.
    Return -1 on error, 0 on success, -2 if output size is too small.
    \a output_size is set to the real output size.
    */
    SIGNAL_PROCESSING_EXPORT int resample_time_serie(double *sample_x, double *sample_y, int size, double *times, int times_size, int s, double padds, double *output, int *output_size);

    /**
     * Compress image using jpegls encoder.
     * \a err specifies the maximum allowed error.
     */
    SIGNAL_PROCESSING_EXPORT int jpegls_encode(unsigned short *img, int width, int height, int err, char *out, int out_size);
    /**
     * Decompress image using jpegls decoder.
     */
    SIGNAL_PROCESSING_EXPORT int jpegls_decode(char *in, int in_size, unsigned short *img, int width, int height);

    /**
     * Decode a 12 bits JPEG image as used in old IR video file format (from Tore Supra)
     */
    SIGNAL_PROCESSING_EXPORT int jpeg_decode(char *input, int64_t isize, unsigned char *output);

    /**
     * Create a bad pixel correction object based on a first video image.
     * returns the object handle on success, 0 on error.
     */
    SIGNAL_PROCESSING_EXPORT int bad_pixels_create(unsigned short *first_image, int width, int height);
    /**
     * Correct bad pixels on given image.
     */
    SIGNAL_PROCESSING_EXPORT int bad_pixels_correct(int handle, unsigned short *in, unsigned short *out);
    /**
     * Destroy a bad pixel object based on its handle.
     */
    SIGNAL_PROCESSING_EXPORT void bad_pixels_destroy(int handle);

    SIGNAL_PROCESSING_EXPORT int label_image(int type, void *src, int *dst, int w, int h, void *background, double *out_xy, int *out_area);

    SIGNAL_PROCESSING_EXPORT int keep_largest_area(int type, void *src, int *dst, int w, int h, void *background, int foreground);

#ifdef __cplusplus
}
#endif

#endif /*SIGNAL_PROCESSING_H*/
