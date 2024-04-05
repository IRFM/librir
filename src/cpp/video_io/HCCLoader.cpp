
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

	void populate_map_with_header(rir::dict_type mapper, HCCImageHeader &h)
	{
		mapper.clear();

		/* 0d000 */
		mapper["Signature"] = toString(h.Signature);
		mapper["DeviceXMLMinorVersion"] = toString(h.DeviceXMLMinorVersion);
		mapper["DeviceXMLMajorVersion"] = toString(h.DeviceXMLMajorVersion);
		mapper["ImageHeaderLength"] = toString(h.ImageHeaderLength);
		mapper["FrameID"] = toString(h.FrameID);
		mapper["DataOffset"] = toString(h.DataOffset);
		mapper["DataExp"] = toString((int)h.DataExp);
		mapper["ExposureTime"] = toString(h.ExposureTime * 1e-8);
		mapper["CalibrationMode"] = toString(h.CalibrationMode);
		mapper["BPRApplied"] = toString(h.BPRApplied);
		mapper["FrameBufferMode"] = toString(h.FrameBufferMode);
		mapper["CalibrationBlockIndex"] = toString((int)h.CalibrationBlockIndex);
		mapper["Width"] = toString(h.Width);
		mapper["Height"] = toString(h.Height);
		mapper["OffsetX"] = toString(h.OffsetX);
		mapper["OffsetY"] = toString(h.OffsetY);
		mapper["ReverseX"] = toString(h.ReverseX);
		mapper["ReverseY"] = toString(h.ReverseY);
		mapper["TestImageSelector"] = toString(h.TestImageSelector);
		mapper["SensorWellDepth"] = toString(h.SensorWellDepth);
		mapper["AcquisitionFrameRate"] = toString(h.AcquisitionFrameRate);
		mapper["TriggerDelay"] = toString(h.TriggerDelay);
		mapper["TriggerMode"] = toString(h.TriggerMode);
		mapper["TriggerSource"] = toString(h.TriggerSource);
		mapper["IntegrationMode"] = toString(h.IntegrationMode);
		mapper["AveragingNumber"] = toString(h.AveragingNumber);
		mapper["ExposureAuto"] = toString(h.ExposureAuto);
		mapper["AECResponseTime"] = toString(h.AECResponseTime);
		mapper["AECImageFraction"] = toString(h.AECImageFraction);
		mapper["AECTargetWellFilling"] = toString(h.AECTargetWellFilling);
		mapper["FWMode"] = toString(h.FWMode);
		mapper["FWSpeedSetpoint"] = toString(h.FWSpeedSetpoint);
		mapper["FWSpeed"] = toString(h.FWSpeed);
		mapper["POSIXTime"] = toString(h.POSIXTime);
		mapper["SubSecondTime"] = toString(h.SubSecondTime);
		mapper["TimeSource"] = toString(h.TimeSource);
		mapper["GPSModeIndicator"] = toString(h.GPSModeIndicator);
		mapper["GPSLongitude"] = toString(h.GPSLongitude);
		mapper["GPSLatitude"] = toString(h.GPSLatitude);
		mapper["GPSAltitude"] = toString(h.GPSAltitude);
		mapper["FWEncoderAtExposureStart"] = toString(h.FWEncoderAtExposureStart);
		mapper["FWEncoderAtExposureEnd"] = toString(h.FWEncoderAtExposureEnd);
		mapper["FWPosition"] = toString(h.FWPosition);
		mapper["ICUPosition"] = toString(h.ICUPosition);
		mapper["NDFilterPosition"] = toString(h.NDFilterPosition);
		mapper["EHDRIExposureIndex"] = toString(h.EHDRIExposureIndex);
		mapper["FrameFlag"] = toString(h.FrameFlag);
		mapper["PostProcessed"] = toString(h.PostProcessed);
		mapper["SensorTemperatureRaw"] = toString(h.SensorTemperatureRaw);
		mapper["AlarmVector"] = toString(h.AlarmVector);
		mapper["ExternalBlackBodyTemperature"] = toString(h.ExternalBlackBodyTemperature);
		mapper["TemperatureSensor"] = toString(h.TemperatureSensor);
		mapper["TemperatureInternalLens"] = toString(h.TemperatureInternalLens);
		mapper["TemperatureExternalLens"] = toString(h.TemperatureExternalLens);
		mapper["TemperatureInternalCalibrationUnit"] = toString(h.TemperatureInternalCalibrationUnit);
		mapper["TemperatureExternalThermistor"] = toString(h.TemperatureExternalThermistor);
		mapper["TemperatureFilterWheel"] = toString(h.TemperatureFilterWheel);
		mapper["TemperatureCompressor"] = toString(h.TemperatureCompressor);
		mapper["TemperatureColdFinger"] = toString(h.TemperatureColdFinger);
		mapper["CalibrationBlockPOSIXTime"] = toString(h.CalibrationBlockPOSIXTime);
		mapper["ExternalLensSerialNumber"] = toString(h.ExternalLensSerialNumber);
		mapper["ManualFilterSerialNumber"] = toString(h.ManualFilterSerialNumber);
		mapper["SensorID"] = toString(h.SensorID);
		mapper["PixelDataResolution"] = toString(h.PixelDataResolution);
		mapper["DeviceCalibrationFilesMajorVersion"] = toString(h.DeviceCalibrationFilesMajorVersion);
		mapper["DeviceCalibrationFilesMinorVersion"] = toString(h.DeviceCalibrationFilesMinorVersion);
		mapper["DeviceCalibrationFilesSubMinorVersion"] = toString(h.DeviceCalibrationFilesSubMinorVersion);
		mapper["DeviceDataFlowMajorVersion"] = toString(h.DeviceDataFlowMajorVersion);
		mapper["DeviceDataFlowMinorVersion"] = toString(h.DeviceDataFlowMinorVersion);
		mapper["DeviceFirmwareMajorVersion"] = toString(h.DeviceFirmwareMajorVersion);
		mapper["DeviceFirmwareMinorVersion"] = toString(h.DeviceFirmwareMinorVersion);
		mapper["DeviceFirmwareSubMinorVersion"] = toString(h.DeviceFirmwareSubMinorVersion);
		mapper["DeviceFirmwareBuildVersion"] = toString(h.DeviceFirmwareBuildVersion);
		mapper["ActualizationPOSIXTime"] = toString(h.ActualizationPOSIXTime);
		mapper["DeviceSerialNumber"] = toString(h.DeviceSerialNumber);
		mapper["ExposureTime (s)"] = toString(h.ExposureTime * 1e-8);
		mapper["TriggerDelay (us)"] = toString(h.TriggerDelay);
		mapper["AECResponseTime (ms)"] = toString(h.AECResponseTime);
		mapper["AECImageFraction (%)"] = toString(h.AECImageFraction);
		mapper["AECTargetWellFilling (%)"] = toString(h.AECTargetWellFilling);
		mapper["PostProcessed"] = h.PostProcessed == 1 ? "Yes" : "No";
		mapper["TemperatureSensor (cC)"] = toString(h.TemperatureSensor);
		mapper["TemperatureInternalLens (cC)"] = toString(h.TemperatureInternalLens);
		mapper["TemperatureExternalLens (cC)"] = toString(h.TemperatureExternalLens);
		mapper["TemperatureInternalCalibrationUnit (cC)"] = toString(h.TemperatureInternalCalibrationUnit);
		mapper["TemperatureExternalThermistor (cC)"] = toString(h.TemperatureExternalThermistor);
		mapper["TemperatureFilterWheel (cC)"] = toString(h.TemperatureFilterWheel);
		mapper["TemperatureCompressor (cC)"] = toString(h.TemperatureCompressor);
		mapper["TemperatureColdFinger (cC)"] = toString(h.TemperatureColdFinger);
		mapper["FWPosition"] = toString((int)h.FWPosition);
		mapper["ExternalBlackBodyTemperature (cC)"] = toString(h.ExternalBlackBodyTemperature);

		mapper["Header"] = std::string((char *)&h, sizeof(h));
	}
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
		populate_map_with_header(d_data->attributes, d_data->header);

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

		populate_map_with_header(d_data->imageAttributes, h);

		d_data->imageAttributes["Type"] = toString(type);
		// d_data->imageAttributes["ExposureTime (s)"] = toString(h.ExposureTime * 1e-8);
		// d_data->imageAttributes["TriggerDelay (us)"] = toString(h.TriggerDelay);
		// d_data->imageAttributes["AECResponseTime (ms)"] = toString(h.AECResponseTime);
		// d_data->imageAttributes["AECImageFraction (%)"] = toString(h.AECImageFraction);
		// d_data->imageAttributes["AECTargetWellFilling (%)"] = toString(h.AECTargetWellFilling);
		// d_data->imageAttributes["PostProcessed"] = h.PostProcessed == 1 ? "Yes" : "No";
		// d_data->imageAttributes["TemperatureSensor (cC)"] = toString(h.TemperatureSensor);
		// d_data->imageAttributes["TemperatureInternalLens (cC)"] = toString(h.TemperatureInternalLens);
		// d_data->imageAttributes["TemperatureExternalLens (cC)"] = toString(h.TemperatureExternalLens);
		// d_data->imageAttributes["TemperatureInternalCalibrationUnit (cC)"] = toString(h.TemperatureInternalCalibrationUnit);
		// d_data->imageAttributes["TemperatureExternalThermistor (cC)"] = toString(h.TemperatureExternalThermistor);
		// d_data->imageAttributes["TemperatureFilterWheel (cC)"] = toString(h.TemperatureFilterWheel);
		// d_data->imageAttributes["TemperatureCompressor (cC)"] = toString(h.TemperatureCompressor);
		// d_data->imageAttributes["TemperatureColdFinger (cC)"] = toString(h.TemperatureColdFinger);
		// d_data->imageAttributes["FWPosition"] = toString((int)h.FWPosition);
		// d_data->imageAttributes["ExternalBlackBodyTemperature (cC)"] = toString(h.ExternalBlackBodyTemperature);

		// d_data->imageAttributes["Header"] = std::string((char *)&h, sizeof(h));

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
