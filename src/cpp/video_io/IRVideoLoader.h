#ifndef VIDEO_LOADER_H
#define VIDEO_LOADER_H

#include "Misc.h"
#include "Primitives.h"
#include "BaseCalibration.h"
#include "BadPixels.h"

#include <string>
#include <vector>
#include <map>

#undef close

/** @file
 */

namespace rir
{
	

	/**
	Base class for IR video loaders.
	Note that usually, all IRVideoLoader should be used through a BinFileLoader proxy.
	*/
	class IO_EXPORT IRVideoLoader
	{
		std::vector<float> m_invEmissivities;
		float m_globalEmi;

	public:
		IRVideoLoader();
		virtual ~IRVideoLoader();

		// Bad pixels management, must be used within derived classes
		virtual bool supportBadPixels() const { return false; }
		virtual void setBadPixelsEnabled(bool) {}
		virtual bool badPixelsEnabled() const { return false; }

		// Motion correction
		virtual bool loadTranslationFile(const char *filename) { return false; }
		virtual void enableMotionCorrection(bool) {}
		virtual bool motionCorrectionEnabled() const { return false; }

		/**Set global scene emissivity*/
		void setEmissivity(float emi)
		{
			if (emi != m_globalEmi || m_invEmissivities.empty())
			{
				Size s = imageSize();
				m_invEmissivities.resize(s.height * s.width);
				std::fill_n(m_invEmissivities.data(), m_invEmissivities.size(), 1.f / emi);
				m_globalEmi = emi;
			}
		}
		/**Set per pixel emissivity*/
		bool setEmissivities(const float *emi, size_t size)
		{
			if (size == 0)
				return false;
			Size s = imageSize();
			m_invEmissivities.resize(s.width * s.height);
			size_t _s = size;
			if ((size_t)m_invEmissivities.size() < _s)
				_s = (size_t)m_invEmissivities.size();
			for (size_t i = 0; i < _s; ++i)
				m_invEmissivities[i] = 1.f / emi[i];
			// fill missing values
			for (size_t i = _s - 1; i < m_invEmissivities.size(); ++i)
				m_invEmissivities[i] = 1.f;
			m_globalEmi = 0;
			return true;
		}
		bool setInvEmissivities(const float *inv_emi, size_t size)
		{
			if (size == 0)
				return false;
			Size s = imageSize();
			m_invEmissivities.resize(s.width * s.height);
			size_t _s = size;
			if ((size_t)m_invEmissivities.size() < _s)
				_s = (size_t)m_invEmissivities.size();
			for (size_t i = 0; i < _s; ++i)
				m_invEmissivities[i] = inv_emi[i];
			// fill missing values
			for (size_t i = _s - 1; i < m_invEmissivities.size(); ++i)
				m_invEmissivities[i] = 1.f;
			m_globalEmi = 0;
			return true;
		}
		/**Returns vector of inverted emissivities*/
		const std::vector<float> &invEmissivities() const
		{
			return m_invEmissivities;
		}
		float globalEmissivity() const { return m_globalEmi; }

		/**Returns true if the last read image calibration saturated, false otherwise*/
		virtual bool saturate() const { return false; }
		/**Movie size in number of images*/
		virtual int size() const = 0;
		/**Images timestamps in nanoseconds*/
		virtual const TimestampVector &timestamps() const = 0;
		/**Size of an image*/
		virtual Size imageSize() const = 0;
		/**Read a raw (digital levels) or calibrated (temperature) image */
		virtual bool readImage(int pos, int calibration, unsigned short *pixels) = 0;
		/**Retrieve the raw (DL) value at given position for the last read image*/
		virtual bool getRawValue(int x, int y, unsigned short *value) const = 0;

		/**
		 * Returns the names of internal used tables (floating point matrices) used for calibration.
		 * By default, returns table defined by the calibration object.
		 */
		virtual StringList tableNames() const;
		virtual std::vector<float> getTable(const char *name) const;

		/**Returns video global attributes (if any)*/
		virtual const std::map<std::string, std::string> &globalAttributes() const
		{
			static std::map<std::string, std::string> empty_map;
			return empty_map;
		}
		/**Returns class type name*/
		virtual const char *typeName() const = 0;

		/**Returns calibration object attached to this loader (if any) */
		virtual BaseCalibration *calibration() const = 0;

		virtual bool setCalibration(BaseCalibration *calibration) = 0;

		/**Returns supported calibration for this video reader.
		For IR videos, this function should return something like ("Raw Data","Temperature").*/
		virtual StringList supportedCalibration() const = 0;
		/**Returns true if the reader is valid (opened properly and having images)*/
		virtual bool isValid() const = 0;
		/**Close the read. isValid() should return false after.*/
		virtual void close() {}
		/**Returns the video filename (if any).*/
		virtual std::string filename() const { return std::string(); }
		/**Returns frame attributes for the last loaded image*/
		virtual bool extractAttributes(std::map<std::string, std::string> &) const { return false; }

		/**
		Calibrate an image using this video calibration
		*/
		virtual bool calibrate(unsigned short * /*img*/, float * /*out*/, int /*size*/, int /*calibration*/) { return true; }
		/**
		Calibrate in-place an image using this video calibration
		*/
		virtual bool calibrateInplace(unsigned short * /*img*/, int /*size*/, int /*calibration*/) { return true; }

		/**Returns all instances of IRVideoLoader currently existing*/
		static std::vector<IRVideoLoader *> instances();
		static void closeAll();
	};

	/**
	 * Tool used to build a IRVideoLoader object from a video file.
	 */
	class IRVideoLoaderBuilder
	{
	public:
		virtual ~IRVideoLoaderBuilder() {}

		/**
		 * Returns the IRVideoLoader name (as returned by IRVideoLoader::name)
		 */
		virtual const char *typeName() const = 0;

		/**
		 * Tells if a IRVideoLoader be build from a filename (possibly NULL) and the first file bytes (possibly NULL)
		 */
		virtual bool probe(const char *filename, void *bytes, size_t size) const = 0;

		/**
		 * Build a IRVideoLoader object from a filename (possibly NULL) and the associated file reader object (possibly NULL).
		 * If fileReader is not NULL, the IRVideoLoader must use it but does NOT take ownership of it.
		 * Might return NULL on error.
		 */
		virtual IRVideoLoader *build(const char *filename, void *fileReader) const = 0;

		virtual IRVideoLoader *buildEmpty() const = 0;
	};

	/**
	 * Register a IRVideoLoaderBuilder object
	 */
	IO_EXPORT void registerIRVideoLoaderBuilder(IRVideoLoaderBuilder *builder);
	/**
	 * Returns the first found IRVideoLoaderBuilder object with given name
	 */
	IO_EXPORT const IRVideoLoaderBuilder *findIRVideoLoaderBuilder(const char *name);
	/**
	Build a IRVideoLoader object from a filename (possibly NULL) and a file reader object (possibly NULL).
	Internally scan all registered IRVideoLoaderBuilder and, based on the result of probe member, returns the first
	successfully built IRVideoLoader object.
	*/
	IO_EXPORT IRVideoLoader *buildIRVideoLoader(const char *filename, void *fileReader);
}

#endif
