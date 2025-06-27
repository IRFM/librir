#ifndef BIN_FILE_LOADER_H
#define BIN_FILE_LOADER_H

#include <cstdint>
#include <cstddef>
#include <istream>
#include <functional>

#include "IRVideoLoader.h"
#include "BaseCalibration.h"
#include "ReadFileChunk.h"

/** @file
 */

//! unknown file type
#define BIN_FILE_UNKNOWN 0
//! bin video file using the BIN_HEADER and BIN_TRIGGER structures
#define BIN_FILE_BIN 1
//! pcr file using the PCR_HEADER structure
#define BIN_FILE_PCR 2
//! pcr file encapsulated in a bin file
#define BIN_FILE_PCR_ENCAPSULATED 3
//! video file compressed with zstd (see ZFile.h for more details)
#define BIN_FILE_Z_COMPRESSED 4
//! h264 video file
#define BIN_FILE_H264 5
//! HCC file
#define BIN_FILE_HCC 6
//! Other type
#define BIN_FILE_OTHER 7

#undef close

namespace rir
{
	class FileAttributes;

	/**
	File header for PCR files (old Tore Supra video file format)
	*/
#pragma pack(push, 1)
	typedef union PCR_HEADER
	{
		struct
		{
			int Version;
			int NbImages;
			int X;
			int Y;
			int Band;
			int Bits; // 8 or 16
			int Interlaced;
			int Frequency;
			int ImagesPerBuffer;
			int TransfertSize;
			int GrabSizeX;
			int GrabSizeY;
		};
		char reserved[1024];
	} PCR_HEADER;
#pragma pack(pop)

	/**
	A IR video reader that can read several IR video formats: PCR files, a BIN (WEST format) files, PCR encapsulated into a BIN file, H264 video files, or any additional formats
	defined using the IRVideoLoaderBuilder mechanism.
	*/
	using time_point = std::chrono::time_point<std::chrono::system_clock>;
	IO_EXPORT std::string tostring_binary32(unsigned value, unsigned size);
	IO_EXPORT std::string serializeTimePoint(const time_point &time, const std::string &format);
	class IO_EXPORT IRFileLoader : public IRVideoLoader
	{
	public:
		// typedef for motion correction function.
		// Signature: void(unsigned short * in_out_pixels, int width, int height, int im_pos)
		using motion_correction_function = std::function<void(unsigned short*, int, int, int)>;

		static int findFileType(std::istream *f, PCR_HEADER *infos, int64_t *start_images, int64_t *start_time, int *frame_count = NULL);
		static int findFileType(char *buf, PCR_HEADER *infos, int64_t *start_images, int64_t *start_time, int *frame_count = NULL);

		IRFileLoader();
		~IRFileLoader();

		bool open(const char *filename);
		bool openFileReader(const FileReaderPtr & file_reader);

		bool isPCR() const;
		bool isBIN() const;
		bool isPCREncapsulated() const;
		bool isZCompressed() const;
		bool isH264() const;
		bool isHCC() const;
		bool hasTimes() const;
		bool hasCalibration() const;
		bool is_in_T() const;
		virtual CalibrationPtr calibration() const;
		virtual bool setCalibration(const CalibrationPtr &calibration);

		virtual bool supportBadPixels() const { return true; }
		virtual void setBadPixelsEnabled(bool enable);
		virtual bool badPixelsEnabled() const;

		virtual void setEmissivity(float emi);
		virtual bool setEmissivities(const float* emi, size_t size);
		virtual bool setInvEmissivities(const float* inv_emi, size_t size);

		virtual const char *typeName() const { return "IRFileLoader"; }
		virtual bool saturate() const;
		virtual std::string filename() const;
		virtual int size() const;
		virtual const TimestampVector &timestamps() const;
		virtual Size imageSize() const;
		virtual bool readImage(int pos, int calibration, unsigned short *pixels);
		virtual bool readImageF(int pos, int calibration, float* pixels);
		virtual StringList supportedCalibration() const;
		virtual bool isValid() const { return timestamps().size() > 0; }
		virtual bool getRawValue(int x, int y, unsigned short *value) const;
		virtual bool calibrate(unsigned short *img, float *out, int size, int calibration);
		virtual bool calibrateInplace(unsigned short *img, int size, int calibration);
		virtual const std::map<std::string, std::string> &globalAttributes() const;
		virtual bool extractAttributes(std::map<std::string, std::string> &) const;
		virtual void close();

		virtual bool loadTranslationFile(const char *filename);
		virtual void enableMotionCorrection(bool);
		virtual bool motionCorrectionEnabled() const;

		motion_correction_function motionCorrectionFunction() const;
		void setMotionCorrectionFunction(const motion_correction_function& fun);

		void setAttributes(const dict_type& attrs);

		const FileAttributes* fileAttributes() const;

		void removeBadPixels(unsigned short *img, int w, int h);
		void removeMotion(unsigned short *img, int w, int h, int pos);

		const IRVideoLoader* internalLoader() const;

	private:
		class PrivateData;
		PrivateData *m_data;
	};

}



#endif
