#ifndef FILTERS_H
#define FILTERS_H

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "Primitives.h"

/** @file

Miscellaneous filters
*/

namespace rir
{

	namespace detail
	{
#define PIX_SORT(a, b)           \
	{                            \
		if ((a) > (b))           \
			std::swap((a), (b)); \
	}

		/*----------------------------------------------------------------------------
		Median for 3 values
		---------------------------------------------------------------------------*/
		template <class T>
		inline T opt_med3(T *p)
		{
			PIX_SORT(p[0], p[1]);
			PIX_SORT(p[1], p[2]);
			PIX_SORT(p[0], p[1]);
			return (p[1]);
		}

		/*----------------------------------------------------------------------------
		Median for 9 values
		---------------------------------------------------------------------------*/
		template <class T>
		inline T opt_med9(T *p)
		{
			PIX_SORT(p[1], p[2]);
			PIX_SORT(p[4], p[5]);
			PIX_SORT(p[7], p[8]);
			PIX_SORT(p[0], p[1]);
			PIX_SORT(p[3], p[4]);
			PIX_SORT(p[6], p[7]);
			PIX_SORT(p[1], p[2]);
			PIX_SORT(p[4], p[5]);
			PIX_SORT(p[7], p[8]);
			PIX_SORT(p[0], p[3]);
			PIX_SORT(p[5], p[8]);
			PIX_SORT(p[4], p[7]);
			PIX_SORT(p[3], p[6]);
			PIX_SORT(p[1], p[4]);
			PIX_SORT(p[2], p[5]);
			PIX_SORT(p[4], p[7]);
			PIX_SORT(p[4], p[2]);
			PIX_SORT(p[6], p[4]);
			PIX_SORT(p[4], p[2]);
			return (p[4]);
		}

	}

	/**
	Simple and fast image median filter on a 3*3 kernel using multithreading
	*/
	template <class T, class U>
	void medianFilter(const T *src, U *out, size_t w, size_t h)
	{
		// first row
		const T *_src = src;
		T *_out = out;

		_out[0] = std::min(_src[0], _src[1]);
		_out[w - 1] = std::min(_src[w - 2], _src[w - 1]);
		for (size_t i = 1; i < w - 1; ++i)
		{
			T tmp[3];
			std::copy(_src + i - 1, _src + i + 2, tmp);
			_out[i] = detail::opt_med3(tmp);
		}
		// last row
		_src = src + (h - 1) * w;
		_out = out + (h - 1) * w;

		_out[0] = std::min(_src[0], _src[1]);
		_out[w - 1] = std::min(_src[w - 2], _src[w - 1]);
		for (size_t i = 1; i < w - 1; ++i)
		{
			T tmp[3];
			std::copy(_src + i - 1, _src + i + 2, tmp);
			_out[i] = detail::opt_med3(tmp);
		}

		// first column
		_src = src;
		_out = out;
		for (size_t y = 1; y < h - 1; ++y)
		{
			T tmp[3] = {_src[(y - 1) * w], _src[y * w], _src[(y + 1) * w]};
			_out[y * w] = detail::opt_med3(tmp);
		}

		// last column
		_src = src + w - 1;
		_out = out + w - 1;
		for (size_t y = 1; y < h - 1; ++y)
		{
			T tmp[3] = {_src[(y - 1) * w], _src[y * w], _src[(y + 1) * w]};
			_out[y * w] = detail::opt_med3(tmp);
		}

		// remaining
#pragma omp parallel for
		for (signed_integral y = 1; y < h - 1; ++y)
			for (signed_integral x = 1; x < w - 1; ++x)
			{
				const T *s1 = src + (y - 1) * w + x - 1;
				const T *s2 = src + (y)*w + x - 1;
				const T *s3 = src + (y + 1) * w + x - 1;
				T tmp[9] = {s1[0], s1[1], s1[2], s2[0], s2[1], s2[2], s3[0], s3[1], s3[2]};
				std::nth_element(tmp, tmp + 4, tmp + 9);
				out[x + y * w] = tmp[4];
			}
	}

	/**
	Detect bad pixels on the first camera image.
	Returns a list of coordinates.
	*/
	template <class T>
	Polygon badPixels(const T *src, size_t w, size_t h, double std_factor = 5)
	{
		Polygon res;
		const signed_integral win_w = 5;
		const signed_integral win_h = 5;
		std::vector<T> pixels(win_w * win_h);

		for (signed_integral y = 0; y < h; ++y)
			for (signed_integral x = 0; x < w; ++x)
			{

				T *pix = pixels.data();
				for (signed_integral dy = y - win_h / 2; dy <= y + win_h / 2; ++dy)
					for (signed_integral dx = x - win_w / 2; dx <= x + win_w / 2; ++dx)
						if (dx >= 0 && dy >= 0 && dx < w && dy < h)
							*pix++ = src[dx + dy * w];

				signed_integral size = (signed_integral)(pix - pixels.data());
				std::sort(pixels.data(), pixels.data() + size);
				signed_integral med = pixels[size / 2];
				double sum2 = 0;
				signed_integral c = 0;
				for (signed_integral i = size / 5; i < size * 4 / 5; ++i, ++c)
					sum2 += (pixels[i] - med) * (pixels[i] - med);
				sum2 /= c;
				double std = std::sqrt(sum2);

				// consider pixels outside avg +- 5*std
				double lower = med - std_factor * std;
				double upper = med + std_factor * std;

				if (src[x + y * w] < lower || src[x + y * w] > upper)
					res.push_back(Point(x, y));
			}

		return res;
	}

	/**
	 * Fast clamp min on an unsigned short image using SSE4.1 and multithreading
	 */
	SIGNAL_PROCESSING_EXPORT void clampMin(unsigned short *img, size_t size, unsigned short min_value);

	/**
	 * Returns the median pixel for input image.
	 * \a percent specify the requested percent quantile (0.5 for median).
	 */
	SIGNAL_PROCESSING_EXPORT unsigned short findMedianPixel(unsigned short *pixels, size_t size, float percent);

	/**
	 * Returns the median pixel for input masked image.
	 * \a percent specify the requested percent quantile (0.5 for median).
	 */
	SIGNAL_PROCESSING_EXPORT unsigned short findMedianPixelMask(unsigned short *pixels, size_t size, float percent, unsigned char *mask);

	enum ResampleStrategy
	{
		ResampleUnion = 0,
		ResampleIntersection = 0x01,
		ResamplePadd0 = 0x02,
		ResampleInterpolation = 0x04
	};

	/**
	Extract the time vector for several std::vector<double>.
	Returns a growing time vector containing all different time values of given std::vector<double> samples,
	respecting the given resample strategy.
	*/
	SIGNAL_PROCESSING_EXPORT std::vector<double> extractTimes(const double **vectors, size_t vector_count, size_t *vector_sizes, int strategy);
	SIGNAL_PROCESSING_EXPORT std::vector<double> resampleSignal(const double *sample_x, const double *sample_y, size_t size, const double *times, size_t times_size, int strategy, double padds);
	SIGNAL_PROCESSING_EXPORT void resampleSignals(std::vector<double> &sig1_x, std::vector<double> &sig1_y, std::vector<double> &sig2_x, std::vector<double> &sig2_y, int strategy, double padd0 = 0);

	namespace detail
	{
		inline size_t wrap(size_t value, size_t max) { return (value + max) % max; }
		template <class T>
		inline T cast(double value) { return static_cast<T>(value); }
		template <>
		inline bool cast(double value) { return value != 0; }
	}

	enum TranslateBorder
	{
		TranslateUnchanged, // ignore border pixels
		TranslateConstant,	// use constant value for borders
		TranslateWrap,		// use wrap strategy
		TranslateNearest	// use nearset pixel
	};

	/**
	 * Translate in put image by a floating point offset using given border management.
	 */
	template <class T, class U>
	void translate(const T *src, U *dst, U background, size_t w, size_t h, float dx, float dy, TranslateBorder strategy)
	{
#pragma omp parallel for
		for (int y = 0; y < (int)h; ++y)
		{
			for (size_t x = 0; x < w; ++x)
			{
				float px = x - dx;
				float py = y - dy;
				if (px < 0 || px >= w || py < 0 || py >= h)
				{
					if (strategy == TranslateUnchanged)
					{
						// no strategy
					}
					else if (strategy == TranslateConstant)
					{
						// use background
						dst[x + y * w] = background;
					}
					else if (strategy == TranslateWrap)
					{
						// wrap
						size_t leftCellEdge = detail::wrap((size_t)(px), w);
						size_t rightCellEdge = detail::wrap((size_t)(px + 1), w);
						size_t topCellEdge = detail::wrap((size_t)(py), h);
						size_t bottomCellEdge = detail::wrap((size_t)(py + 1), h);
						const T p1 = src[bottomCellEdge * w + leftCellEdge];
						const T p2 = src[topCellEdge * w + leftCellEdge];
						const T p3 = src[bottomCellEdge * w + rightCellEdge];
						const T p4 = src[topCellEdge * w + rightCellEdge];
						const double u = std::abs(px - (int)px); //(px - leftCellEdge);// / (rightCellEdge - leftCellEdge);
						const double v = std::abs(py - (int)py); //(bottomCellEdge - py);// / (topCellEdge - bottomCellEdge);
						U value = detail::cast<U>((p1 * (1 - v) + p2 * v) * (1 - u) + (p3 * (1 - v) + p4 * v) * u);
						dst[x + y * w] = value;
					}
					else
					{ // if (strategy == TranslateNearest) {
						// nearest pixel
						size_t _x, _y;
						if (px < 0)
							_x = 0;
						else if (px >= w)
							_x = w - 1;
						else
							_x = (size_t)px;
						if (py < 0)
							_y = 0;
						else if (py >= h)
							_y = h - 1;
						else
							_y = (size_t)py;
						dst[x + y * w] = src[_x + _y * w];
					}
				}
				else
				{
					const size_t leftCellEdge = (size_t)(px);
					size_t rightCellEdge = (size_t)((px) + 1);
					if (rightCellEdge == w)
						rightCellEdge = leftCellEdge;
					const size_t topCellEdge = (size_t)(py);
					size_t bottomCellEdge = (size_t)(py + 1);
					if (bottomCellEdge == h)
						bottomCellEdge = topCellEdge;
					const T p1 = src[bottomCellEdge * w + leftCellEdge];
					const T p2 = src[topCellEdge * w + leftCellEdge];
					const T p3 = src[bottomCellEdge * w + rightCellEdge];
					const T p4 = src[topCellEdge * w + rightCellEdge];
					const double u = (px - leftCellEdge);	// / (rightCellEdge - leftCellEdge);
					const double v = (bottomCellEdge - py); // / (topCellEdge - bottomCellEdge);
					U value = detail::cast<U>((p1 * (1 - v) + p2 * v) * (1 - u) + (p3 * (1 - v) + p4 * v) * u);
					dst[x + y * w] = value;
				}
			}
		}
	}

	namespace detail
	{
		inline void extend(std::vector<signed_integral> &vec, signed_integral size)
		{
			while ((signed_integral)vec.size() < size)
				vec.push_back((signed_integral)vec.size());
		}

		static inline signed_integral find_top(std::vector<signed_integral> &relabel, signed_integral label)
		{
			signed_integral origin = label;
			while (label != relabel[label])
				label = relabel[label];

			// relabel the full path toward top most value to fasten following look ups
			do
			{
				relabel[origin] = label;
				origin = relabel[origin];
			} while (origin != relabel[origin]);

			return label;
		}
	}

	/**
	 * Label representing a ROI inside an image
	 */
	struct Label
	{
		Point first;
		size_t area;
		Label()
			: first(-1, -1), area(0) {}
	};

	/**
	 * Very fast image labelling algorithm
	 */
	template <class T, class U>
	std::vector<Label> labelImage(const T *input, U *output, const signed_integral w, const signed_integral h, const T background)
	{
		// In the worst case, relabel vector should have a size of w*h.
		// In practice, when the number of ROI is small, it's size is usually < 1000.
		// Therefore, we do not allocate it first, and extend it only when necessary.
		// This strategy divides by 2 the computation time for low ROI number (probably due to allocation + data locality)
		std::vector<signed_integral> relabel;

		signed_integral next_label = 1;
		signed_integral minx = w;
		signed_integral miny = -1;
		signed_integral maxx = -1;
		signed_integral maxy = -1;

		signed_integral index = 0;
		for (signed_integral y = 0; y < h; ++y)
		{
			for (signed_integral x = 0; x < w; ++x, ++index)
			{
				output[index] = (U)0;
				const T value = input[index];

				// ignore background value
				if (value == background)
				{

					continue;
				}

				if (x < minx)
					minx = x;
				if (x > maxx)
					maxx = x;
				if (miny < 0)
					miny = y;
				maxy = y;

				// check neighbors

				U label;

				// check left
				if (x > 0 && input[index - 1] == value)
				{
					label = output[index - 1];

					if (y > 0)
					{
						// check top
						if (const U top_label = output[index - w])
						{
							// top pixel has a different label
							if (top_label != label)
							{

								detail::extend(relabel, std::max(label, top_label) + 1);

								if (relabel[label] == label)
								{

									// get top_label origin
									signed_integral top = detail::find_top(relabel, top_label);

									if (top != label)
										relabel[label] = top_label;
								}
								else if (relabel[label] != top_label)
								{

									signed_integral v = detail::find_top(relabel, top_label);
									signed_integral v2 = detail::find_top(relabel, label);
									if (v != v2)
									{
										if (v > v2)
											relabel[v] = v2;
										else
											relabel[v2] = v;
									}
								}
							}
						}
					}
				}
				else if (y > 0 && output[index - w]) // check top
					label = output[index - w];
				else
					label = next_label++;

				output[index] = label;
			}
		}

		detail::extend(relabel, next_label);

		// at this point, we could relable and we would have isolated all components.
		// however, their labels would not necessarly be consecutive.
		// so we need to modify the relabel vector to have consecutive ids

		// just find the labels that don't move: they are the remaining ones
		std::vector<signed_integral> final_labels(next_label);
		final_labels[0] = 0;
		signed_integral label_count = 0;
		for (signed_integral i = 1; i < next_label; ++i)
		{

			// find final label
			signed_integral v = relabel[i];
			while (v != relabel[v])
			{
				v = relabel[v];
			}
			// affect new or existing label
			if (!final_labels[v])
				final_labels[v] = ++label_count;
			signed_integral label = final_labels[v];

			v = relabel[i];
			final_labels[i] = label;
			while (v != relabel[v] && final_labels[v] != label)
			{
				final_labels[v] = label;
				v = relabel[v];
			}
		}

		std::vector<Label> res(label_count + 1);

		// second pass: relabel output
		for (signed_integral y = miny; y < maxy + 1; ++y)
		{
			for (signed_integral x = minx; x < maxx + 1; ++x)
			{
				const signed_integral i = x + y * w;
				if (signed_integral v = (output[i] = final_labels[output[i]]))
				{
					// build output
					res[v].area++;
					if (res[v].first.x() < 0)
						res[v].first = Point(x, y);
				}
			}
		}

		return res;
	}

	template <class T, class U>
	void keepLargestArea(const T *input, U *output, const signed_integral w, const signed_integral h, const T background, const U foreground)
	{
		std::vector<Label> labels = labelImage(input, output, w, h, background);
		if (labels.size() < 2)
		{
			return;
		}

		// find biggest ROI
		Label *found = &labels[1];
		U value = output[found->first.x() + found->first.y() * w];
		for (size_t i = 1; i < labels.size(); ++i)
		{
			if (labels[i].area > found->area)
			{
				found = &labels[i];
				value = output[found->first.x() + found->first.y() * w];
			}
		}

		size_t size = w * h;
		for (size_t i = 0; i < size; ++i)
		{
			if (output[i] != value)
				output[i] = (U)background;
			else
				output[i] = foreground;
		}
	}

}
#endif
