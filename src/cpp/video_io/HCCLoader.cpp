
// Libraries includes
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <fstream>

#include "ReadFileChunk.h"
#include "HCCLoader.h"
#include "Log.h"
#include "Misc.h"
#include "IRFileLoader.h"
#include "FileAttributes.h"

namespace rir
{

	void HCCLoader::populate_map_with_header(dict_type &mapper, const HCCImageHeader &h)
	{
		/* 0d000 */
		mapper["Signature"] = toString(h.Signature);
		mapper["DeviceXMLMinorVersion"] = toString((std::uint16_t)h.DeviceXMLMinorVersion);
		mapper["DeviceXMLMajorVersion"] = toString((std::uint16_t)h.DeviceXMLMajorVersion);
		mapper["ImageHeaderLength"] = toString(h.ImageHeaderLength);
		mapper["FrameID"] = toString(h.FrameID);
		mapper["DataOffset"] = toString(h.DataOffset);
		mapper["DataExp"] = toString((std::uint16_t)h.DataExp);
		mapper["ExposureTime"] = toString(h.ExposureTime * 1e-8);
		mapper["CalibrationMode"] = toString((std::uint16_t)h.CalibrationMode);
		mapper["BPRApplied"] = toString((std::uint16_t)h.BPRApplied);
		mapper["FrameBufferMode"] = toString((std::uint16_t)h.FrameBufferMode);
		mapper["CalibrationBlockIndex"] = toString((std::uint16_t)h.CalibrationBlockIndex);
		mapper["Width"] = toString(h.Width);
		mapper["Height"] = toString(h.Height);
		mapper["OffsetX"] = toString(h.OffsetX);
		mapper["OffsetY"] = toString(h.OffsetY);
		mapper["ReverseX"] = toString((std::uint16_t)h.ReverseX);
		mapper["ReverseY"] = toString((std::uint16_t)h.ReverseY);
		mapper["TestImageSelector"] = toString((std::uint16_t)h.TestImageSelector);
		mapper["SensorWellDepth"] = toString((std::uint16_t)h.SensorWellDepth);
		mapper["AcquisitionFrameRate"] = toString(h.AcquisitionFrameRate);
		mapper["TriggerDelay"] = toString(h.TriggerDelay);
		mapper["TriggerMode"] = toString((std::uint16_t)h.TriggerMode);
		mapper["TriggerSource"] = toString((std::uint16_t)h.TriggerSource);
		mapper["IntegrationMode"] = toString((std::uint16_t)h.IntegrationMode);
		mapper["AveragingNumber"] = toString((std::uint16_t)h.AveragingNumber);
		mapper["ExposureAuto"] = toString((std::uint16_t)h.ExposureAuto);
		mapper["AECResponseTime"] = toString(h.AECResponseTime);
		mapper["AECImageFraction"] = toString(h.AECImageFraction);
		mapper["AECTargetWellFilling"] = toString(h.AECTargetWellFilling);
		mapper["FWMode"] = toString((std::uint16_t)h.FWMode);
		mapper["FWSpeedSetpoint"] = toString(h.FWSpeedSetpoint);
		mapper["FWSpeed"] = toString(h.FWSpeed);
		mapper["POSIXTime"] = toString(h.POSIXTime);
		mapper["SubSecondTime"] = toString(h.SubSecondTime);
		mapper["TimeSource"] = toString((std::uint16_t)h.TimeSource);
		mapper["GPSModeIndicator"] = toString((std::uint16_t)h.GPSModeIndicator);
		mapper["GPSLongitude"] = toString(h.GPSLongitude);
		mapper["GPSLatitude"] = toString(h.GPSLatitude);
		mapper["GPSAltitude"] = toString(h.GPSAltitude);
		mapper["FWEncoderAtExposureStart"] = toString(h.FWEncoderAtExposureStart);
		mapper["FWEncoderAtExposureEnd"] = toString(h.FWEncoderAtExposureEnd);
		mapper["FWPosition"] = toString((std::uint16_t)h.FWPosition);
		mapper["ICUPosition"] = toString((std::uint16_t)h.ICUPosition);
		mapper["NDFilterPosition"] = toString((std::uint16_t)h.NDFilterPosition);
		mapper["EHDRIExposureIndex"] = toString((std::uint16_t)h.EHDRIExposureIndex);
		mapper["FrameFlag"] = toString((std::uint16_t)h.FrameFlag);
		mapper["PostProcessed"] = toString((std::uint16_t)h.PostProcessed);
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
		mapper["SensorID"] = toString((std::uint16_t)h.SensorID);
		mapper["PixelDataResolution"] = toString((std::uint16_t)h.PixelDataResolution);
		mapper["DeviceCalibrationFilesMajorVersion"] = toString((std::uint16_t)h.DeviceCalibrationFilesMajorVersion);
		mapper["DeviceCalibrationFilesMinorVersion"] = toString((std::uint16_t)h.DeviceCalibrationFilesMinorVersion);
		mapper["DeviceCalibrationFilesSubMinorVersion"] = toString((std::uint16_t)h.DeviceCalibrationFilesSubMinorVersion);
		mapper["DeviceDataFlowMajorVersion"] = toString((std::uint16_t)h.DeviceDataFlowMajorVersion);
		mapper["DeviceDataFlowMinorVersion"] = toString((std::uint16_t)h.DeviceDataFlowMinorVersion);
		mapper["DeviceFirmwareMajorVersion"] = toString((std::uint16_t)h.DeviceFirmwareMajorVersion);
		mapper["DeviceFirmwareMinorVersion"] = toString((std::uint16_t)h.DeviceFirmwareMinorVersion);
		mapper["DeviceFirmwareSubMinorVersion"] = toString((std::uint16_t)h.DeviceFirmwareSubMinorVersion);
		mapper["DeviceFirmwareBuildVersion"] = toString((std::uint16_t)h.DeviceFirmwareBuildVersion);
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
		mapper["FWPosition"] = toString((std::uint16_t)h.FWPosition);
		mapper["ExternalBlackBodyTemperature (cC)"] = toString(h.ExternalBlackBodyTemperature);

		mapper["Header"] = std::string((char *)&h, sizeof(h));
	}
	class HCCLoader::PrivateData
	{
	public:
		FileReaderPtr file;
		bool badPixelsEnabled = false;
		bool saturate = false;
		bool isEAST = false;
		double sampling_ns = 0;
		Size imSize;
		TimestampVector timestamps;
		HCCImageHeader header;
		HCCImageHeader imageHeader;
		std::string filename;
		std::map<std::string, std::string> attributes;
		std::map<std::string, std::string> imageAttributes;
		std::vector<unsigned short> image;
		PrivateData() : badPixelsEnabled(false), saturate(false)
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

	double HCCLoader::samplingTimeNs() const
	{
		return d_data->sampling_ns;
	}

	bool HCCLoader::open(const char *filename)
	{
		close();
		auto p = createFileReader(createFileAccess(filename));
		if (!p)
			return false;
		d_data->filename = filename;
		if (openFileReader(p))
			return true;
		return false;
	}

	bool HCCLoader::openFileReader(const FileReaderPtr & reader)
	{
		if (d_data->filename.empty())
			close();

		auto file = reader;
		seekFile(file, 0, SEEK_SET);
		std::uint64_t file_size = fileSize(file);

		if (file_size < (int)sizeof(HCCImageHeader))
		{
			logError("Error reading HCC file : invalid file size");
			return false;
		}

		readFile(file, &d_data->header, sizeof(d_data->header));

		if (d_data->header.ImageHeaderLength > 6000 || d_data->header.Signature[0] != 'T' || d_data->header.Signature[1] != 'C')
		{
			logError("Wrong file format");
			return false;
		}

		double sampling = 1 / (d_data->header.AcquisitionFrameRate / 1000.0);
		sampling *= 1000000000;
		d_data->sampling_ns = sampling;
		d_data->imSize = Size(d_data->header.Width, d_data->header.Height);
		int frame_size = d_data->header.ImageHeaderLength + d_data->header.Width * d_data->header.Height * 2;
		int frame_count = file_size / frame_size;

		d_data->isEAST = d_data->header.Width == 640 && d_data->header.Height == 512;
		if (d_data->isEAST)
			d_data->attributes["Device"] = "EAST";

		// Start time in milliseconds since epoch
		std::uint64_t start_time = ((double)d_data->header.POSIXTime + d_data->header.SubSecondTime * 1e-7) * 1000;
		// QDateTime dt = QDateTime::fromMSecsSinceEpoch(start_time);
		std::chrono::system_clock::time_point tp{std::chrono::milliseconds{start_time}};
		auto in_time_t = std::chrono::system_clock::to_time_t(tp);
		std::stringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%d/%m/%Y %H:%M:%S");

		d_data->attributes["Date"] = ss.str();
		d_data->attributes["Size"] = toString(frame_count);
		d_data->attributes["Type"] = "HCC";

		if (d_data->isEAST)
		{
			// find type and unit
			std::string type;
			std::string unit;
			bool K_temp = false;
			bool correct_bad_pixels = d_data->badPixelsEnabled;

			if (d_data->header.CalibrationMode == 0)
			{
				type = "Raw0";
				unit = "DL";
			}
			else if (d_data->header.CalibrationMode == 1)
			{
				type = "NUC";
				unit = "DL";
			}
			else if (d_data->header.CalibrationMode == 2)
			{
				type = "RT";
				unit = "Radiometric Temperature (C)";
				K_temp = true;
			}
			else if (d_data->header.CalibrationMode == 3)
			{
				type = "IBR";
				unit = "W/(m2 sr)";
			}
			else if (d_data->header.CalibrationMode == 4)
			{
				type = "IBI";
				unit = "W/(m2 sr)";
			}
			else if (d_data->header.CalibrationMode == 5)
			{
				type = "Raw";
				unit = "DL";
			}

			d_data->attributes["DataType"] = type;
			d_data->attributes["DataUnit"] = unit;
		}

		d_data->timestamps.resize(frame_count);
		for (int i = 0; i < frame_count; i++)
			d_data->timestamps[i] = static_cast<std::int64_t>(i * sampling);
		
		populate_map_with_header(d_data->attributes, d_data->header);

		d_data->file = reader;

		return true;
	}

	void HCCLoader::setExternalBlackBodyTemperature(float temperature)
	{
		d_data->header.ExternalBlackBodyTemperature = temperature;
		d_data->imageHeader.ExternalBlackBodyTemperature = temperature;
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

		
		// convert image to the right unit
		unsigned short *pix = (unsigned short *)d_data->image.data();
		
		memcpy(pixels, pix, d_data->header.Height * d_data->header.Width * 2);

		d_data->imageAttributes.clear();
		populate_map_with_header(d_data->imageAttributes, h);


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
		delete d_data;
		d_data = new PrivateData();
	}


	bool HCC_extractTimesAndFWPos(const IRFileLoader* loader, std::int64_t* times, int* pos)
	{
		std::string fname = loader->filename();
		if (fname.empty())
			return false;

		if (loader->isH264() ) {
			const FileAttributes* attrs = loader->fileAttributes();
			
			size_t count = attrs->size();
			for (size_t i = 0; i < count; ++i) {
				times[i] = attrs->timestamp(i);

				const auto& dict = attrs->attributes(i);
				auto it = dict.find("FWPosition");
				if (it == dict.end())
					return false;
				pos[i] = fromString<int>(it->second);
			}
			return true;
		}

		if (loader->isHCC()) {

			std::ifstream fin(fname.c_str(), std::ios::binary);
			HCCImageHeader h;
			fin.read((char*)&h, sizeof(h));

			size_t frame_size = h.ImageHeaderLength + h.Width * h.Height * 2;
			double sampling = 1 / (h.AcquisitionFrameRate / 1000.0);
			sampling *= 1000000000;

			for (int i = 0; i < loader->size(); ++i) {
				fin.seekg((std::uint64_t)i * frame_size);
				fin.read((char*)&h, sizeof(h));
				times[i] = (std::int64_t)(i * sampling);
				pos[i] = h.FWPosition;
			}
			return true;
		}
		return false;
	}

	bool HCC_extractAllFWPos(const IRFileLoader* loader, int* pos, int* pos_count)
	{
		*pos_count = 0;
		std::string fname = loader->filename();
		if (fname.empty())
			return false;
		
		if (loader->isH264()) {

			const FileAttributes* attrs = loader->fileAttributes();
			
			size_t count = attrs->size();
			for (size_t i = 0; i < count; ++i) {

				const auto& dict = attrs->attributes(i);
				auto it = dict.find("FWPosition");
				if (it == dict.end())
					return false;
				pos[(*pos_count)] = fromString<int>(it->second);
				if (*pos_count && pos[(*pos_count)] == pos[0])
					break;
				else
					++(*pos_count);
			}
			std::sort(pos, pos + *pos_count);
			return true;
		}
		if (loader->isHCC()) {

			std::ifstream fin(fname.c_str(), std::ios::binary);
			HCCImageHeader h;
			fin.read((char*)&h, sizeof(h));

			size_t frame_size = h.ImageHeaderLength + h.Width * h.Height * 2;
			double sampling = 1 / (h.AcquisitionFrameRate / 1000.0);
			sampling *= 1000000000;
			*pos_count = 0;
			for (int i = 0; i < loader->size(); ++i) {
				fin.seekg((std::uint64_t)i * frame_size);
				fin.read((char*)&h, sizeof(h));
				pos[(*pos_count)] = h.FWPosition;
				if (*pos_count && pos[(*pos_count)] == pos[0])
					break;
				else
					++(*pos_count);
			}
			return true;
		}
		return false;
	}

}

