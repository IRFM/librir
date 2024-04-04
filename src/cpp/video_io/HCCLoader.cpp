
// Libraries includes
#include <cstdint>
#include <chrono>
#include <iomanip>

#include "ReadFileChunk.h"
#include "HCCLoader.h"
#include "Log.h"
#include "Misc.h"

namespace rir
{

	class HCCLoader::PrivateData
	{
	public:
		void *file;
		bool own;
		bool badPixelsEnabled;
		bool saturate;
		Size imSize;
		TimestampVector timestamps;
		HCCImageHeader header;
		HCCImageHeader imageHeader;
		std::string filename;
		std::map<std::string, std::string> attributes;
		std::map<std::string, std::string> imageAttributes;
		std::vector<unsigned short> image;
		PrivateData() : file(NULL), own(false), badPixelsEnabled(false), saturate(false)
		{
			memset(&header, 0, sizeof(header));
			memset(&imageHeader, 0, sizeof(imageHeader));
		}
	};

	HCCLoader::HCCLoader()
	{
		d_data = new PrivateData();
	}
	HCCLoader::~HCCLoader()
	{
		close();
		delete d_data;
	}

	bool HCCLoader::open(const char *filename)
	{
		close();
		void *p = createFileReader(createFileAccess(filename));
		if (!p)
			return false;
		d_data->filename = filename;
		if (openFileReader(p, true))
			return true;

		destroyFileReader(p);
		return false;
	}

	bool HCCLoader::openFileReader(void *file_reader, bool own)
	{
		if (d_data->filename.empty())
			close();

		seekFile(file_reader, 0, SEEK_SET);
		std::uint64_t file_size = fileSize(file_reader);

		if (file_size < (int)sizeof(HCCImageHeader))
		{
			logError("Error reading HCC file : invalid file size");
			return false;
		}

		readFile(file_reader, &d_data->header, sizeof(d_data->header));

		if (d_data->header.ImageHeaderLength > 6000 || d_data->header.Signature[0] != 'T' || d_data->header.Signature[1] != 'C')
		{
			logError("Wrong file format");
			return false;
		}

		double sampling = 1 / (d_data->header.AcquisitionFrameRate / 1000.0);
		sampling *= 1000000000;

		d_data->imSize = Size(d_data->header.Width, d_data->header.Height);
		int frame_size = d_data->header.ImageHeaderLength + d_data->header.Width * d_data->header.Height * 2;
		int frame_count = file_size / frame_size;

		std::uint64_t start_time = ((double)d_data->header.POSIXTime + d_data->header.SubSecondTime * 1e-7) * 1000;
		// QDateTime dt = QDateTime::fromMSecsSinceEpoch(start_time);
		std::chrono::system_clock::time_point tp{std::chrono::milliseconds{start_time}};
		auto in_time_t = std::chrono::system_clock::to_time_t(tp);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%d/%m/%Y %H:%M:%S");

		d_data->attributes["Date"] = ss.str();
		d_data->attributes["Size"] = toString(frame_count);

		d_data->timestamps.resize(frame_count);
		double start = 0;
		for (int i = 0; i < frame_count; i++)
		{
			d_data->timestamps[i] = static_cast<std::int64_t>(start);
			start += sampling;
		}
		/* 0d000 */
		d_data->attributes["Signature"] = toString(d_data->header.Signature);
		d_data->attributes["DeviceXMLMinorVersion"] = toString(d_data->header.DeviceXMLMinorVersion);
		d_data->attributes["DeviceXMLMajorVersion"] = toString(d_data->header.DeviceXMLMajorVersion);
		d_data->attributes["ImageHeaderLength"] = toString(d_data->header.ImageHeaderLength);
		d_data->attributes["FrameID"] = toString(d_data->header.FrameID);
		d_data->attributes["DataOffset"] = toString(d_data->header.DataOffset);
		d_data->attributes["DataExp"] = toString(d_data->header.DataExp);
		d_data->attributes["ExposureTime"] = toString(d_data->header.ExposureTime);
		d_data->attributes["CalibrationMode"] = toString(d_data->header.CalibrationMode);
		d_data->attributes["BPRApplied"] = toString(d_data->header.BPRApplied);
		d_data->attributes["FrameBufferMode"] = toString(d_data->header.FrameBufferMode);
		d_data->attributes["CalibrationBlockIndex"] = toString(d_data->header.CalibrationBlockIndex);
		d_data->attributes["Width"] = toString(d_data->header.Width);
		d_data->attributes["Height"] = toString(d_data->header.Height);
		d_data->attributes["OffsetX"] = toString(d_data->header.OffsetX);
		d_data->attributes["OffsetY"] = toString(d_data->header.OffsetY);
		d_data->attributes["ReverseX"] = toString(d_data->header.ReverseX);
		d_data->attributes["ReverseY"] = toString(d_data->header.ReverseY);
		d_data->attributes["TestImageSelector"] = toString(d_data->header.TestImageSelector);
		d_data->attributes["SensorWellDepth"] = toString(d_data->header.SensorWellDepth);
		d_data->attributes["AcquisitionFrameRate"] = toString(d_data->header.AcquisitionFrameRate);
		d_data->attributes["TriggerDelay"] = toString(d_data->header.TriggerDelay);
		d_data->attributes["TriggerMode"] = toString(d_data->header.TriggerMode);
		d_data->attributes["TriggerSource"] = toString(d_data->header.TriggerSource);
		d_data->attributes["IntegrationMode"] = toString(d_data->header.IntegrationMode);
		d_data->attributes["AveragingNumber"] = toString(d_data->header.AveragingNumber);
		d_data->attributes["ExposureAuto"] = toString(d_data->header.ExposureAuto);
		d_data->attributes["AECResponseTime"] = toString(d_data->header.AECResponseTime);
		d_data->attributes["AECImageFraction"] = toString(d_data->header.AECImageFraction);
		d_data->attributes["AECTargetWellFilling"] = toString(d_data->header.AECTargetWellFilling);
		d_data->attributes["FWMode"] = toString(d_data->header.FWMode);
		d_data->attributes["FWSpeedSetpoint"] = toString(d_data->header.FWSpeedSetpoint);
		d_data->attributes["FWSpeed"] = toString(d_data->header.FWSpeed);
		d_data->attributes["POSIXTime"] = toString(d_data->header.POSIXTime);
		d_data->attributes["SubSecondTime"] = toString(d_data->header.SubSecondTime);
		d_data->attributes["TimeSource"] = toString(d_data->header.TimeSource);
		d_data->attributes["GPSModeIndicator"] = toString(d_data->header.GPSModeIndicator);
		d_data->attributes["GPSLongitude"] = toString(d_data->header.GPSLongitude);
		d_data->attributes["GPSLatitude"] = toString(d_data->header.GPSLatitude);
		d_data->attributes["GPSAltitude"] = toString(d_data->header.GPSAltitude);
		d_data->attributes["FWEncoderAtExposureStart"] = toString(d_data->header.FWEncoderAtExposureStart);
		d_data->attributes["FWEncoderAtExposureEnd"] = toString(d_data->header.FWEncoderAtExposureEnd);
		d_data->attributes["FWPosition"] = toString(d_data->header.FWPosition);
		d_data->attributes["ICUPosition"] = toString(d_data->header.ICUPosition);
		d_data->attributes["NDFilterPosition"] = toString(d_data->header.NDFilterPosition);
		d_data->attributes["EHDRIExposureIndex"] = toString(d_data->header.EHDRIExposureIndex);
		d_data->attributes["FrameFlag"] = toString(d_data->header.FrameFlag);
		d_data->attributes["PostProcessed"] = toString(d_data->header.PostProcessed);
		d_data->attributes["SensorTemperatureRaw"] = toString(d_data->header.SensorTemperatureRaw);
		d_data->attributes["AlarmVector"] = toString(d_data->header.AlarmVector);
		d_data->attributes["ExternalBlackBodyTemperature"] = toString(d_data->header.ExternalBlackBodyTemperature);
		d_data->attributes["TemperatureSensor"] = toString(d_data->header.TemperatureSensor);
		d_data->attributes["TemperatureInternalLens"] = toString(d_data->header.TemperatureInternalLens);
		d_data->attributes["TemperatureExternalLens"] = toString(d_data->header.TemperatureExternalLens);
		d_data->attributes["TemperatureInternalCalibrationUnit"] = toString(d_data->header.TemperatureInternalCalibrationUnit);
		d_data->attributes["TemperatureExternalThermistor"] = toString(d_data->header.TemperatureExternalThermistor);
		d_data->attributes["TemperatureFilterWheel"] = toString(d_data->header.TemperatureFilterWheel);
		d_data->attributes["TemperatureCompressor"] = toString(d_data->header.TemperatureCompressor);
		d_data->attributes["TemperatureColdFinger"] = toString(d_data->header.TemperatureColdFinger);
		d_data->attributes["CalibrationBlockPOSIXTime"] = toString(d_data->header.CalibrationBlockPOSIXTime);
		d_data->attributes["ExternalLensSerialNumber"] = toString(d_data->header.ExternalLensSerialNumber);
		d_data->attributes["ManualFilterSerialNumber"] = toString(d_data->header.ManualFilterSerialNumber);
		d_data->attributes["SensorID"] = toString(d_data->header.SensorID);
		d_data->attributes["PixelDataResolution"] = toString(d_data->header.PixelDataResolution);
		d_data->attributes["DeviceCalibrationFilesMajorVersion"] = toString(d_data->header.DeviceCalibrationFilesMajorVersion);
		d_data->attributes["DeviceCalibrationFilesMinorVersion"] = toString(d_data->header.DeviceCalibrationFilesMinorVersion);
		d_data->attributes["DeviceCalibrationFilesSubMinorVersion"] = toString(d_data->header.DeviceCalibrationFilesSubMinorVersion);
		d_data->attributes["DeviceDataFlowMajorVersion"] = toString(d_data->header.DeviceDataFlowMajorVersion);
		d_data->attributes["DeviceDataFlowMinorVersion"] = toString(d_data->header.DeviceDataFlowMinorVersion);
		d_data->attributes["DeviceFirmwareMajorVersion"] = toString(d_data->header.DeviceFirmwareMajorVersion);
		d_data->attributes["DeviceFirmwareMinorVersion"] = toString(d_data->header.DeviceFirmwareMinorVersion);
		d_data->attributes["DeviceFirmwareSubMinorVersion"] = toString(d_data->header.DeviceFirmwareSubMinorVersion);
		d_data->attributes["DeviceFirmwareBuildVersion"] = toString(d_data->header.DeviceFirmwareBuildVersion);
		d_data->attributes["ActualizationPOSIXTime"] = toString(d_data->header.ActualizationPOSIXTime);
		d_data->attributes["DeviceSerialNumber"] = toString(d_data->header.DeviceSerialNumber);

		d_data->attributes["ExposureTime (s)"] = toString(d_data->header.ExposureTime * 1e-8);
		d_data->attributes["TriggerDelay (us)"] = toString(d_data->header.TriggerDelay);
		d_data->attributes["AECResponseTime (ms)"] = toString(d_data->header.AECResponseTime);
		d_data->attributes["AECImageFraction (%)"] = toString(d_data->header.AECImageFraction);
		d_data->attributes["AECTargetWellFilling (%)"] = toString(d_data->header.AECTargetWellFilling);
		d_data->attributes["PostProcessed"] = toString(d_data->header.PostProcessed == 1 ? "Yes" : "No");
		d_data->attributes["TemperatureSensor (cC)"] = toString(d_data->header.TemperatureSensor);
		d_data->attributes["TemperatureInternalLens (cC)"] = toString(d_data->header.TemperatureInternalLens);
		d_data->attributes["TemperatureExternalLens (cC)"] = toString(d_data->header.TemperatureExternalLens);
		d_data->attributes["TemperatureInternalCalibrationUnit (cC)"] = toString(d_data->header.TemperatureInternalCalibrationUnit);
		d_data->attributes["TemperatureExternalThermistor (cC)"] = toString(d_data->header.TemperatureExternalThermistor);
		d_data->attributes["TemperatureFilterWheel (cC)"] = toString(d_data->header.TemperatureFilterWheel);
		d_data->attributes["TemperatureCompressor (cC)"] = toString(d_data->header.TemperatureCompressor);
		d_data->attributes["TemperatureColdFinger (cC)"] = toString(d_data->header.TemperatureColdFinger);
		d_data->attributes["ExternalBlackBodyTemperature (cC)"] = toString(d_data->header.ExternalBlackBodyTemperature);

		d_data->attributes["FileHeader"] = std::string((char *)&d_data->header, sizeof(d_data->header));

		d_data->file = file_reader;
		d_data->own = own;

		return true;
	}

	void HCCLoader::setExternalBlackBodyTemperature(float temperature)
	{
		d_data->header.ExternalBlackBodyTemperature = temperature;
	}

	void HCCLoader::setBadPixelsEnabled(bool enable)
	{
		d_data->badPixelsEnabled = enable;
	}
	bool HCCLoader::badPixelsEnabled() const
	{
		return d_data->badPixelsEnabled;
	}

	bool HCCLoader::saturate() const
	{
		return d_data->saturate;
	}
	std::string HCCLoader::filename() const
	{
		return d_data->filename;
	}
	int HCCLoader::size() const
	{
		return (int)d_data->timestamps.size();
	}
	const TimestampVector &HCCLoader::timestamps() const
	{
		return d_data->timestamps;
	}
	Size HCCLoader::imageSize() const
	{
		return d_data->imSize;
	}

	bool HCCLoader::readImage(int pos, int calibration, unsigned short *pixels)
	{
		if (!isValid())
			return false;

		std::int64_t frame_size = d_data->header.ImageHeaderLength + d_data->header.Width * d_data->header.Height * 2;
		std::int64_t offset = frame_size * pos;

		if (seekFile(d_data->file, offset, SEEK_SET) < 0)
			return false;

		// read image header
		HCCImageHeader h;
		readFile(d_data->file, &h, sizeof(h));

		if (seekFile(d_data->file, offset + d_data->header.ImageHeaderLength, SEEK_SET) < 0)
			return false;

		if (d_data->image.size() != d_data->header.Height * d_data->header.Width)
			d_data->image.resize(d_data->header.Height * d_data->header.Width);

		// Read raw image
		if (readFile(d_data->file, d_data->image.data(), d_data->header.Height * d_data->header.Width * 2) <= 0)
			return false;

		// find type and unit
		std::string type;
		// std::string unit;
		// bool convert = false;
		// bool K_temp = false;
		bool correct_bad_pixels = d_data->badPixelsEnabled;

		if (d_data->header.CalibrationMode == 0)
		{
			type = "Raw0";
			// unit = "DL";
		}
		else if (d_data->header.CalibrationMode == 1)
		{
			type = "NUC";
			// unit = "DL";
		}
		else if (d_data->header.CalibrationMode == 2)
		{
			type = "RT";
			// unit = "Radiometric Temperature (C)";
			// convert = propertyAt(0)->data().value<int>();
			// K_temp = true;
		}
		else if (d_data->header.CalibrationMode == 3)
		{
			type = "IBR";
			// unit = "W/(m2 sr)";
			// convert = propertyAt(0)->data().value<int>();
		}
		else if (d_data->header.CalibrationMode == 4)
		{
			type = "IBI";
			// unit = "W/(m2 sr)";
			// convert = propertyAt(0)->data().value<int>();
		}
		else if (d_data->header.CalibrationMode == 5)
		{
			type = "Raw";
			// unit = "DL";
		}

		float factor = std::pow(2, d_data->header.DataExp);
		int invalid_pixels = 0;

		// convert image to the right unit
		unsigned short *pix = (unsigned short *)d_data->image.data();
		const bool little_endian = is_little_endian();

		if (correct_bad_pixels || !little_endian)
		{

			for (int y = 0; y < d_data->header.Height; ++y)
				for (int x = 0; x < d_data->header.Width; ++x)
				{
					const int index = x + y * d_data->header.Width;
					// raw data are stored in little endian
					unsigned short p = pix[index];
					if (!little_endian)
						pix[index] = p = swap_uint16(p);

					if (correct_bad_pixels && p >= 0xFFF1)
					{

						// invalid pixel, replace with closest if possible
						++invalid_pixels;

						// find a valid pixel to the left
						int _x = x;
						while (x > 0)
						{
							--_x;
							if (pix[_x + y * d_data->header.Width] < 0xFFF1)
							{
								p = pix[_x + y * d_data->header.Width];
								break;
							}
						}
						if (p >= 0xFFF1)
						{
							// find a valid pixel to the right
							int _x = x;
							while (x < d_data->header.Width - 1)
							{
								++_x;
								if (pix[_x + y * d_data->header.Width] < 0xFFF1)
								{
									p = pix[_x + y * d_data->header.Width];
									break;
								}
							}
							if (p >= 0xFFF1)
							{
								// find a valid pixel to the top
								int _y = y;
								while (_y > 0)
								{
									--_y;
									if (pix[x + _y * d_data->header.Width] < 0xFFF1)
									{
										p = pix[x + _y * d_data->header.Width];
										break;
									}
								}
								if (p >= 0xFFF1)
								{
									int _y = y;
									while (_y < d_data->header.Height - 1)
									{
										++_y;
										if (pix[x + _y * d_data->header.Width] < 0xFFF1)
										{
											p = pix[x + _y * d_data->header.Width];
											break;
										}
									}
								}
							}
						}
						pix[index] = p;
					}
				}
		}

		memcpy(pixels, pix, d_data->header.Height * d_data->header.Width * 2);

		d_data->imageAttributes.clear();

		d_data->imageAttributes["Signature"] = toString(h.Signature);
		d_data->imageAttributes["DeviceXMLMinorVersion"] = toString(h.DeviceXMLMinorVersion);
		d_data->imageAttributes["DeviceXMLMajorVersion"] = toString(h.DeviceXMLMajorVersion);
		d_data->imageAttributes["ImageHeaderLength"] = toString(h.ImageHeaderLength);
		d_data->imageAttributes["FrameID"] = toString(h.FrameID);
		d_data->imageAttributes["DataOffset"] = toString(h.DataOffset);
		d_data->imageAttributes["DataExp"] = toString(h.DataExp);
		d_data->imageAttributes["ExposureTime"] = toString(h.ExposureTime);
		d_data->imageAttributes["CalibrationMode"] = toString(h.CalibrationMode);
		d_data->imageAttributes["BPRApplied"] = toString(h.BPRApplied);
		d_data->imageAttributes["FrameBufferMode"] = toString(h.FrameBufferMode);
		d_data->imageAttributes["CalibrationBlockIndex"] = toString((int)h.CalibrationBlockIndex);
		d_data->imageAttributes["Width"] = toString(h.Width);
		d_data->imageAttributes["Height"] = toString(h.Height);
		d_data->imageAttributes["OffsetX"] = toString(h.OffsetX);
		d_data->imageAttributes["OffsetY"] = toString(h.OffsetY);
		d_data->imageAttributes["ReverseX"] = toString(h.ReverseX);
		d_data->imageAttributes["ReverseY"] = toString(h.ReverseY);
		d_data->imageAttributes["TestImageSelector"] = toString(h.TestImageSelector);
		d_data->imageAttributes["SensorWellDepth"] = toString(h.SensorWellDepth);
		d_data->imageAttributes["AcquisitionFrameRate"] = toString(h.AcquisitionFrameRate);
		d_data->imageAttributes["TriggerDelay"] = toString(h.TriggerDelay);
		d_data->imageAttributes["TriggerMode"] = toString(h.TriggerMode);
		d_data->imageAttributes["TriggerSource"] = toString(h.TriggerSource);
		d_data->imageAttributes["IntegrationMode"] = toString(h.IntegrationMode);
		d_data->imageAttributes["AveragingNumber"] = toString(h.AveragingNumber);
		d_data->imageAttributes["ExposureAuto"] = toString(h.ExposureAuto);
		d_data->imageAttributes["AECResponseTime"] = toString(h.AECResponseTime);
		d_data->imageAttributes["AECImageFraction"] = toString(h.AECImageFraction);
		d_data->imageAttributes["AECTargetWellFilling"] = toString(h.AECTargetWellFilling);
		d_data->imageAttributes["FWMode"] = toString(h.FWMode);
		d_data->imageAttributes["FWSpeedSetpoint"] = toString(h.FWSpeedSetpoint);
		d_data->imageAttributes["FWSpeed"] = toString(h.FWSpeed);
		d_data->imageAttributes["POSIXTime"] = toString(h.POSIXTime);
		d_data->imageAttributes["SubSecondTime"] = toString(h.SubSecondTime);
		d_data->imageAttributes["TimeSource"] = toString(h.TimeSource);
		d_data->imageAttributes["GPSModeIndicator"] = toString(h.GPSModeIndicator);
		d_data->imageAttributes["GPSLongitude"] = toString(h.GPSLongitude);
		d_data->imageAttributes["GPSLatitude"] = toString(h.GPSLatitude);
		d_data->imageAttributes["GPSAltitude"] = toString(h.GPSAltitude);
		d_data->imageAttributes["FWEncoderAtExposureStart"] = toString(h.FWEncoderAtExposureStart);
		d_data->imageAttributes["FWEncoderAtExposureEnd"] = toString(h.FWEncoderAtExposureEnd);
		d_data->imageAttributes["FWPosition"] = toString(h.FWPosition);
		d_data->imageAttributes["ICUPosition"] = toString(h.ICUPosition);
		d_data->imageAttributes["NDFilterPosition"] = toString(h.NDFilterPosition);
		d_data->imageAttributes["EHDRIExposureIndex"] = toString(h.EHDRIExposureIndex);
		d_data->imageAttributes["FrameFlag"] = toString(h.FrameFlag);
		d_data->imageAttributes["PostProcessed"] = toString(h.PostProcessed);
		d_data->imageAttributes["SensorTemperatureRaw"] = toString(h.SensorTemperatureRaw);
		d_data->imageAttributes["AlarmVector"] = toString(h.AlarmVector);
		d_data->imageAttributes["ExternalBlackBodyTemperature"] = toString(h.ExternalBlackBodyTemperature);
		d_data->imageAttributes["TemperatureSensor"] = toString(h.TemperatureSensor);
		d_data->imageAttributes["TemperatureInternalLens"] = toString(h.TemperatureInternalLens);
		d_data->imageAttributes["TemperatureExternalLens"] = toString(h.TemperatureExternalLens);
		d_data->imageAttributes["TemperatureInternalCalibrationUnit"] = toString(h.TemperatureInternalCalibrationUnit);
		d_data->imageAttributes["TemperatureExternalThermistor"] = toString(h.TemperatureExternalThermistor);
		d_data->imageAttributes["TemperatureFilterWheel"] = toString(h.TemperatureFilterWheel);
		d_data->imageAttributes["TemperatureCompressor"] = toString(h.TemperatureCompressor);
		d_data->imageAttributes["TemperatureColdFinger"] = toString(h.TemperatureColdFinger);
		d_data->imageAttributes["CalibrationBlockPOSIXTime"] = toString(h.CalibrationBlockPOSIXTime);
		d_data->imageAttributes["ExternalLensSerialNumber"] = toString(h.ExternalLensSerialNumber);
		d_data->imageAttributes["ManualFilterSerialNumber"] = toString(h.ManualFilterSerialNumber);
		d_data->imageAttributes["SensorID"] = toString(h.SensorID);
		d_data->imageAttributes["PixelDataResolution"] = toString(h.PixelDataResolution);
		d_data->imageAttributes["DeviceCalibrationFilesMajorVersion"] = toString(h.DeviceCalibrationFilesMajorVersion);
		d_data->imageAttributes["DeviceCalibrationFilesMinorVersion"] = toString(h.DeviceCalibrationFilesMinorVersion);
		d_data->imageAttributes["DeviceCalibrationFilesSubMinorVersion"] = toString(h.DeviceCalibrationFilesSubMinorVersion);
		d_data->imageAttributes["DeviceDataFlowMajorVersion"] = toString(h.DeviceDataFlowMajorVersion);
		d_data->imageAttributes["DeviceDataFlowMinorVersion"] = toString(h.DeviceDataFlowMinorVersion);
		d_data->imageAttributes["DeviceFirmwareMajorVersion"] = toString(h.DeviceFirmwareMajorVersion);
		d_data->imageAttributes["DeviceFirmwareMinorVersion"] = toString(h.DeviceFirmwareMinorVersion);
		d_data->imageAttributes["DeviceFirmwareSubMinorVersion"] = toString(h.DeviceFirmwareSubMinorVersion);
		d_data->imageAttributes["DeviceFirmwareBuildVersion"] = toString(h.DeviceFirmwareBuildVersion);
		d_data->imageAttributes["ActualizationPOSIXTime"] = toString(h.ActualizationPOSIXTime);
		d_data->imageAttributes["DeviceSerialNumber"] = toString(h.DeviceSerialNumber);

		d_data->imageAttributes["Type"] = toString(type);
		d_data->imageAttributes["ExposureTime (s)"] = toString(h.ExposureTime * 1e-8);
		d_data->imageAttributes["TriggerDelay (us)"] = toString(h.TriggerDelay);
		d_data->imageAttributes["AECResponseTime (ms)"] = toString(h.AECResponseTime);
		d_data->imageAttributes["AECImageFraction (%)"] = toString(h.AECImageFraction);
		d_data->imageAttributes["AECTargetWellFilling (%)"] = toString(h.AECTargetWellFilling);
		d_data->imageAttributes["PostProcessed"] = h.PostProcessed == 1 ? "Yes" : "No";
		d_data->imageAttributes["TemperatureSensor (cC)"] = toString(h.TemperatureSensor);
		d_data->imageAttributes["TemperatureInternalLens (cC)"] = toString(h.TemperatureInternalLens);
		d_data->imageAttributes["TemperatureExternalLens (cC)"] = toString(h.TemperatureExternalLens);
		d_data->imageAttributes["TemperatureInternalCalibrationUnit (cC)"] = toString(h.TemperatureInternalCalibrationUnit);
		d_data->imageAttributes["TemperatureExternalThermistor (cC)"] = toString(h.TemperatureExternalThermistor);
		d_data->imageAttributes["TemperatureFilterWheel (cC)"] = toString(h.TemperatureFilterWheel);
		d_data->imageAttributes["TemperatureCompressor (cC)"] = toString(h.TemperatureCompressor);
		d_data->imageAttributes["TemperatureColdFinger (cC)"] = toString(h.TemperatureColdFinger);
		d_data->imageAttributes["FWPosition"] = toString((int)h.FWPosition);
		d_data->imageAttributes["ExternalBlackBodyTemperature (cC)"] = toString(h.ExternalBlackBodyTemperature);

		d_data->imageAttributes["Header"] = std::string((char *)&h, sizeof(h));

		return true;
	}

	StringList HCCLoader::supportedCalibration() const
	{
		return StringList();
	}

	bool HCCLoader::getRawValue(int x, int y, unsigned short *value) const
	{
		int index = x + y * imageSize().width;
		if (index < 0 || index >= (int)d_data->image.size())
			return false;
		*value = d_data->image[index];
		return true;
	}

	bool HCCLoader::calibrate(unsigned short *img, float *out, int size, int calibration)
	{
		(void)img;
		(void)out;
		(void)size;
		(void)calibration;
		return false;
	}

	bool HCCLoader::calibrateInplace(unsigned short *img, int size, int calibration)
	{
		(void)img;
		(void)size;
		(void)calibration;
		return false;
	}

	const std::map<std::string, std::string> &HCCLoader::globalAttributes() const
	{
		return d_data->attributes;
	}

	bool HCCLoader::extractAttributes(std::map<std::string, std::string> &out) const
	{
		out = d_data->imageAttributes;
		return true;
	}

	void HCCLoader::close()
	{
		if (d_data->file)
		{
			if (d_data->own)
			{
				destroyFileReader(d_data->file);
			}
		}

		delete d_data;
		d_data = new PrivateData();
	}

}
