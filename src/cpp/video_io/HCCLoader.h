#pragma once

#include "IRVideoLoader.h"
#include "ReadFileChunk.h"

/** @file
 */

namespace rir
{

	// Image header structure in HCC files
#pragma pack(1)
	typedef struct
	{
		char Signature[2]; /* 0d000 */
		std::uint8_t DeviceXMLMinorVersion;
		std::uint8_t DeviceXMLMajorVersion;
		std::uint16_t ImageHeaderLength;
		char free[2];
		std::uint32_t FrameID;
		float DataOffset;
		std::int8_t DataExp;
		char free_[7];
		std::uint32_t ExposureTime;
		std::uint8_t CalibrationMode;
		std::uint8_t BPRApplied;
		std::uint8_t FrameBufferMode;
		std::uint8_t CalibrationBlockIndex;
		std::uint16_t Width;
		std::uint16_t Height;
		std::uint16_t OffsetX;
		std::uint16_t OffsetY;
		std::uint8_t ReverseX;
		std::uint8_t ReverseY;
		std::uint8_t TestImageSelector;
		std::uint8_t SensorWellDepth;
		std::uint32_t AcquisitionFrameRate;
		float TriggerDelay;
		std::uint8_t TriggerMode;
		std::uint8_t TriggerSource;
		std::uint8_t IntegrationMode;
		char free_2[1];
		std::uint8_t AveragingNumber;
		char free_3[2];
		std::uint8_t ExposureAuto;
		float AECResponseTime;
		float AECImageFraction;
		float AECTargetWellFilling;
		char free_4[3];
		std::uint8_t FWMode;
		std::uint16_t FWSpeedSetpoint;
		std::uint16_t FWSpeed;
		char free_5[20];
		std::uint32_t POSIXTime;
		std::uint32_t SubSecondTime;
		std::uint8_t TimeSource;
		char free_6[2];
		std::uint8_t GPSModeIndicator;
		std::int32_t GPSLongitude;
		std::int32_t GPSLatitude;
		std::int32_t GPSAltitude;
		std::uint16_t FWEncoderAtExposureStart;
		std::uint16_t FWEncoderAtExposureEnd;
		std::uint8_t FWPosition;
		std::uint8_t ICUPosition;
		std::uint8_t NDFilterPosition;
		std::uint8_t EHDRIExposureIndex;
		std::uint8_t FrameFlag;
		std::uint8_t PostProcessed;
		std::uint16_t SensorTemperatureRaw;
		std::uint32_t AlarmVector;
		char free_7[16];
		float ExternalBlackBodyTemperature;
		std::int16_t TemperatureSensor;
		char free_8[2];
		std::int16_t TemperatureInternalLens;
		std::int16_t TemperatureExternalLens;
		std::int16_t TemperatureInternalCalibrationUnit;
		char free_89[10];
		std::int16_t TemperatureExternalThermistor;
		std::int16_t TemperatureFilterWheel;
		std::int16_t TemperatureCompressor;
		std::int16_t TemperatureColdFinger;
		char free_10[24];
		std::uint32_t CalibrationBlockPOSIXTime;
		std::uint32_t ExternalLensSerialNumber;
		std::uint32_t ManualFilterSerialNumber;
		std::uint8_t SensorID;
		std::uint8_t PixelDataResolution;
		char free_11[9];
		std::uint8_t DeviceCalibrationFilesMajorVersion;
		std::uint8_t DeviceCalibrationFilesMinorVersion;
		std::uint8_t DeviceCalibrationFilesSubMinorVersion;
		std::uint8_t DeviceDataFlowMajorVersion;
		std::uint8_t DeviceDataFlowMinorVersion;
		std::uint8_t DeviceFirmwareMajorVersion;
		std::uint8_t DeviceFirmwareMinorVersion;
		std::uint8_t DeviceFirmwareSubMinorVersion;
		std::uint8_t DeviceFirmwareBuildVersion;
		std::uint32_t ActualizationPOSIXTime;
		std::uint32_t DeviceSerialNumber;
		char free_12[4];

	} HCCImageHeader;

#pragma pack()

	/// @brief Loader for HCC video files
	class IO_EXPORT HCCLoader : public IRVideoLoader
	{
	public:
		HCCLoader();
		virtual ~HCCLoader();

		bool open(const char *filename);
		bool openFileReader(const FileReaderPtr & reader);

		virtual bool supportBadPixels() const { return true; }
		virtual void setBadPixelsEnabled(bool enable);
		virtual bool badPixelsEnabled() const;

		virtual const char *typeName() const { return "HCCLoader"; }
		virtual bool saturate() const;
		virtual std::string filename() const;
		virtual int size() const;
		virtual const TimestampVector &timestamps() const;
		virtual Size imageSize() const;
		virtual bool readImage(int pos, int calibration, unsigned short *pixels);
		virtual StringList supportedCalibration() const;
		virtual bool isValid() const { return timestamps().size() > 0; }
		virtual bool getRawValue(int x, int y, unsigned short *value) const;
		virtual bool calibrate(unsigned short *img, float *out, int size, int calibration);
		virtual bool calibrateInplace(unsigned short *img, int size, int calibration);
		virtual bool setCalibration(const CalibrationPtr&) { return false; }
		virtual CalibrationPtr calibration() const { return CalibrationPtr(); }
		virtual const std::map<std::string, std::string> &globalAttributes() const;
		virtual bool extractAttributes(std::map<std::string, std::string> &) const;
		virtual void close();

		void setExternalBlackBodyTemperature(float temperature);
		double samplingTimeNs() const;
	private:
		class PrivateData;
		PrivateData *d_data;
	};


	class IRFileLoader;
	IO_EXPORT bool HCC_extractTimesAndFWPos(const IRFileLoader* loader, std::int64_t* times, int* pos);
	IO_EXPORT bool HCC_extractAllFWPos(const IRFileLoader* loader, int* pos, int* pos_count);
}
