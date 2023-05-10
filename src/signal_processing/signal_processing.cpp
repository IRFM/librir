#include "signal_processing.h"
#include "Filters.h"
#include "tools.h"
#include "charls.h"

extern "C" {
#include <string.h>
}

using namespace rir;


template< class T>
int translate_internal(void* src, void* dst, int w, int h, float dx, float dy, void* background, const char* strategy)
{
	const T back = *((T*)background);
	T* _src = (T*)src;
	T* _dst = (T*)dst;
	if (!strategy || strcmp(strategy, "noborder") == 0 || strlen(strategy) == 0) {
		translate(_src, _dst, back, w, h, dx, dy, TranslateUnchanged);
		return 0;
	}
	else if (strcmp(strategy, "background") == 0) {
		translate(_src, _dst, back, w, h, dx, dy, TranslateConstant);
		return 0;
	}
	else if (strcmp(strategy, "wrap") == 0) {
		translate(_src, _dst, back, w, h, dx, dy, TranslateWrap);
		return 0;
	}
	else if (strcmp(strategy, "nearest") == 0) {
		translate(_src, _dst, back, w, h, dx, dy, TranslateNearest);
		return 0;
	}
	else
		return -1;
}

int translate(int type, void* src, void* dst, int w, int h, float dx, float dy, void* background, const char* strategy)
{
	switch (type) {
	case '?': return translate_internal<bool>(src, dst, w, h, dx, dy, background, strategy);
	case 'b': return translate_internal<char>(src, dst, w, h, dx, dy, background, strategy);
	case 'B': return translate_internal<unsigned char>(src, dst, w, h, dx, dy, background, strategy);
	case 'h': return translate_internal<short>(src, dst, w, h, dx, dy, background, strategy);
	case 'H': return translate_internal<unsigned short>(src, dst, w, h, dx, dy, background, strategy);
	case 'i': return translate_internal<int>(src, dst, w, h, dx, dy, background, strategy);
	case 'I': return translate_internal<unsigned int>(src, dst, w, h, dx, dy, background, strategy);
	case 'l': return translate_internal<long long>(src, dst, w, h, dx, dy, background, strategy);
	case 'L': return translate_internal<unsigned long long>(src, dst, w, h, dx, dy, background, strategy);
	case 'f': return translate_internal<float>(src, dst, w, h, dx, dy, background, strategy);
	case 'd': return translate_internal<double>(src, dst, w, h, dx, dy, background, strategy);
	default: return -1;
	}

}



#ifndef M_PI
#define M_PI     0.318309886183790671538
#endif

static void generate_kernel(float sigma, float* dst, int radius)
{
	float s = 2.0f * sigma * sigma;
	float sum = 0.0;
	int w = radius * 2 + 1;

	// generate kernel 
	for (int x = -radius; x <= radius; x++) {
		for (int y = -radius; y <= radius; y++) {
			float r = (float)std::sqrt(x * x + y * y);
			dst[x + radius + (y + radius) * w] = (float)((std::exp(-(r * r) / s)) / (M_PI * s));
			sum += dst[x + radius + (y + radius) * w];
		}
	}
	// normalize kernel
	int size = w * w;
	for (int i = 0; i < size; ++i)
		dst[i] /= sum;
}

int gaussian_filter(float* src, float* dst, int w, int h, float sigma)
{
	int radius = (int)(sigma * 2);
	if (radius < 1)
		radius = 1;
	int kernel_w = (radius * 2 + 1);

	std::vector<float> kernel(kernel_w * kernel_w);
	generate_kernel(sigma, kernel.data(), radius);

#pragma omp parallel for
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; ++x) {

			if (x >= radius && x < w - radius && y >= radius && y < h - radius) {
				//simple convolution
				float res = 0;
				for (int dx = -radius; dx <= radius; ++dx)
					for (int dy = -radius; dy <= radius; ++dy) {
						res += kernel[dx + radius + (dy + radius) * kernel_w] * src[x + dx + (y + dy) * w];
					}
				dst[x + y * w] = res;
			}
			else {
				//border convolution
				float res = 0;
				float sum = 0;
				for (int dx = -radius; dx <= radius; ++dx)
					for (int dy = -radius; dy <= radius; ++dy) {
						int _x = x + dx;
						int _y = y + dy;
						if (_x >= 0 && _x < w && _y >= 0 && _y < h) {
							float k = kernel[dx + radius + (dy + radius) * kernel_w];
							sum += k;
							res += k * src[_x + _y * w];
						}
					}
				dst[x + y * w] = res / sum;
			}
		}
	return 0;
}



int find_median_pixel(unsigned short* pixels, int size, float percent)
{
	return findMedianPixel(pixels, size, percent);
}

int find_median_pixel_mask(unsigned short* pixels, unsigned char* mask, int size, float percent)
{
	return findMedianPixelMask(pixels, size, percent, mask);
}

int extract_times(double* vectors, int vector_count, int* _vector_sizes, int s, double* output, int* output_size)
{
	std::vector<const double*> in_vectors(vector_count);
	int size = 0;
	for (int i = 0; i < vector_count; ++i) {
		in_vectors[i] = vectors + size;
		size += _vector_sizes[i];
	}

	std::vector<size_t> vector_sizes((size_t)vector_count);
	std::copy(_vector_sizes, _vector_sizes + vector_count, vector_sizes.begin());

	std::vector<double> res = extractTimes(in_vectors.data(), vector_count, vector_sizes.data(), s);
	if ((int)res.size() > *output_size) {
		*output_size = (int)res.size();
		return -2;
	}

	std::copy(res.begin(), res.end(), output);
	*output_size = (int)res.size();
	return 0;
}


int resample_time_serie(double* sample_x, double* sample_y, int size, double* times, int times_size, int s, double padds, double* output, int* output_size)
{
	std::vector<double> res = resampleSignal(sample_x, sample_y, size, times, times_size, s, padds);

	if ((int)res.size() > *output_size) {
		*output_size = (int)res.size();
		return -1;
	}
	std::copy(res.begin(), res.end(), output);
	*output_size = (int)res.size();
	return 0;
}




int jpegls_encode(unsigned short* img, int width, int height, int err, char* out, int out_size)
{

	charls_jpegls_encoder* enc = charls_jpegls_encoder_create();

	charls_frame_info infos;
	infos.width = width;
	infos.height = height;
	infos.bits_per_sample = 16;
	infos.component_count = 1;

	charls_jpegls_errc err1 = charls_jpegls_encoder_set_frame_info(enc, &infos);
	if (err1 != charls::ApiResult::success) return -1;
	charls_jpegls_errc err2 = charls_jpegls_encoder_set_near_lossless(enc, err);
	if (err2 != charls::ApiResult::success) return -1;
	charls_jpegls_errc err3 = charls_jpegls_encoder_set_destination_buffer(enc, out, out_size);
	if (err3 != charls::ApiResult::success) return -1;
	charls_jpegls_errc err4 = charls_jpegls_encoder_encode_from_buffer(enc, img, width * height * 2, width * 2);
	if (err4 != charls::ApiResult::success) return -1;
	size_t written = 0;
	charls_jpegls_errc err5 = charls_jpegls_encoder_get_bytes_written(enc, &written);
	if (err5 != charls::ApiResult::success) return -1;

	charls_jpegls_encoder_destroy(enc);
	return (int)written;
}

int jpegls_decode(char* in, int in_size, unsigned short* img, int width, int height)
{

	charls_jpegls_decoder* dec = charls_jpegls_decoder_create();

	charls_jpegls_errc err1 = charls_jpegls_decoder_set_source_buffer(dec, in, in_size);
	if (err1 != charls::ApiResult::success) return -1;
	err1 = charls_jpegls_decoder_read_header(dec);
	if (err1 != charls::ApiResult::success) return -1;
	err1 = charls_jpegls_decoder_decode_to_buffer(dec, img, width * height * 2, width * 2);
	if (err1 != charls::ApiResult::success) return -1;

	charls_jpegls_decoder_destroy(dec);
	return 0;
}

extern "C" {
#include <jpeglib.h>
}

int jpeg_decode(char* input, int64_t isize, unsigned char* output)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned long location = 0;

	cinfo.err = jpeg_std_error(&jerr);

	//jpeg_create_decompress(&cinfo);
	jpeg_CreateDecompress((&cinfo), JPEG_LIB_VERSION, (size_t)sizeof(struct jpeg_decompress_struct));
	jpeg_mem_src(&cinfo, (unsigned char*)input, isize);

	(void)jpeg_read_header(&cinfo, TRUE);
	if (!jpeg_start_decompress(&cinfo))
		return -1;
	JSAMPLE* row[1];
	while (cinfo.output_scanline < cinfo.image_height)
	{
		row[0] = reinterpret_cast<JSAMPLE*>(output) + location;
		jpeg_read_scanlines(&cinfo, row, 1);
		location += cinfo.image_width;
	}
	// wrap up decompression, destroy objects, free pointers and close open files
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return 0;

}

#include "BadPixels.h"
int bad_pixels_create(unsigned short* first_image, int width, int height)
{
	BadPixels* bp = new BadPixels();
	bp->init(first_image, width, height);
	return set_void_ptr(bp);
}
int bad_pixels_correct(int handle, unsigned short* in, unsigned short* out)
{
	BadPixels* bp = (BadPixels*)get_void_ptr(handle);
	if (!bp)
		return -1;
	bp->correct(in, out);
	return 0;
}
void bad_pixels_destroy(int handle)
{
	BadPixels* bp = (BadPixels*)get_void_ptr(handle);
	if (!bp) return;
	else rm_void_ptr(handle);
	delete bp;
}

int label_image(int type, void* src, int* dst, int w, int h, void* background,  double* out_xy, int* out_area)
{
	std::vector<Label> labels;

	switch (type) {
	case '?': labels = rir::labelImage((bool*)src, dst, w, h, *(bool*)background); break;
	case 'b': labels = rir::labelImage((char*)src, dst, w, h, *(char*)background); break;
	case 'B': labels = rir::labelImage((unsigned char*)src, dst, w, h, *(unsigned char*)background); break;
	case 'h': labels = rir::labelImage((short*)src, dst, w, h, *(short*)background); break;
	case 'H': labels = rir::labelImage((unsigned short*)src, dst, w, h, *(unsigned short*)background); break;
	case 'i': labels = rir::labelImage((int*)src, dst, w, h, *(int*)background); break;
	case 'I': labels = rir::labelImage((unsigned int*)src, dst, w, h, *(unsigned int*)background); break;
	case 'l': labels = rir::labelImage((long long*)src, dst, w, h, *(long long*)background); break;
	case 'L': labels = rir::labelImage((unsigned long long*)src, dst, w, h, *(unsigned long long*)background); break;
	case 'f': labels = rir::labelImage((float*)src, dst, w, h, *(float*)background); break;
	case 'd': labels = rir::labelImage((double*)src, dst, w, h, *(double*)background); break;
	default: return -1;
	}

	for (size_t i = 0; i < labels.size(); ++i) {
		out_xy[i * 2] = labels[i].first.x();
		out_xy[i * 2 + 1] = labels[i].first.x();
		out_area[i] = labels[i].area;
	}
	return (int)labels.size();
}


int keep_largest_area(int type, void* src, int* dst, int w, int h, void* background, int foreground)
{
	switch (type) {
	case '?': rir::keepLargestArea((bool*)src, dst, w, h, *(bool*)background, foreground); break;
	case 'b': rir::keepLargestArea((char*)src, dst, w, h, *(char*)background, foreground); break;
	case 'B': rir::keepLargestArea((unsigned char*)src, dst, w, h, *(unsigned char*)background, foreground); break;
	case 'h': rir::keepLargestArea((short*)src, dst, w, h, *(short*)background, foreground); break;
	case 'H': rir::keepLargestArea((unsigned short*)src, dst, w, h, *(unsigned short*)background, foreground); break;
	case 'i': rir::keepLargestArea((int*)src, dst, w, h, *(int*)background, foreground); break;
	case 'I': rir::keepLargestArea((unsigned int*)src, dst, w, h, *(unsigned int*)background, foreground); break;
	case 'l': rir::keepLargestArea((long long*)src, dst, w, h, *(long long*)background, foreground); break;
	case 'L': rir::keepLargestArea((unsigned long long*)src, dst, w, h, *(unsigned long long*)background, foreground); break;
	case 'f': rir::keepLargestArea((float*)src, dst, w, h, *(float*)background, foreground); break;
	case 'd': rir::keepLargestArea((double*)src, dst, w, h, *(double*)background, foreground); break;
	default: return -1;
	}
	return 0;
}
