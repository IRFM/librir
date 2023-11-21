#pragma once

#include <string>
#include <vector>

#include "rir_config.h"
#include "Misc.h"

/** @file
 */

namespace rir
{
	class IRVideoLoader;

	/**
	 * Base class for IR image calibration in surface temperature
	 */
	class BaseCalibration
	{
	public:
		enum Features
		{
			SupportGlobalEmissivity = 0x01,
			SupportPixelEmissivity = 0x02,
			SupportOpticalTemperature = 0x04,
			SupportSTEFITemperature = 0x08 // Only for WEST cameras
		};

		virtual ~BaseCalibration() {}
		virtual std::string name() const = 0;

		/**Return supported features as a combination of Features enum*/
		virtual int supportedFeatures() const = 0;

		/**Returns true if the calibration is valid, false otherwise*/
		virtual bool isValid() const = 0;
		/**Returns potential warnings (with new line separators) emitted while building the calibration*/
		virtual std::string error() const = 0;
		/**Returns potential errors (with new line separators) emitted while building the calibration*/
		virtual std::string warning() const = 0;
		/** Flip transmission factors. Default implementation does nothing.*/
		virtual bool flipTransmissions(bool fip_lr, bool flip_ud, int width, int height)
		{
			(void)fip_lr;
			(void)flip_ud;
			(void)width;
			(void)height;
			return true;
		}

		/**Returns the list of calibration files required for this calibration*/
		virtual StringList calibrationFiles() const = 0;

		/**Convert Digital Level value and Integration time to temperature (�C) for given integration time*/
		virtual unsigned rawDLToTemp(unsigned DL, int ti) const = 0;
		/**Convert Digital Level value and Integration time to temperature (�C) for given integration time*/
		virtual float rawDLToTempF(unsigned DL, int ti) const = 0;
		/**Convert temperature (�C) to Digital Level for given Integration time*/
		virtual unsigned tempToRawDL(unsigned temp, int ti) const = 0;
		virtual unsigned tempToRawDLF(float temp, int ti) const = 0;

		/**Returns optics temperature*/
		virtual unsigned short opticalTemperature() const = 0;
		/**Set optics temperature (B30 temperature)*/
		virtual void setOpticalTemperature(unsigned short) = 0;

		/**Returns STEFI temperature*/
		virtual unsigned short STEFITemperature() const = 0;
		/**Set STEFI temperature */
		virtual void setSTEFITemperature(unsigned short) = 0;

		/**
		Apply inverted calibration (from T�C to DL).
		IT is the image of integration time (can be NULL for some implementations)
		*/
		virtual bool applyInvert(const unsigned short *T, const unsigned char *IT, unsigned int size, unsigned short *out) const = 0;
		virtual int applyInvert(const unsigned short *T, const unsigned char *IT, int index) const = 0;
		/**
		Calibrate input Digital Levels to surface temperature (C).
		@param DL input DL image
		@param inv_emissivities inverted emissivities per pixel
		@param size input image size
		@param out output temperature image
		@param saturate if not NULL, set to true if the calibration saturated, false otherwise
		Returns true on success, false otherwise.
		*/
		virtual bool apply(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, unsigned short *out, bool *saturate = NULL) const = 0;

		/**
		Calibrate input Digital Levels to temperature.
		@param DL input DL image
		@param inv_emissivities inverted emissivities per pixel
		@param size input image size
		@param out output temperature image
		@param saturate if not NULL, set to true if the calibration saturated, false otherwise
		Returns true on success, false otherwise.

		This function performs a floating point calibration (precision < 1�C) and is slightly slower than #apply() function.
		*/
		virtual bool applyF(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, float *out, bool *saturate = NULL) const = 0;
	};

	/**
	 * Tool used to build a calibration object for a video file.
	 */
	class CalibrationBuilder
	{
	public:
		virtual ~CalibrationBuilder() {}

		/**
		 * Returns the calibration name (as returned by BaseCalibration::name)
		 */
		virtual std::string name() const = 0;

		/**
		 * Tells if a calibration object can be build from a filename (possibly NULL) and a IRVideoLoader object(possibly NULL)
		 */
		virtual bool probe(const char *filename, IRVideoLoader *loader) const = 0;
		/**
		 * Tells if a calibration object can be build from a pulse number, a view name and a IRVideoLoader object(possibly NULL)
		 */
		virtual bool probe(double pulse, const char *view, IRVideoLoader *loader) const = 0;
		/**
		 * Build a calibration object from a filename (possibly NULL) and a IRVideoLoader object(possibly NULL).
		 * Might return NULL on error.
		 */
		virtual BaseCalibration *build(const char *filename, IRVideoLoader *loader) const = 0;
		/**
		 * Build a calibration object from a pulse number, a camera view name and a IRVideoLoader object(possibly NULL).
		 * Might return NULL on error.
		 */
		virtual BaseCalibration *build(double pulse, const char *view, IRVideoLoader *loader) const = 0;

		virtual BaseCalibration *buildEmpty() const = 0;
	};

	/**
	 * Register a CalibrationBuilder object
	 */
	IO_EXPORT void registerCalibrationBuilder(CalibrationBuilder *builder);
	/**
	 * Returns the first found CalibrationBuilder object with given name
	 */
	IO_EXPORT const CalibrationBuilder *findCalibrationBuilder(const char *name);

	/**
	Build a calibration object from a filename (possibly NULL) and a IRVideoLoader object(possibly NULL).
	Internally scan all registered CalibrationBuilder and, based on the result of probe member, returns the first
	successfully built BaseCalibration object.
	*/
	IO_EXPORT BaseCalibration *buildCalibration(const char *filename, IRVideoLoader *loader);
	IO_EXPORT BaseCalibration *buildCalibration(double pulse, const char *view, IRVideoLoader *loader);

}