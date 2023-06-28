#include "Filters.h"
#include "SIMD.h"

namespace rir
{

	void clampMin(unsigned short *img, size_t size, unsigned short min_value)
	{
		bool sse41 = detectInstructionSet().HW_SSE41;
#ifndef __SSE4_1__
		sse41 = false;
#endif

		if (sse41)
		{
#ifdef __SSE4_1__
			size_t end = size & (~7ULL);
			const __m128i med_val = _mm_set1_epi16(min_value);
			if ((uint64_t)img % 16 == 0)
			{
				for (size_t i = 0; i < end; i += 8)
				{
					__m128i val = _mm_load_si128((const __m128i *)(img + i));
					val = _mm_max_epu16(val, med_val);
					_mm_store_si128((__m128i *)(img + i), val);
				}
			}
			else
			{
#pragma omp parallel for num_threads(4)
				for (size_t i = 0; i < end; i += 8)
				{
					__m128i val = _mm_loadu_si128((const __m128i *)(img + i));
					val = _mm_max_epu16(val, med_val);
					_mm_storeu_si128((__m128i *)(img + i), val);
				}
			}
			for (size_t i = end; i < size; ++i)
				if (img[i] < min_value)
					img[i] = min_value;
#endif
		}
		else
		{
#pragma omp parallel for num_threads(4)
			for (size_t i = 0; i < size; ++i)
				if (img[i] < min_value)
					img[i] = min_value;
		}
	}

	/**
	 * Returns the median pixel for input image.
	 * \a percent specify the requested percent quantile (0.5 for median).
	 */
	unsigned short findMedianPixel(unsigned short *pixels, size_t size, float percent)
	{
		// build histogram
		std::vector<size_t> hist(65535, 0);
		for (size_t i = 0; i < size; ++i)
			hist[pixels[i]]++;

		size_t s = static_cast<size_t>(std::round(size * percent));
		size_t count = 0;
		for (size_t i = 0; i < hist.size(); ++i)
		{
			count += hist[i];
			if (count >= s)
				return static_cast<unsigned short>(i);
		}
		return 0;
	}

	/**
	 * Returns the median pixel for input masked image.
	 * \a percent specify the requested percent quantile (0.5 for median).
	 */
	unsigned short findMedianPixelMask(unsigned short *pixels, size_t size, float percent, unsigned char *mask)
	{
		// build histogram
		std::vector<size_t> hist(65535, 0);
		size_t c = 0;
		for (size_t i = 0; i < size; ++i)
		{
			if (mask[i])
			{
				hist[pixels[i]]++;
				++c;
			}
		}
		// printf("count; %i\n", c);
		size_t s = (int)std::round(c * percent);
		size_t count = 0;
		for (size_t i = 0; i < hist.size(); ++i)
		{
			count += hist[i];
			if (count >= s)
				return (unsigned short)i;
		}
		return 0;
	}

	namespace detail
	{
		static double prevTime(const double *iter)
		{
			return *(iter - 1);
		}
	}

	std::vector<double> extractTimes(const double **vectors, size_t vector_count, size_t *vector_sizes, int s)
	{
		if (vector_count == 0)
			return std::vector<double>();
		else if (vector_count == 1)
		{
			return std::vector<double>(vectors[0], vectors[0] + vector_sizes[0]);
		}

		std::vector<double> res;
		res.reserve(vector_sizes[0]);

		// create our time iterators
		std::vector<const double *> iters, ends;
		for (size_t i = 0; i < vector_count; ++i)
		{
			// search for nan time value
			const double *v = vectors[i];
			const size_t size = vector_sizes[i];
			size_t p = 0;
			for (; p < size; ++p)
			{
				if (std::isnan(v[p]))
					break;
			}
			if (p != size)
			{
				// split the vector in 2 vectors
				iters.push_back(vectors[i]);
				ends.push_back(vectors[i]);
				ends.back() += p;
				iters.push_back(vectors[i]);
				iters.back() += p + 1;
				ends.push_back(vectors[i] + vector_sizes[i]);
			}
			else
			{
				iters.push_back(vectors[i]);
				ends.push_back(vectors[i] + vector_sizes[i]);
			}
		}

		if (s & ResampleIntersection)
		{
			// resample on intersection:
			// find the intersection time range
			double start = 0, end = -1;
			for (size_t i = 0; i < vector_count; ++i)
			{
				if (end < start)
				{ // init
					start = vectors[i][0];
					end = vectors[i][vector_sizes[i] - 1];
				}
				else
				{ // intersection
					if (vectors[i][vector_sizes[i] - 1] < start)
						return res; // null intersection
					else if (vectors[i][0] > end)
						return res; // null intersection
					start = std::max(start, vectors[i][0]);
					end = std::min(end, vectors[i][vector_sizes[i] - 1]);
				}
			}

			// reduce the iterator ranges
			for (size_t i = 0; i < iters.size(); ++i)
			{
				while (*iters[i] < start)
					++iters[i];
				while (ends[i] != iters[i] && detail::prevTime(ends[i]) > end)
					--ends[i]; /*{
					 --ends[i];
					 while (*ends[i] > end) --ends[i];
				 }*/
			}
		}

		while (iters.size())
		{
			// find minimum time among all time vectors
			double min_time = *iters.front();
			for (size_t i = 1; i < iters.size(); ++i)
				min_time = std::min(min_time, *iters[i]);

			// increment each iterator equal to min_time
			for (size_t i = 0; i < iters.size(); ++i)
			{
				if (iters[i] != ends[i])
					if (*iters[i] == min_time)
						if (++iters[i] == ends[i])
						{
							iters.erase(iters.begin() + i);
							ends.erase(ends.begin() + i);
							--i;
						}
			}
			res.push_back(min_time);
		}

		return res;
	}

	std::vector<double> resampleSignal(const double *sample_x, const double *sample_y, size_t size, const double *times, size_t times_size, int s, double padds)
	{
		std::vector<double> res(times_size);

		if (size == 0)
		{
			double val = padds;
			if (!(s & ResamplePadd0))
				val = 0;
			std::fill(res.begin(), res.end(), val);
			return res;
		}

		const double *iters_x = sample_x;
		const double *ends_x = sample_x + size;
		const double *iters_y = sample_y;
		// const double* ends_y = sample_y + size;

		for (size_t t = 0; t < times_size; ++t)
		{
			const double time = times[t];

			// we already reached the last sample value
			if (iters_x == ends_x)
			{
				if (s & ResamplePadd0)
					res[t] = padds;
				else
					res[t] = sample_y[size - 1];
				continue;
			}

			const double samp_x = *iters_x;
			const double samp_y = *iters_y;

			// same time: just add the sample
			if (time == samp_x)
			{
				res[t] = samp_y;
				++iters_x;
				++iters_y;
			}
			// we are before the sample
			else if (time < samp_x)
			{
				// sample starts after
				if (iters_x == sample_x)
				{
					if (s & ResamplePadd0)
						res[t] = padds;
					else
						res[t] = samp_y;
				}
				// in between 2 values
				else
				{
					const double *prev_x = iters_x - 1;
					const double *prev_y = iters_y - 1;
					if (s & ResampleInterpolation)
					{
						double factor = (double)(time - *prev_x) / (double)(samp_x - *prev_x);
						res[t] = samp_y * factor + (1 - factor) * (*prev_y);
					}
					else
					{
						// take the closest value
						if (time - *prev_x < samp_x - time)
							res[t] = *prev_y;
						else
							res[t] = samp_y;
					}
				}
			}
			else
			{
				// we are after the sample, increment until this is not the case
				while (iters_x != ends_x && *iters_x < time)
				{
					++iters_x;
					++iters_y;
				}
				if (iters_x != ends_x)
				{
					if (*iters_x == time)
					{
						res[t] = *iters_y;
					}
					else
					{
						const double *prev_x = iters_x - 1;
						const double *prev_y = iters_y - 1;
						if (s & ResampleInterpolation)
						{
							// interpolation
							double factor = (double)(time - *prev_x) / (double)(*iters_x - *prev_x);
							res[t] = *iters_y * factor + (1 - factor) * *prev_y;
						}
						else
						{
							// take the closest value
							if (time - *prev_x < *iters_x - time)
								res[t] = *prev_y;
							else
								res[t] = *iters_y;
						}
					}
				}
				else
				{
					// reach sample end
					if (s & ResamplePadd0)
						res[t] = padds;
					else
						res[t] = sample_y[size - 1];
				}
			}
		}
		return res;
	}

	void resampleSignals(std::vector<double> &sig1_x, std::vector<double> &sig1_y, std::vector<double> &sig2_x, std::vector<double> &sig2_y, int strategy, double padd0)
	{
		std::vector<double> times;
		if (sig1_x.size() == 0)
			times = sig2_x;
		else if (sig2_x.size() == 0)
			times = sig1_x;
		else
		{
			const double *time_vecs[2] = {sig1_x.data(), sig2_x.data()};
			size_t sizes[2] = {(size_t)sig1_x.size(), (size_t)sig2_x.size()};
			times = extractTimes(time_vecs, 2, sizes, strategy);
		}

		std::vector<double> new_sig1_y = resampleSignal(sig1_x.data(), sig1_y.data(), (int)sig1_x.size(), times.data(), (int)times.size(), strategy, padd0);
		std::vector<double> new_sig2_y = resampleSignal(sig2_x.data(), sig2_y.data(), (int)sig2_x.size(), times.data(), (int)times.size(), strategy, padd0);

		std::swap(sig1_x, times);
		sig2_x = sig1_x;

		std::swap(sig1_y, new_sig1_y);
		std::swap(sig2_y, new_sig2_y);
	}

}