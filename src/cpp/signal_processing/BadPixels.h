#pragma once

#include <vector>
#include "Primitives.h"

/** @file
 */

namespace rir
{
	/**
	 * Bad pixels correction for an IR video of unisgned short images
	 */
	class SIGNAL_PROCESSING_EXPORT BadPixels : public BaseShared
	{
	public:
		BadPixels();
		~BadPixels() {};

		/**
		 * Create the bad pixel list on the first image
		 */
		void init(const unsigned short *img, int width, int height, int std_factor = 5);
		/**
		 * Correct image
		 */
		void correct(const unsigned short *in, unsigned short *out);

		static void correctOnePass(unsigned short *img, int width, int height, int std_factor = 5);

	private:
		int m_width;
		int m_height;
		int m_median_value;
		Polygon m_bad_pixels;
	};

}