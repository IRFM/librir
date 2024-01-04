#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <bitset>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <memory>

#include "IRFileLoader.h"
#include "BaseCalibration.h"
#include "h264.h"
#include <mutex>

#include "HCCLoader.h"
// #include "ZFile.h"
#include "Log.h"
#include "Filters.h"
#include "ReadFileChunk.h"
#include "SIMD.h"

namespace rir
{

	// using namespace std::chrono;
	using time_point = std::chrono::time_point<std::chrono::system_clock>;

	std::string serializeTimePoint(const time_point &time, const std::string &format)
	{
		static std::mutex mu;
		std::tm *tmp = NULL;
		std::tm tm;

		{
			std::unique_lock<std::mutex> lock(mu);
			std::time_t tt = std::chrono::system_clock::to_time_t(time);
			tmp = std::gmtime(&tt);
			// Add check on  std::gmtime result for invalid time point
			if (!tmp)
				return std::string();
			tm = *tmp; // GMT (UTC)
		}
		// std::tm tm = *std::localtime(&tt); //Locale time-zone, usually UTC by default.
		std::stringstream ss;
		ss << std::put_time(&tm, format.c_str());
		return ss.str();
	}

	struct BinFile
	{
		uint32_t width, height;
		uint32_t count;
		int64_t start;
		int64_t transferSize;
		std::vector<int64_t> times;
		void *zfile;
		void *file; // file reader created with createFileReader
		H264_Loader h264;
		HCCLoader hcc;
		std::unique_ptr<IRVideoLoader> other;
		int type;
		int has_times;
		std::vector<float> pixels;
	};

	typedef union BIN_HEADER
	{
		struct
		{
			uint8_t version;
			uint8_t triggers;
			uint8_t compression;
		};
		char reserved[128];
	} BIN_HEADER;

	typedef union BIN_TRIGGER
	{
		struct
		{
			int64_t date;
			int64_t rate;
			int64_t samples;
			int64_t samples_pre_trigger;
			int64_t type;
			int64_t nb_channels;
			int64_t data_type;
			int64_t data_format;
			int64_t data_repetition;
			int64_t data_size_x;
			int64_t data_size_y;
		};
		char reserved[128];
	} BIN_TRIGGER;

	std::string tostring_binary32(unsigned value, unsigned size)
	{
		std::string res = std::bitset<32>(value).to_string();
		if (res.size() > size)
			return res.substr(res.size() - size, size);
		else
			return res;
	}

	int IRFileLoader::findFileType(char *buf, PCR_HEADER *infos, int64_t *start_images, int64_t *start_time, int *frame_count)
	{
		// try to open bin file
		BIN_HEADER bin_header;
		memcpy(&bin_header, buf, sizeof(bin_header));

		// check for mp4 file
		char ftyp[5];
		memset(ftyp, 0, sizeof(ftyp));
		memcpy(ftyp, ((char *)&bin_header) + 4, 4);
		if (strcmp(ftyp, "ftyp") == 0)
			return BIN_FILE_H264;

		PCR_HEADER pcr_header;
		memcpy(&pcr_header, buf, sizeof(pcr_header));

		PCR_HEADER pcr_enc; //= (PCR_HEADER*)(bytes + 128 + 5);
							// fseek(f, 128 + 5, SEEK_SET);
							// fread(&pcr_enc, 1, sizeof(pcr_enc), f);
		memcpy(&pcr_enc, buf + 128 + 5, sizeof(pcr_enc));

		// test PCR file
		if (pcr_header.Bits == 16 && ((abs(pcr_header.TransfertSize - (pcr_header.X * pcr_header.Y * 2)) < 2000) /*|| pcr_header.Frequency == 50 || pcr_header.Frequency == 25*/) && pcr_header.X > 0 && pcr_header.Y > 0 && pcr_header.X < 2000 && pcr_header.Y < 2000)
		{
			/*if (((abs(pcr_header.TransfertSize - (pcr_header.X * pcr_header.Y * 2)) > 2000)))
			pcr_header.TransfertSize = pcr_header.X * pcr_header.Y * 2;*/
			// wrong transfer size, correct it
			if (infos)
				*infos = pcr_header;
			if (start_images)
				*start_images = sizeof(PCR_HEADER);
			if (start_time)
				*start_time = 0;
			if (frame_count)
				*frame_count = pcr_header.NbImages;
			return BIN_FILE_PCR;
		}
		// test PCR encapsulated
		if (pcr_enc.Bits == 16 && ((abs(pcr_enc.TransfertSize - (pcr_enc.X * pcr_enc.Y * 2)) < 2000) /*|| pcr_enc.Frequency == 50 || pcr_enc.Frequency == 25*/) &&
			pcr_enc.X > 0 && pcr_enc.Y > 0 && pcr_enc.X < 1000 && pcr_enc.Y < 1000)
		{
			/*if (((abs(pcr_enc.TransfertSize - (pcr_enc.X * pcr_enc.Y * 2)) > 2000)))
			pcr_enc.TransfertSize = pcr_enc.X * pcr_enc.Y * 2;*/
			// wrong transfer size, correct it
			if (infos)
				*infos = pcr_enc;
			if (start_images)
				*start_images = sizeof(PCR_HEADER) + 128 + 5;
			if (start_time)
				*start_time = 0;
			if (frame_count)
				*frame_count = pcr_enc.NbImages;
			return BIN_FILE_PCR_ENCAPSULATED;
		}
		// test BIN file (WEST format)
		if (bin_header.version < 10 && bin_header.compression == 0 && bin_header.triggers == 1)
		{
			BIN_TRIGGER bin_trigger; // = (BIN_TRIGGER*)(bytes+ sizeof(BIN_HEADER));
									 // fseek(f, sizeof(BIN_HEADER), SEEK_SET);
									 // fread(&bin_trigger, 1, sizeof(bin_trigger), f);
			memcpy(&bin_trigger, buf + sizeof(BIN_HEADER), sizeof(bin_trigger));

			if (bin_trigger.data_size_x > 0 && bin_trigger.data_size_x < 1000 && bin_trigger.data_size_y > 0 && bin_trigger.data_size_y < 1000 && bin_trigger.rate > 0 && bin_trigger.rate < 1000)
			{
				if (infos)
				{
					infos->X = (int)bin_trigger.data_size_x;
					infos->Y = (int)bin_trigger.data_size_y;
					infos->TransfertSize = infos->X * infos->Y * 2;
					infos->Frequency = (int)bin_trigger.rate;
					infos->Bits = 16;
				}
				if (start_images)
					*start_images = sizeof(BIN_HEADER) + sizeof(BIN_TRIGGER);
				if (start_time)
					*start_time = bin_trigger.date;
				if (frame_count)
					*frame_count = (int)bin_trigger.samples;
				return BIN_FILE_BIN;
			}
		}
		// test Z compressed file
		if (bin_header.version == 1 && (bin_header.compression >= 1 && bin_header.compression <= 3) && bin_header.triggers == 1)
		{
			BIN_TRIGGER bin_trigger; // = (BIN_TRIGGER*)(bytes + sizeof(BIN_HEADER));
									 // fseek(f, sizeof(BIN_HEADER), SEEK_SET);
									 // fread(&bin_trigger, 1, sizeof(bin_trigger), f);

			memcpy(&bin_trigger, buf + sizeof(BIN_HEADER), sizeof(bin_trigger));

			if (bin_trigger.data_size_x > 0 && bin_trigger.data_size_x < 3000 && bin_trigger.data_size_y > 0 && bin_trigger.data_size_y < 3000 && bin_trigger.rate > 0 && bin_trigger.rate < 1000)
			{
				if (infos)
				{
					infos->X = (int)bin_trigger.data_size_x;
					infos->Y = (int)bin_trigger.data_size_y;
					infos->TransfertSize = infos->X * infos->Y * 2;
					infos->Frequency = (int)bin_trigger.rate;
					infos->Bits = 16;
				}
				if (frame_count)
					*frame_count = (int)bin_trigger.samples;
				return BIN_FILE_Z_COMPRESSED;
			}
		}

		std::string tmp(buf, 1000);
		if (tmp.find("H.265") != std::string::npos)
			return BIN_FILE_H264;
		if (tmp.find("matroska") != std::string::npos)
			return BIN_FILE_H264;
		if (tmp.find("G@") == 0)
			return BIN_FILE_H264; // TS format

		// TEST HCC file
		if (buf[0] == 'T' && buf[1] == 'C')
			return BIN_FILE_HCC;

		return BIN_FILE_UNKNOWN;
	}
	int IRFileLoader::findFileType(std::istream *f, PCR_HEADER *infos, int64_t *start_images, int64_t *start_time, int *frame_count)
	{
		char buf[2000];
		f->read(buf, sizeof(buf));
		return findFileType(buf, infos, start_images, start_time, frame_count);
	}

	static int findTimes(BinFile *f, int64_t *times)
	{
		// f->file.rdbuf()->pubsetbuf(0, 0);
		// for new PCR format, the timestamps are stored within the last 8 bytes of each image
		for (unsigned int i = 0; i < f->count; ++i)
		{
			// f->file.seekg((f->start + f->transferSize * (int64_t)(i+1) - 8));
			seekFile(f->file, f->start + f->transferSize * (int64_t)(i + 1) - 8, AVSEEK_SET);
			int64_t t = 0;

			if (readFile(f->file, (char *)&t, 8) != 8)
				return 0;
			// if (!f->file.read((char*)&t, 8))
			//	return 0;

			times[i] = t;
			if (i > 0)
			{
				if (times[i] <= times[i - 1] /*|| times[i] > times[i - 1] + 20 * 20*/)
					return 0;
			}
		}

		/*int64_t t0 = times[0];
		for (unsigned int i = 0; i < f->count; ++i)
			times[i] -= t0;*/
		return 1;
	}

	static BinFile *bin_open_file_from_file_reader(const char *filename, void *reader)
	{
		if (!reader)
			return NULL;
		BinFile *f = new BinFile();
		f->start = (0);
		f->zfile = (NULL);
		f->width = f->height = f->count = 0;
		f->transferSize = 0;
		f->zfile = NULL;
		f->file = NULL;
		// f->file.close();
		f->type = 0;
		f->has_times = 0;

		// f->file.open(filename, std::ios::binary);
		f->file = reader;
		if (!f->file)
		{
			delete f;
			return NULL;
		}

		PCR_HEADER infos;
		int64_t start_image, start_time;
		char buf[2000];
		readFile(f->file, buf, 2000);
		f->type = IRFileLoader::findFileType((char *)buf, &infos, &start_image, &start_time);

		if (f->type == BIN_FILE_UNKNOWN)
		{
			if (IRVideoLoader *l = buildIRVideoLoader(filename, reader))
			{
				f->type = BIN_FILE_OTHER;
				f->times = l->timestamps();
				f->width = l->imageSize().width;
				f->height = l->imageSize().height;
				f->count = l->size();
				f->has_times = 1;
				f->other = std::move(std::unique_ptr<IRVideoLoader>(l));
			}
			else
			{
				destroyFileReader(f->file);
				delete f;
				f = NULL;
			}
		}
		else if (f->type == BIN_FILE_H264)
		{
			if (!f->h264.open(f->file))
			{
				destroyFileReader(f->file);
				delete f;
				f = NULL;
				goto end;
			}
			f->times = f->h264.timestamps();
			f->width = f->h264.imageSize().width;
			f->height = f->h264.imageSize().height;
			f->count = f->h264.size();
			f->has_times = 1;
		}
		else if (f->type == BIN_FILE_HCC)
		{
			if (!f->hcc.openFileReader(f->file, false))
			{
				destroyFileReader(f->file);
				delete f;
				f = NULL;
				goto end;
			}
			f->times = f->hcc.timestamps();
			f->width = f->hcc.imageSize().width;
			f->height = f->hcc.imageSize().height;
			f->count = f->hcc.size();
			f->has_times = 1;
		}
		else
		{
			// obtain file size:
			size_t s = fileSize(f->file);

			if (infos.TransfertSize)
				f->count = (unsigned)((s - start_image) / (infos.TransfertSize));
			if (f->count == 0)
			{
				// f->file.close();
				destroyFileReader(f->file);
				delete f;
				f = NULL;
				goto end;
			}
			f->start = start_image;
			f->transferSize = infos.TransfertSize;
			f->width = infos.X;
			f->height = infos.Y;
			f->times.resize(f->count); // = (int64_t*)malloc(f->count * sizeof(int64_t));
			f->has_times = findTimes(f, f->times.data());
			if (!f->has_times)
			{
				if (infos.Frequency <= 0)
					infos.Frequency = 50;
				double sampling = 1000000000.0 / (double)infos.Frequency;
				for (uint32_t i = 0; i < f->count; ++i)
					f->times[i] = (int64_t)(i * sampling);
			}
			else
			{

				int64_t t0 = f->times[0];

				if (t0 > 28000 && t0 < 32000)
				{
					// the time is in ms since origin, do NOT subtract first timestamp
					// remove 10ms to have a precision of +- 10ms
					for (unsigned int i = 0; i < f->count; ++i)
						f->times[i] = (f->times[i] * (int64_t)1000000); // -10000000ULL; //convert to ns
				}
				else
				{
					if (!(f->times.front() < -1000000000 || f->times.back() > 1000000000))
					{ // time already in ns
						for (unsigned int i = 0; i < f->count; ++i)
							f->times[i] = (f->times[i] - t0) * (int64_t)1000000; // convert to ns
					}
				}
			}
		}

	end:

		if (!f)
			return f;

		if (f->type != BIN_FILE_OTHER)
		{
			// try to get timestamps strating at -2s for WEST videos only
			if (f->times.size() && f->times.front() > 28000000000LL && f->times.front() < 32000000000LL)
			{
				for (size_t i = 0; i < f->times.size(); ++i)
					f->times[i] -= 32000000000LL;
			}
		}
		return f;
	}

	static BinFile *bin_open_file_read(const char *filename)
	{
		return bin_open_file_from_file_reader(filename, createFileReader(createFileAccess(filename)));
	}

	static void bin_close_file(BinFile *f)
	{

		f->h264.close();
		f->hcc.close();
		if (f->other)
			f->other->close();
		f->other.reset();
		if (f->file)
			destroyFileReader(f->file);
		delete f;
	}

	static int bin_has_times(BinFile *f)
	{
		return f->has_times;
	}

	static int bin_image_count(BinFile *f)
	{
		return f->count;
	}

	static int bin_file_type(BinFile *f)
	{
		return f->type;
	}

	static int bin_image_size(BinFile *f, int *width, int *height)
	{
		*width = f->width;
		*height = f->height;
		return 0;
	}

	static int bin_read_image(BinFile *f, int pos, unsigned short *img, int64_t *timestamp, int calib = 0)
	{
		if (pos < 0 || pos >= (int)f->count)
			return -1;

		if (f->type == BIN_FILE_OTHER)
		{
			if (timestamp)
				*timestamp = f->times[pos];
			f->other->readImage(pos, calib, img);
		}
		else if (f->type == BIN_FILE_H264)
		{
			if (timestamp)
				*timestamp = f->times[pos];
			f->h264.readImage(pos, 0, img);
			return 0;
		}
		else if (f->type == BIN_FILE_HCC)
		{
			if (timestamp)
				*timestamp = f->times[pos];
			f->hcc.readImage(pos, 0, img);
			return 0;
		}
		else
		{
			if (timestamp)
				*timestamp = f->times[pos];

			seekFile(f->file, f->start + f->transferSize * pos, AVSEEK_SET);
			if (readFile(f->file, (char *)img, 2 * f->width * f->height) != (int)(2 * f->width * f->height))
				return -1;

			// f->file.seekg(f->start + f->transferSize * pos);
			// if (!f->file.read((char*)img, 2 * f->width*f->height))
			//	return -1;

			return 0;
		}
		return -1;
	}

	static int64_t *bin_get_timestamps(BinFile *f)
	{
		return f->times.data();
	}

	class IRFileLoader::PrivateData
	{
	public:
		Size size;
		TimestampVector timestamps;
		BinFile *file;
		std::string filename;
		int type;
		int min_T;
		int min_T_height;
		bool store_it;
		bool motionCorrectionEnabled;
		std::vector<unsigned short> img;
		bool has_times;
		bool saturate;
		bool bp_enabled;
		int median_value;
		int initOpticalTemperature;
		int initSTEFITemperature;
		Polygon bad_pixels;
		BaseCalibration *calib;

		PrivateData()
			: file(NULL), type(0), min_T(0), min_T_height(0), store_it(false), motionCorrectionEnabled(false), saturate(false), bp_enabled(false), median_value(-1), calib(NULL)
		{
		}
	};
	IRFileLoader::IRFileLoader()
	{
		m_data = new PrivateData();
	}

	IRFileLoader::~IRFileLoader()
	{
		close();

		delete m_data;
	}

	bool IRFileLoader::isPCR() const { return m_data->type == BIN_FILE_PCR; }
	bool IRFileLoader::isBIN() const { return m_data->type == BIN_FILE_BIN; }
	bool IRFileLoader::isPCREncapsulated() const { return m_data->type == BIN_FILE_PCR_ENCAPSULATED; }
	bool IRFileLoader::isZCompressed() const { return m_data->type == BIN_FILE_Z_COMPRESSED; }
	bool IRFileLoader::isH264() const { return m_data->type == BIN_FILE_H264; }
	bool IRFileLoader::isHCC() const { return m_data->type == BIN_FILE_HCC; }
	bool IRFileLoader::hasTimes() const { return m_data->has_times; }
	bool IRFileLoader::hasCalibration() const { return m_data->calib && m_data->calib->isValid(); }
	BaseCalibration *IRFileLoader::calibration() const { return m_data->calib; }

	void IRFileLoader::setBadPixelsEnabled(bool enable)
	{
		if (m_data->bp_enabled != enable)
		{
			// compute only once
			if (m_data->bad_pixels.empty())
			{
				std::vector<unsigned short> img(imageSize().width * imageSize().height);
				readImage(0, 0, img.data());
				m_data->bad_pixels = badPixels(img.data(), imageSize().width, imageSize().height - 3);

				/*if (m_data->median_value < 0) {
					int size = imageSize().width*(imageSize().height - 3);
					std::sort(img.data(), img.data() + size);
					m_data->median_value = img[size / 2];

					//compute std
					double sum = 0;
					int c = 0;
					for (int i = 0; i < size; ++i, ++c)
						sum += (img[i] - m_data->median_value)*(img[i] - m_data->median_value);
					sum /= c;
					sum = sqrt(sum);
					m_data->median_value -= (int)(sum * 3);
				}*/
			}
			m_data->bp_enabled = enable;

			m_data->file->hcc.setBadPixelsEnabled(enable);
		}
	}
	bool IRFileLoader::badPixelsEnabled() const
	{
		return m_data->bp_enabled;
	}

	void IRFileLoader::removeBadPixels(unsigned short *img, int w, int h)
	{
		if (!m_data->bp_enabled)
			return;

		unsigned short pixels[9];
		// unsigned short buff[10];

		for (size_t i = 0; i < m_data->bad_pixels.size(); ++i)
		{
			int x = m_data->bad_pixels[i].x();
			int y = m_data->bad_pixels[i].y();

			unsigned short *pix = pixels;
			for (int dx = x - 1; dx <= x + 1; ++dx)
				for (int dy = y - 1; dy <= y + 1; ++dy)
					if (dx >= 0 && dy >= 0 && dx < w && dy < h)
					{
						*pix++ = img[dx + dy * w];
					}
			int c = (int)(pix - pixels);
			std::nth_element(pixels, pixels + c / 2, pixels + c);
			img[x + y * w] = pixels[c / 2];
		}

		// remove low values
		// if (m_data->median_value > 0) {
		//	clampMin(img, w*h, m_data->median_value);
		// }
	}

	void IRFileLoader::removeMotion(unsigned short *img, int w, int h, int pos)
	{
		// TODO
	}

	bool IRFileLoader::loadTranslationFile(const char *filename)
	{
		// Load translation file (csv file with tabulation separator)
		return false;
	}
	void IRFileLoader::enableMotionCorrection(bool enable)
	{
		m_data->motionCorrectionEnabled = enable;
	}
	bool IRFileLoader::motionCorrectionEnabled() const
	{
		return m_data->motionCorrectionEnabled;
	}

	bool IRFileLoader::open(const char *filename)
	{
		m_data->has_times = false;
		close();
		m_data->file = bin_open_file_read(filename);
		if (!m_data->file)
			return false;

		m_data->has_times = bin_has_times(m_data->file) != 0 ? true : false;
		m_data->type = bin_file_type(m_data->file);
		int size = bin_image_count(m_data->file);
		m_data->timestamps.resize(size);
		int64_t *times = bin_get_timestamps(m_data->file);
		std::copy(times, times + size, m_data->timestamps.begin());

		int w, h;
		bin_image_size(m_data->file, &w, &h);
		m_data->size.width = w;
		m_data->size.height = h;

		m_data->filename = filename;
		replace(m_data->filename, "\\", "/");

		if ((m_data->calib = buildCalibration(m_data->filename.c_str(), this)))
		{
			this->setOpticaltemperature(m_data->calib->opticalTemperature());
			this->setSTEFItemperature(m_data->calib->STEFITemperature());
		}
		else if (m_data->file->other)
		{
			if ((m_data->calib = m_data->file->other->calibration()))
			{
				this->setOpticaltemperature(m_data->calib->opticalTemperature());
				this->setSTEFItemperature(m_data->calib->STEFITemperature());
			}
		}

		m_data->min_T = m_data->min_T_height = 0;
		std::map<std::string, std::string>::const_iterator min_T = this->globalAttributes().find("MIN_T");
		std::map<std::string, std::string>::const_iterator min_T_height = this->globalAttributes().find("MIN_T_HEIGHT");
		std::map<std::string, std::string>::const_iterator use_it = this->globalAttributes().find("STORE_IT");
		if (min_T != this->globalAttributes().end())
			m_data->min_T = fromString<int>(min_T->second);
		if (min_T_height != this->globalAttributes().end())
			m_data->min_T_height = fromString<int>(min_T_height->second);
		if (use_it != this->globalAttributes().end())
			m_data->store_it = 1;
		else
			m_data->store_it = 0;

		if (m_data->min_T_height == 0)
		{
			m_data->min_T_height = imageSize().height - 3;
		}

		return true;
	}

	bool IRFileLoader::openFileReader(void *file_reader)
	{
		m_data->has_times = false;
		close();
		m_data->file = bin_open_file_from_file_reader(NULL, file_reader);
		if (!m_data->file)
			return false;

		m_data->has_times = bin_has_times(m_data->file) != 0 ? true : false;
		m_data->type = bin_file_type(m_data->file);
		int size = bin_image_count(m_data->file);
		m_data->timestamps.resize(size);
		int64_t *times = bin_get_timestamps(m_data->file);
		std::copy(times, times + size, m_data->timestamps.begin());

		int w, h;
		bin_image_size(m_data->file, &w, &h);
		m_data->size.width = w;
		m_data->size.height = h;

		if ((m_data->calib = buildCalibration(NULL, this)))
		{
			this->setOpticaltemperature(m_data->calib->opticalTemperature());
			this->setSTEFItemperature(m_data->calib->STEFITemperature());
		}
		else if (m_data->file->other)
		{
			if ((m_data->calib = m_data->file->other->calibration()))
			{
				this->setOpticaltemperature(m_data->calib->opticalTemperature());
				this->setSTEFItemperature(m_data->calib->STEFITemperature());
			}
		}

		m_data->min_T = m_data->min_T_height = 0;
		std::map<std::string, std::string>::const_iterator min_T = this->globalAttributes().find("MIN_T");
		std::map<std::string, std::string>::const_iterator min_T_height = this->globalAttributes().find("MIN_T_HEIGHT");
		std::map<std::string, std::string>::const_iterator use_it = this->globalAttributes().find("STORE_IT");
		if (min_T != this->globalAttributes().end())
			m_data->min_T = fromString<int>(min_T->second);
		if (min_T_height != this->globalAttributes().end())
			m_data->min_T_height = fromString<int>(min_T_height->second);
		if (use_it != this->globalAttributes().end())
			m_data->store_it = 1;
		else
			m_data->store_it = 0;

		if (m_data->min_T_height == 0)
		{
			m_data->min_T_height = imageSize().height - 3;
		}

		return true;
	}

	void IRFileLoader::close()
	{
		if (m_data->calib)
		{
			if (m_data->file->other && m_data->file->other->calibration() != m_data->calib)
				delete m_data->calib;
			m_data->calib = NULL;
		}
		if (m_data->file)
		{
			bin_close_file(m_data->file);
			m_data->file = NULL;
			m_data->type = 0;
		}

		m_data->filename.clear();
	}

	StringList IRFileLoader::supportedCalibration() const
	{
		StringList res;
		res.push_back("Digital Level");
		if (m_data->calib)
		{
			res.push_back("Apparent T(C)");
		}
		return res;
	}

	bool IRFileLoader::saturate() const
	{
		return m_data->saturate;
	}

	bool IRFileLoader::getRawValue(int x, int y, unsigned short *value) const
	{
		int index = x + y * imageSize().width;
		if (index < 0 || index >= (int)m_data->img.size())
			return false;
		*value = m_data->img[index];

		if (/*m_data->min_T &&*/ m_data->min_T_height)
		{
			// the pixel is in temperature, we must switch back to DL
			if (!m_data->calib)
				return false;
			if (y >= imageSize().height - 3)
				*value = m_data->img[index]; // last 3 lines: lines contain extra infos, no need to uncalibrate
			else if (m_data->store_it)
				*value = m_data->calib->applyInvert(m_data->img.data(), m_data->file->h264.lastIt().data(), index);
		}
		return true;
	}

	const std::map<std::string, std::string> &IRFileLoader::globalAttributes() const
	{
		static std::map<std::string, std::string> empty_map;
		const std::map<std::string, std::string> &res = (m_data->type == BIN_FILE_H264) ? ((BinFile *)m_data->file)->h264.globalAttributes() : (m_data->type == BIN_FILE_HCC ? ((BinFile *)m_data->file)->hcc.globalAttributes() : (m_data->type == BIN_FILE_OTHER ? m_data->file->other->globalAttributes() : empty_map));

		return res;
	}

	bool IRFileLoader::extractAttributes(std::map<std::string, std::string> &attrs) const
	{
		bool res = true;
		if (m_data->type == BIN_FILE_H264)
			res = ((BinFile *)m_data->file)->h264.extractAttributes(attrs);
		else if (m_data->type == BIN_FILE_HCC)
			res = ((BinFile *)m_data->file)->hcc.extractAttributes(attrs);
		else if (m_data->type == BIN_FILE_OTHER)
			res = m_data->file->other->extractAttributes(attrs);

		for (std::map<std::string, std::string>::const_iterator it = attributes.begin(); it != attributes.end(); ++it)
			attrs.insert(*it);

		return res;
	}

	std::string IRFileLoader::filename() const { return m_data->filename; }
	int IRFileLoader::size() const { return (int)m_data->timestamps.size(); }
	const TimestampVector &IRFileLoader::timestamps() const { return m_data->timestamps; }
	Size IRFileLoader::imageSize() const { return m_data->size; }

	bool IRFileLoader::calibrate(unsigned short *img, float *out, int size, int calibration)
	{
		if (calibration == 0)
			return true;
		else if (calibration == 1)
		{
			// apply the calibration
			if (m_data->calib->opticalTemperature() != this->opticalTemperature())
				m_data->calib->setOpticalTemperature(this->opticalTemperature());
			if (m_data->calib->STEFITemperature() != this->STEFITemperature())
				m_data->calib->setSTEFITemperature(this->STEFITemperature());
			if (!m_data->calib->applyF(img, this->invEmissivities(), size, out, &m_data->saturate))
				return false;
			return true;
		}
		return false;
	}
	bool IRFileLoader::calibrateInplace(unsigned short *img, int size, int calibration)
	{
		if (calibration == 0)
			return true;
		else if (calibration == 1)
		{
			// apply the calibration
			if (m_data->calib->opticalTemperature() != this->opticalTemperature())
				m_data->calib->setOpticalTemperature(this->opticalTemperature());
			if (m_data->calib->STEFITemperature() != this->STEFITemperature())
				m_data->calib->setSTEFITemperature(this->STEFITemperature());
			if (!m_data->calib->apply(img, this->invEmissivities(), size, img, &m_data->saturate))
				return false;
			return true;
		}
		return false;
	}

	bool IRFileLoader::readImage(int pos, int calibration, unsigned short *pixels)
	{
		attributes.clear();

		if (pos < 0 || pos >= size() || !m_data->file)
			return false;

		int64_t time;

		// special case: other type with its own calibration
		if (m_data->type == BIN_FILE_OTHER && m_data->calib && m_data->calib == m_data->file->other->calibration())
		{
			if (bin_read_image(m_data->file, pos, pixels, &time, calibration) != 0)
				return false;

			removeBadPixels(pixels, imageSize().width, imageSize().height);
			removeMotion(pixels, imageSize().width, imageSize().height, pos);
			return true;
		}

		if (bin_read_image(m_data->file, pos, pixels, &time) != 0)
			return false;

		if (m_data->initOpticalTemperature == -1 && m_data->calib)
			m_data->initOpticalTemperature = m_data->calib->opticalTemperature();
		if (m_data->initSTEFITemperature == -1 && m_data->calib)
			m_data->initSTEFITemperature = m_data->calib->STEFITemperature();

		bool is_in_T = m_data->store_it;

		// If the image is already in temperature with subtracted min, add the min temperature stored as attribute
		if (m_data->min_T && m_data->min_T_height)
		{
			int size = m_data->size.width * m_data->min_T_height;
			for (int i = 0; i < size; ++i)
				pixels[i] += m_data->min_T;
		}

		// no need to remove bad pixels of image already in temperature, it has already been done during compression
		if (!is_in_T)
			removeBadPixels(pixels, imageSize().width, imageSize().height - 3);

		if ((int)m_data->img.size() != m_data->size.height * m_data->size.width)
			m_data->img.resize(m_data->size.height * m_data->size.width);
		memcpy(m_data->img.data(), pixels, m_data->img.size() * 2);

		m_data->saturate = false;

		if (calibration == 0)
		{
			if (is_in_T)
			{
				// image is already in temperature we must invert the calibration
				if (!m_data->calib)
					return true; // TEST: if no calibration found, just return the raw image as read in file
				// invert calibration, but not on the last 3 lines
				m_data->calib->applyInvert(pixels, m_data->file->h264.lastIt().data(), m_data->min_T_height * imageSize().width, pixels);
			}

			// Remove motion if possible
			removeMotion(pixels, imageSize().width, imageSize().height - 3, pos);

			return true;
		}
		else if (!m_data->calib)
			return false;
		else if (calibration != 1)
			return false;
		else
		{
			if (is_in_T)
			{
				if (m_data->initOpticalTemperature != this->opticalTemperature() || m_data->initSTEFITemperature != this->STEFITemperature() || this->globalEmissivity() != 1.f)
				{
					// switch back to DL without the last 3 lines
					m_data->calib->applyInvert(pixels, m_data->file->h264.lastIt().data(), m_data->min_T_height * imageSize().width, pixels);
					m_data->calib->setOpticalTemperature(this->opticalTemperature());
					m_data->calib->setSTEFITemperature(this->STEFITemperature());
					// back to T for the full image
					if (!m_data->calib->apply(pixels, this->invEmissivities(), m_data->size.height * m_data->size.width, pixels, &m_data->saturate))
						return false;
				}
			}
			else
			{
				// apply the calibration
				if (m_data->calib->opticalTemperature() != this->opticalTemperature())
					m_data->calib->setOpticalTemperature(this->opticalTemperature());
				if (m_data->calib->STEFITemperature() != this->STEFITemperature())
					m_data->calib->setSTEFITemperature(this->STEFITemperature());
				if (!m_data->calib->apply(pixels, this->invEmissivities(), m_data->size.height * m_data->size.width, pixels, &m_data->saturate))
					return false;
			}

			// Remove motion if possible
			removeMotion(pixels, imageSize().width, imageSize().height - 3, pos);

			return true;
		}
	}

}
