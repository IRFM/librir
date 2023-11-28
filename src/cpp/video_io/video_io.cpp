#include <fstream>
extern "C"
{
#include "video_io.h"
#include "tools.h"
}
#include "Log.h"
#include "IRFileLoader.h"
#include "h264.h"
#include "ZFile.h"

using namespace rir;

#include <fstream>
int open_camera_file(const char *filename, int *file_format)
{
	if (file_format)
		*file_format = 0;

	IRFileLoader *loader = new IRFileLoader();
	if (loader->open(filename))
	{
		if (file_format)
		{
			if (loader->isPCR())
				*file_format = FILE_FORMAT_PCR;
			else if (loader->isBIN())
				*file_format = FILE_FORMAT_WEST;
			else if (loader->isPCREncapsulated())
				*file_format = FILE_FORMAT_PCR_ENCAPSULATED;
			else if (loader->isZCompressed())
				*file_format = FILE_FORMAT_ZSTD_COMPRESSED;
			else if (loader->isHCC())
				*file_format = FILE_FORMAT_HCC;
			else if (loader->isH264())
				*file_format = FILE_FORMAT_H264;
			else
				*file_format = FILE_FORMAT_OTHER;
		}
		return set_void_ptr(loader);
	}
	else
	{
		delete loader;
		logError(("Unable to open camera file " + std::string(filename) + ": wrong file format").c_str());
		return 0;
	}
}

int video_file_format(const char *filename)
{
	IRFileLoader loader;
	if (loader.open(filename))
	{
		if (loader.isPCR())
			return FILE_FORMAT_PCR;
		else if (loader.isBIN())
			return FILE_FORMAT_WEST;
		else if (loader.isPCREncapsulated())
			return FILE_FORMAT_PCR_ENCAPSULATED;
		else if (loader.isZCompressed())
			return FILE_FORMAT_ZSTD_COMPRESSED;
		else if (loader.isHCC())
			return FILE_FORMAT_HCC;
		else if (loader.isH264())
			return FILE_FORMAT_H264;
		else
			return FILE_FORMAT_OTHER;
	}
	return -1;
}

int open_camera_file_reader(void *file_reader, int *file_format)
{
	if (file_format)
		*file_format = 0;

	IRFileLoader *loader = new IRFileLoader();
	if (loader->openFileReader(file_reader))
	{
		if (file_format)
		{
			if (loader->isPCR())
				*file_format = FILE_FORMAT_PCR;
			else if (loader->isBIN())
				*file_format = FILE_FORMAT_WEST;
			else if (loader->isPCREncapsulated())
				*file_format = FILE_FORMAT_PCR_ENCAPSULATED;
			else if (loader->isZCompressed())
				*file_format = FILE_FORMAT_ZSTD_COMPRESSED;
			else if (loader->isHCC())
				*file_format = FILE_FORMAT_HCC;
			else if (loader->isH264())
				*file_format = FILE_FORMAT_H264;
			else
				*file_format = FILE_FORMAT_OTHER;
		}
		return set_void_ptr(loader);
	}
	else
	{
		delete loader;
		logError(("Unable to open camera file: wrong file format"));
		return 0;
	}
}

int close_camera(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_image_count: NULL camera");
		return -1;
	}
	rm_void_ptr(cam);
	delete l;
	return 0;
}

int get_image_count(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_image_count: NULL camera");
		return -1;
	}
	return l->size();
}

int get_image_time(int cam, int pos, int64_t *time)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_image_time: NULL camera");
		return -1;
	}
	const std::vector<std::int64_t> &times = l->timestamps();
	if (pos < 0 || pos >= (int)times.size())
	{
		logError("get_image_time: position out of range");
		return -1;
	}
	*time = times[pos];
	return 0;
}

int get_image_size(int cam, int *width, int *height)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_image_size: NULL camera");
		return -1;
	}
	Size s = l->imageSize();
	*width = s.width;
	*height = s.height;
	return 0;
}

int get_filename(int cam, char *filename)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_filename: NULL camera");
		return -1;
	}
	std::string fname = l->filename();
	if (fname.size() > UNSPECIFIED_CHAR_LENGTH - 1)
		fname = fname.substr(0, UNSPECIFIED_CHAR_LENGTH - 1);
	// strcpy(filename,fname.c_str());
	memset(filename, 0, UNSPECIFIED_CHAR_LENGTH);
	memcpy(filename, fname.c_str(), fname.size());
	return 0;
}

int supported_calibrations(int cam, int *count)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("support_calibration: NULL camera");
		return -1;
	}
	*count = (int)l->supportedCalibration().size();
	return 0;
}

int calibration_name(int cam, int calibration, char *name)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("support_calibration: NULL camera");
		return -1;
	}

	const StringList calibs = l->supportedCalibration();
	if (calibration < 0 || calibration >= (int)calibs.size())
	{
		logError("calibration_name: calibration index out of range");
		return -1;
	}

	memcpy(name, calibs[calibration].c_str(), calibs[calibration].size());
	return 0;
}

int flip_camera_calibration(int camera, int flip_rl, int flip_ud)
{
	void *c = get_void_ptr(camera);
	if (!c)
		return -1;
	IRVideoLoader *cam = (IRVideoLoader *)c;
	BaseCalibration *full = cam->calibration();
	if (!full)
		return -1;
	Size s = cam->imageSize();
	if (s.width == 640)
	{
		full->flipTransmissions(flip_rl != 0, flip_ud != 0, 640, 512); // only work for SCD camera!
		return 0;
	}
	else if (s.width == 320)
	{
		full->flipTransmissions(flip_rl != 0, flip_ud != 0, 320, 240); // only work for CEDIP camera!
		return 0;
	}
	return -1;
}

int set_global_emissivity(int cam, float emi)
{
	if (emi < 0.f || emi > 1.f)
	{
		logError("set_emissivity: wrong emissivity value");
		return -1;
	}

	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("set_global_emissivity: NULL camera");
		return -1;
	}
	l->setEmissivity(emi);
	return 0;
}

int set_emissivity(int cam, float *emi, int size)
{

	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("set_emissivity: NULL camera");
		return -1;
	}
	if (!l->setEmissivities(emi, size))
	{
		logError("set_emissivity: wrong vector size");
		return -1;
	}
	return 0;
}

int get_emissivity(int cam, float *emi, int size)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_emissivity: NULL camera");
		return -1;
	}

	int s = size;
	if ((int)l->invEmissivities().size() < s)
		s = (int)l->invEmissivities().size();
	for (int i = 0; i < s; ++i)
		emi[i] = 1.f / l->invEmissivities()[i];
	if (s == 0)
		*emi = 1;

	return s;
}

int support_emissivity(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l || !l->calibration())
	{
		logError("support_emissivity: NULL camera");
		return -1;
	}
	return (l->calibration()->supportedFeatures() & BaseCalibration::SupportGlobalEmissivity) ? 1 : 0;

	/*if ((strcmp(l->typeName(), "IRLoader") == 0))
		return 1;
	else if ((strcmp(l->typeName(), "IRFileLoader") == 0)) {
		IRFileLoader* bin = static_cast<IRFileLoader*>(camera);
		return (int)(bin->supportedCalibration().size() == 2);
	}
	else
		return 0;*/
}

int set_optical_temperature(int cam, unsigned short temp_C)
{
	int ok = support_optical_temperature(cam);
	if (!ok)
		return -1;

	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("set_optical_temperature: NULL camera");
		return -1;
	}

	l->setOpticaltemperature(temp_C);
	return 0;
}
unsigned short get_optical_temperature(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_optical_temperature: NULL camera");
		return -1;
	}
	return l->opticalTemperature();
}

int set_STEFI_temperature(int cam, unsigned short temp_C)
{
	int ok = support_optical_temperature(cam);
	if (!ok)
		return -1;

	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("set_STEFI_temperature: NULL camera");
		return -1;
	}

	l->setSTEFItemperature(temp_C);
	return 0;
}
unsigned short get_STEFI_temperature(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_STEFI_temperature: NULL camera");
		return -1;
	}
	return l->STEFITemperature();
}

int support_optical_temperature(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l || !l->calibration())
	{
		return 0;
	}
	return (l->calibration()->supportedFeatures() & BaseCalibration::SupportOpticalTemperature) ? 1 : 0;
	/*if (strcmp(l->typeName(), "IRLoader") != 0)
	{
		return 1;
	}
	IRLoader* ir = static_cast<IRLoader*>(l);
	std::string id = ir->identifier();
	if (id.find("CEA") != std::string::npos)
		return 0;
	return 1;*/
}

int load_image(int cam, int pos, int calibration, unsigned short *data)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("load_image: NULL camera");
		return -1;
	}

	if (l->readImage(pos, calibration, data))
		return 0;
	else
		return -1;
}

int calibrate_inplace(int cam, unsigned short *img, int size, int calibration)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("load_image: NULL camera");
		return -1;
	}

	if (l->calibrateInplace(img, size, calibration))
		return 0;
	return -1;
}

int camera_saturate(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("camera_saturate: NULL camera");
		return -1;
	}
	return (int)l->saturate();
}

int calibration_files(int cam, char *dst, int *dstSize)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l || !l->calibration())
		return -1;
	StringList lst = l->calibration()->calibrationFiles();
	std::string res = join(lst, "\n");
	if (*dstSize <= (int)res.size())
	{
		*dstSize = (int)res.size() + 1;
		return -2;
	}

	memset(dst, 0, *dstSize);
	memcpy(dst, res.c_str(), res.size());
	*dstSize = (int)res.size() + 1;
	return 0;
}

int enable_bad_pixels(int cam, int enable)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("enable_bad_pixels: NULL camera");
		return -1;
	}
	l->setBadPixelsEnabled(enable != 0);
	return 0;
}

int bad_pixels_enabled(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		return 0;
	}
	return l->badPixelsEnabled();
}

int calibrate_image(int cam, unsigned short *img, float *out, int size, int calib)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("calibrate_image: NULL camera");
		return -1;
	}

	bool r = l->calibrate(img, out, size, calib);
	if (!r)
		return -1;
	return 0;
}
int calibrate_image_inplace(int cam, unsigned short *img, int size, int calib)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("calibrate_image_inplace: NULL camera");
		return -1;
	}

	bool r = l->calibrateInplace(img, size, calib);
	if (!r)
		return -1;
	return 0;
}

int load_motion_correction_file(int cam, const char *filename)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("load_motion_correction_file: NULL camera");
		return -1;
	}

	if (!l->loadTranslationFile(filename))
	{
		logError("unable to load file");
		return -1;
	}
	return 0;
}

int enable_motion_correction(int cam, int enable)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("enable_motion_correction: NULL camera");
		return -1;
	}
	l->enableMotionCorrection((bool)enable);
	return 0;
}

int motion_correction_enabled(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("motion_correction_enabled: NULL camera");
		return 0;
	}
	return l->motionCorrectionEnabled();
}

int get_attribute_count(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_attribute_count: NULL camera");
		return -1;
	}

	std::map<std::string, std::string> attributes;
	bool r = l->extractAttributes(attributes);
	return !r ? 0 : (int)attributes.size();
}

int get_attribute(int cam, int index, char *key, int *key_len, char *value, int *value_len)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_attribute: NULL camera");
		return -1;
	}

	std::map<std::string, std::string> attributes;
	bool r = l->extractAttributes(attributes);
	if (!r)
		return -1;
	if (index >= (int)attributes.size())
		return -1;

	std::map<std::string, std::string>::iterator it = attributes.begin();
	std::advance(it, index);

	int s1 = (int)it->first.size();
	int s2 = (int)it->second.size();
	int oldklen = *key_len;
	int oldvlen = *value_len;
	if (s1 > *key_len || s2 > *value_len)
	{
		*key_len = (int)s1;
		*value_len = (int)s2;
		return -2;
	}
	*key_len = (int)s1;
	*value_len = (int)s2;
	memcpy(key, it->first.c_str(), s1);
	memcpy(value, it->second.c_str(), s2);
	if (oldklen > s1)
		key[s1] = 0;
	if (oldvlen > s2)
		value[s2] = 0;
	return 0;
}

int get_global_attribute_count(int cam)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_global_attribute_count: NULL camera");
		return -1;
	}

	return (int)l->globalAttributes().size();
}

int get_global_attribute(int cam, int index, char *key, int *key_len, char *value, int *value_len)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_global_attribute: NULL camera");
		return -1;
	}

	const std::map<std::string, std::string> &attributes = l->globalAttributes();
	if (index >= (int)attributes.size())
		return -1;

	std::map<std::string, std::string>::const_iterator it = attributes.begin();
	std::advance(it, index);

	int s1 = (int)it->first.size();
	int s2 = (int)it->second.size();
	//	int oldklen = *key_len;
	int oldvlen = *value_len;
	if (s1 + 1 > *key_len || s2 > *value_len)
	{
		*key_len = (int)s1 + 1;
		*value_len = (int)s2;
		return -2;
	}
	*key_len = (int)s1;
	*value_len = (int)s2;
	memcpy(key, it->first.c_str(), s1);
	memcpy(value, it->second.c_str(), s2);
	key[s1] = 0;
	if (oldvlen > s2)
		value[s2] = 0;
	return 0;
}

#include "h264.h"

struct H264
{
	H264_Saver saver;
	int width, height, lossy_height;
	std::string filename;
	std::map<std::string, std::string> attributes; // global attributes
};

int h264_open_file(const char *filename, int width, int height, int lossy_height)
{
	H264 *saver = new H264();
	saver->width = width;
	saver->height = height;
	saver->lossy_height = lossy_height;
	saver->filename = filename;
	if (file_exists(filename))
	{
		if (remove(filename) != 0)
		{
			logError("h264_open_file: cannot remove output file");
			delete saver;
			return 0;
		}
	}
	return set_void_ptr(saver);
}

void h264_close_file(int file)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_close_file: NULL identifier");
		return;
	}
	saver->saver.close();
	rm_void_ptr(file);
	delete saver;
}
int h264_set_parameter(int file, const char *param, const char *value)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_set_parameter: NULL identifier");
		return -1;
	}
	if (saver->saver.setParameter(param, value))
		return 0;
	return -1;
}
int h264_set_global_attributes(int file, int attribute_count, char *keys, int *key_lens, char *values, int *value_lens)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_set_global_attributes: NULL identifier");
		return -1;
	}

	saver->attributes.clear();
	for (int i = 0; i < attribute_count; ++i)
	{
		std::string k(keys, keys + *key_lens);
		std::string v(values, values + *value_lens);
		saver->attributes[k] = v;
		keys += *key_lens;
		values += *value_lens;
		++key_lens;
		++value_lens;
	}
	if (saver->saver.isOpen())
		saver->saver.setGlobalAttributes(saver->attributes);
	return 0;
}
int h264_add_image_lossless(int file, unsigned short *img, int64_t timestamps_ns, int attribute_count, char *keys, int *key_lens, char *values, int *value_lens)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_set_global_attributes: NULL identifier");
		return -1;
	}
	std::map<std::string, std::string> attributes;
	for (int i = 0; i < attribute_count; ++i)
	{
		std::string k(keys, keys + *key_lens);
		std::string v(values, values + *value_lens);
		attributes[k] = v;
		keys += *key_lens;
		values += *value_lens;
		++key_lens;
		++value_lens;
	}

	if (!saver->saver.isOpen())
	{
		// open and write global attributes
		if (!saver->saver.open(saver->filename.c_str(), saver->width, saver->height, saver->lossy_height, 50))
			return -1;
		saver->saver.setGlobalAttributes(saver->attributes);
	}
	if (saver->saver.addImageLossLess(img, timestamps_ns, attributes))
		return 0;
	return -1;
}
int h264_add_image_lossy(int file, unsigned short *img_DL, int64_t timestamps_ns, int attribute_count, char *keys, int *key_lens, char *values, int *value_lens)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_set_global_attributes: NULL identifier");
		return -1;
	}
	std::map<std::string, std::string> attributes;
	for (int i = 0; i < attribute_count; ++i)
	{
		std::string k(keys, keys + *key_lens);
		std::string v(values, values + *value_lens);
		attributes[k] = v;
		keys += *key_lens;
		values += *value_lens;
		++key_lens;
		++value_lens;
	}
	if (!saver->saver.isOpen())
	{
		// open and write global attributes
		if (!saver->saver.open(saver->filename.c_str(), saver->width, saver->height, saver->lossy_height, 50))
			return -1;
		saver->saver.setGlobalAttributes(saver->attributes);
	}
	// if (saver->saver.addImage(img_T, img_DL, timestamps_ns, attributes))
	if (saver->saver.addImageLossy(img_DL, timestamps_ns, attributes))
		return 0;
	return -1;
}

int h264_add_loss(int file, unsigned short *img)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_add_loss: NULL identifier");
		return -1;
	}
	if (!saver->saver.isOpen())
	{
		// open and write global attributes
		if (!saver->saver.open(saver->filename.c_str(), saver->width, saver->height, saver->lossy_height, 50))
			return -1;
	}
	if (saver->saver.addLoss(img))
		return 0;
	return -1;
}

int h264_get_low_errors(int file, unsigned short *errors, int *size)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_get_low_erros: NULL identifier");
		return -1;
	}
	const std::vector<unsigned short> &err = saver->saver.lowErrors();
	if (*size < (int)err.size())
	{
		*size = (int)err.size();
		return -2;
	}
	*size = (int)err.size();
	memcpy(errors, err.data(), *size * sizeof(unsigned short));
	return 0;
}
int h264_get_high_errors(int file, unsigned short *errors, int *size)
{
	H264 *saver = (H264 *)get_void_ptr(file);
	if (!saver)
	{
		logError("h264_get_high_erros: NULL identifier");
		return -1;
	}
	const std::vector<unsigned short> &err = saver->saver.highErrors();
	if (*size < (int)err.size())
	{
		*size = (int)err.size();
		return -2;
	}
	*size = (int)err.size();
	memcpy(errors, err.data(), *size * sizeof(unsigned short));
	return 0;
}

int get_table_names(int cam, char *dst, int *dst_size)
{
	IRVideoLoader *loader = (IRVideoLoader *)get_void_ptr(cam);
	if (!loader)
	{
		logError("get_table_names: NULL identifier");
		return -1;
	}

	StringList lst = loader->tableNames();
	std::string out;
	for (const auto &s : lst)
	{
		out += s;
		out += '\n';
	}
	if ((int)out.size() > *dst_size)
	{
		*dst_size = (int)out.size();
		return -2;
	}

	memcpy(dst, out.c_str(), out.size());
	*dst_size = (int)out.size();
	return 0;
}

int get_table(int cam, const char *name, float *dst, int *dst_size)
{
	IRVideoLoader *loader = (IRVideoLoader *)get_void_ptr(cam);
	if (!loader)
	{
		logError("get_table_names: NULL identifier");
		return -1;
	}

	std::vector<float> table = loader->getTable(name);
	if (table.size() == 0)
		return -1;

	if ((int)table.size() > *dst_size)
	{
		*dst_size = (int)table.size();
		return -2;
	}

	std::copy(table.begin(), table.end(), dst);
	*dst_size = (int)table.size();
	return 0;
}

int open_video_write(const char *filename, int width, int height, int rate, int method, int clevel)
{
	return set_void_ptr(z_open_file_write(filename, width, height, rate, method, clevel));
}
int image_write(int writter, unsigned short *img, int64_t time)
{
	void *w = get_void_ptr(writter);
	if (w)
		return z_write_image(w, img, time);
	return -1;
}
int64_t close_video(int writter)
{
	void *w = get_void_ptr(writter);
	if (w)
	{
		return z_close_file(w);
		rm_void_ptr(writter);
	}
	return 0;
}

int get_last_image_raw_value(int cam, int x, int y, unsigned short *value)
{
	void *camera = get_void_ptr(cam);
	IRVideoLoader *l = static_cast<IRVideoLoader *>(camera);
	if (!l)
	{
		logError("get_last_image_raw_value: NULL camera");
		return -1;
	}

	if (l->getRawValue(x, y, value))
		return 0;
	return -1;
}

int correct_PCR_file(const char *filename, int width, int height, int freq)
{
	PCR_HEADER h;
	IRFileLoader l;
	if (l.open(filename))
		return -1;

	std::fstream fout(filename, std::ios::in | std::ios::out | std::ios::binary);
	fout.read((char *)&h, sizeof(h));
	h.X = h.GrabSizeX = width;
	h.Y = h.GrabSizeY = height;
	h.Bits = 16;
	h.Version = 0;
	h.TransfertSize = width * height * 2;
	h.Frequency = freq;

	fout.seekp(0);
	fout.write((char *)&h, sizeof(h));
	return 0;
}
