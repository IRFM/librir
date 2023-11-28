#include "BadPixels.h"
#include "Filters.h"
#include <cstring>

namespace rir
{

	BadPixels::BadPixels()
		: m_width(0), m_height(0), m_median_value(-1)
	{
	}

	void BadPixels::init(const unsigned short *first, int width, int height, int std_factor)
	{
		m_width = width;
		m_height = height;
		m_bad_pixels = badPixels(first, width, height, std_factor);

		int size = width * height;
		std::vector<unsigned short> img(first, first + size);
		std::sort(img.data(), img.data() + size);
		m_median_value = img[size / 2];

		// compute std
		double sum = 0;
		int c = 0;
		for (int i = 0; i < size; ++i, ++c)
			sum += (first[i] - m_median_value) * (first[i] - m_median_value);
		sum /= c;
		sum = sqrt(sum);
		m_median_value -= (int)(sum * 2);
	}

	void BadPixels::correct(const unsigned short *in, unsigned short *out)
	{
		unsigned short pixels[9];
		// unsigned short buff[10];
		int w = m_width;
		int h = m_height;

		if (in != out)
			memcpy(out, in, m_width * m_height * 2);
		for (size_t i = 0; i < m_bad_pixels.size(); ++i)
		{
			int x = m_bad_pixels[i].x();
			int y = m_bad_pixels[i].y();

			unsigned short *pix = pixels;
			for (int dx = x - 1; dx <= x + 1; ++dx)
				for (int dy = y - 1; dy <= y + 1; ++dy)
					if (dx >= 0 && dy >= 0 && dx < w && dy < h)
					{
						*pix++ = in[dx + dy * w];
					}
			int c = (int)(pix - pixels);

			std::nth_element(pixels, pixels + c / 2, pixels + c);
			out[x + y * w] = pixels[c / 2];
		}

		// remove low values
		if (m_median_value > 0)
		{
			clampMin(out, w * h, m_median_value);
		}
	}

	void BadPixels::correctOnePass(unsigned short *img, int width, int height, int std_factor)
	{
		long long st = msecs_since_epoch();
		static constexpr int win_w = 5;
		static constexpr int win_h = 5;

#pragma omp parallel for // threads(2)
		for (int y = 0; y < height; ++y)
		{

			unsigned short pixels[win_w * win_h];

			for (int x = 0; x < width; ++x)
			{

				unsigned short *pix = pixels;
				for (int dy = y - win_h / 2; dy <= y + win_h / 2; ++dy)
					for (int dx = x - win_w / 2; dx <= x + win_w / 2; ++dx)
						if (dx >= 0 && dy >= 0 && dx < width && dy < height)
							*pix++ = img[dx + dy * width];

				int size = (int)(pix - pixels);
				std::sort(pixels, pixels + size);
				int med = pixels[size / 2];
				double sum2 = 0;
				int c = 0;
				for (int i = size / 5; i < size * 4 / 5; ++i, ++c)
					sum2 += (signed_integral)(pixels[i] - med) * (signed_integral)(pixels[i] - med);
				sum2 /= c;
				double std = std::sqrt(sum2);

				// consider pixels outside avg +- 5*std
				double lower = med - std_factor * std;
				double upper = med + std_factor * std;

				int pix2 = img[x + y * width];
				if (pix2 < lower || pix2 > upper)
				{
					// correct
					unsigned short *pix = pixels;
					for (int dx = x - 1; dx <= x + 1; ++dx)
						for (int dy = y - 1; dy <= y + 1; ++dy)
							if (dx >= 0 && dy >= 0 && dx < width && dy < height)
							{
								*pix++ = img[dx + dy * width];
							}
					int c = (int)(pix - pixels);

					std::nth_element(pixels, pixels + c / 2, pixels + c);
					img[x + y * width] = pixels[c / 2];
				}
			}
		}

		long long el = msecs_since_epoch() - st;
		printf("el: %i\n", (int)el);
	}

}
