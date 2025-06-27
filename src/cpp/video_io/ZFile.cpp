

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>

#include "ZFile.h"
#include "Log.h"
#include "FileAttributes.h"
#include "ReadFileChunk.h"
#include "Misc.h"
#include "tools.h"

using namespace rir;

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
		uint64_t date;
		uint64_t rate;
		uint64_t samples;
		uint64_t samples_pre_trigger;
		uint64_t type;
		uint64_t nb_channels;
		uint64_t data_type;
		uint64_t data_format;
		uint64_t data_repetition;
		uint64_t data_size_x;
		uint64_t data_size_y;
	};
	char reserved[128];
} BIN_TRIGGER;

typedef struct ZFile
{
	BIN_HEADER bheader;
	BIN_TRIGGER btrigger;

	int readOnly;
	FileAttributes attributes;
	FileReaderPtr file_reader;
	std::fstream file;
	std::string filename;
	unsigned char *mem;
	uint64_t tot_size; // file/mem total size in bytes
	uint64_t pos;	   // file/mem position in bytes

	// compression buffer
	std::vector<char> buffer;
	int clevel; // compression level

	// image pos and timestamps for read only seeking
	std::vector<int64_t> timestamps;
	std::vector<int64_t> positions;
} ZFile;

inline ZFile *createZFile()
{
	ZFile *f = new ZFile(); //(ZFile*)malloc(sizeof(ZFile));
	memset(&f->bheader, 0, sizeof(f->bheader));
	memset(&f->btrigger, 0, sizeof(f->btrigger));
	f->bheader.version = 1;
	f->bheader.compression = 2; // 1 for raw zstd, 2 for blosc+zstd
	f->bheader.triggers = 1;
	f->btrigger.rate = 1;
	f->readOnly = 1;
	// f->file = NULL;
	f->file.close();
	f->file_reader = NULL;
	f->mem = NULL;
	f->tot_size = 0;
	f->pos = 0;
	// f->buffer = NULL;
	// f->buffer_size = 0;
	f->clevel = 0;
	// f->timestamps = NULL;
	// f->positions = NULL;
	return f;
}

static void destroyZFile(void *_f)
{
	ZFile *f = (ZFile *)_f;
	if (f)
	{
		// if (f->file)
		//	fclose(f->file);
		// if (f->buffer)
		//	free(f->buffer);
		// if (f->timestamps)
		//	free(f->timestamps);
		// if (f->positions)
		//	free(f->positions);
		// free(f);
		delete f;
	}
}

void *z_open_file_read(const FileReaderPtr & file_reader)
{
	/*ZFile * res = createZFile();
	res->file.open(filename, std::ios::binary|std::ios::in);// = fopen(filename, "rb");
	if (!res->file) {
		//logError("error");
		destroyZFile(res);
		return NULL;
	}
	res->filename = filename;*/

	if (!file_reader)
		return NULL;
	ZFile *res = createZFile();
	res->file_reader = file_reader;

	// get file size
	/*res->file.seekg(0, std::ios::end);
	res->tot_size = res->file.tellg();
	res->file.seekg(0);*/
	res->tot_size = fileSize(res->file_reader);

	// read headers
	// res->file.read((char*)&res->bheader, sizeof(res->bheader));
	// res->file.read((char*)&res->btrigger, sizeof(res->btrigger));
	seekFile(res->file_reader, 0, AVSEEK_SET);
	readFile(res->file_reader, &res->bheader, sizeof(res->bheader));
	readFile(res->file_reader, &res->btrigger, sizeof(res->btrigger));

	// make sure BIN_HEADER is valid
	if ((res->bheader.compression != 1 && res->bheader.compression != 2 && res->bheader.compression != 3) || res->bheader.version != 1 || res->bheader.triggers != 1)
	{
		destroyZFile(res);
		// printf("error");
		return NULL;
	}

	// make sure BIN_TRIGGER is valid
	if (res->btrigger.data_size_x > 0 && res->btrigger.data_size_x < 3000 && res->btrigger.data_size_y > 0 && res->btrigger.data_size_y < 3000 && res->btrigger.rate > 0 && res->btrigger.rate < 1000)
	{
		res->readOnly = 1;

		// allocate compression buffer
		res->buffer.resize((unsigned)zstd_compress_bound(res->btrigger.data_size_x * res->btrigger.data_size_y * 2));

		// grab all images timestamps and positions in file
		uint64_t pos = posFile(res->file_reader);		   // res->file.tellg();
		res->timestamps.resize(res->btrigger.samples); // = (int64_t*)malloc(sizeof(uint64_t) *res->btrigger.samples);
		res->positions.resize(res->btrigger.samples);  // = (int64_t*)malloc(sizeof(uint64_t) *res->btrigger.samples);

		bool has_partial_attributes = false;
		bool has_attributes = false;
		if (res->attributes.openReadOnly(res->file_reader))
		{
			// the file contains attributes
			// read timestamps and positions from the attributes if possible
			has_partial_attributes = true;
			res->btrigger.samples = res->attributes.size();
			std::map<std::string, std::string>::const_iterator it = res->attributes.globalAttributes().find("positions");
			if (it != res->attributes.globalAttributes().end())
			{
				const std::string positions = it->second;
				if (positions.size() == res->btrigger.samples * 8)
				{

					res->timestamps.resize(res->btrigger.samples);
					res->positions.resize(res->btrigger.samples);
					const char *poss = positions.c_str();
					for (size_t i = 0; i < res->btrigger.samples; ++i)
					{
						res->timestamps[i] = res->attributes.timestamp(i);
						int64_t p = 0;
						memcpy(&p, poss, 8);
						poss += 8;
						if (!is_little_endian())
							p = swap_int64(p);
						res->positions[i] = p;
					}
					has_attributes = true;
				}
			}
		}

		if (!has_attributes)
		{
			seekFile(res->file_reader, pos, AVSEEK_SET);
			if (res->btrigger.samples == 0)
			{
				// scan the file
				res->timestamps.clear();
				res->positions.clear();
				size_t table_size = res->attributes.tableSize();
				// remove the attributes table size when looking for images
				size_t max_size = fileSize(res->file_reader) - (has_partial_attributes ? table_size : 0);
				while (true)
				{
					uint32_t fsize = 0;
					int64_t timestamp = 0;
					int64_t pos = posFile(res->file_reader); // res->file.tellg();
					if ((size_t)pos >= max_size)
						break;

					if (readFile(res->file_reader, &timestamp, sizeof(timestamp)) != sizeof(timestamp))
						break;
					if (readFile(res->file_reader, &fsize, sizeof(fsize)) != sizeof(fsize))
						break;
					if (seekFile(res->file_reader, fsize, AVSEEK_CUR) < 0) // go to next
						break;
					res->timestamps.push_back(timestamp);
					res->positions.push_back(pos);
					res->btrigger.samples++;
				}
			}
			else
			{
				for (size_t i = 0; i < res->btrigger.samples; ++i)
				{
					uint32_t fsize = 0;
					int64_t timestamp = 0;

					res->positions[i] = posFile(res->file_reader); // res->file.tellg();

					if (readFile(res->file_reader, &timestamp, sizeof(timestamp)) != sizeof(timestamp))
					{
						res->btrigger.samples = i + 1;
						break;
					}
					if (readFile(res->file_reader, &fsize, sizeof(fsize)) != sizeof(fsize))
					{
						res->btrigger.samples = i + 1;
						break;
					}
					res->timestamps[i] = timestamp;
					if (seekFile(res->file_reader, fsize, AVSEEK_CUR) < 0)
					{
						res->btrigger.samples = i + 1;
						break;
					}
				}
			}
		}
		seekFile(res->file_reader, pos, AVSEEK_SET);

		return res;
	}

	destroyZFile(res);
	// printf("error");
	return NULL;
}

void *z_open_memory_read(void *mem, uint64_t size)
{
	ZFile *res = createZFile();
	res->tot_size = size;
	res->mem = (unsigned char *)mem;

	// read headers
	memcpy(&res->bheader, res->mem + res->pos, sizeof(res->bheader));
	res->pos += sizeof(res->bheader);
	memcpy(&res->btrigger, res->mem + res->pos, sizeof(res->btrigger));
	res->pos += sizeof(res->btrigger);

	// make sure BIN_HEADER is valid
	if (res->bheader.compression < 1 || res->bheader.compression > 3 || res->bheader.version != 1 || res->bheader.triggers != 1)
	{
		destroyZFile(res);
		// printf("error");
		return NULL;
	}

	// make sure BIN_TRIGGER is valid
	if (res->btrigger.data_size_x > 0 && res->btrigger.data_size_x < 2000 && res->btrigger.data_size_y > 0 && res->btrigger.data_size_y < 2000 && res->btrigger.rate > 0 && res->btrigger.rate < 1000)
	{
		res->readOnly = 1;

		// allocate compression buffer
		// res->buffer_size = (unsigned)ZSTD_compressBound(res->btrigger.data_size_x*res->btrigger.data_size_y * 2);
		// res->buffer = malloc(res->buffer_size);
		res->buffer.resize(zstd_compress_bound(res->btrigger.data_size_x * res->btrigger.data_size_y * 2));

		// grab all images timestamps and positions in file
		uint64_t pos = res->pos;
		res->timestamps.resize(res->btrigger.samples); // = (int64_t*)malloc(sizeof(uint64_t) *res->btrigger.samples);
		res->positions.resize(res->btrigger.samples);  // = (int64_t*)malloc(sizeof(uint64_t) *res->btrigger.samples);
		for (size_t i = 0; i < res->btrigger.samples; ++i)
		{
			uint32_t fsize = 0;
			int64_t timestamp = 0;

			res->positions[i] = pos;
			memcpy(&timestamp, res->mem + pos, sizeof(timestamp));
			pos += sizeof(timestamp);
			memcpy(&fsize, res->mem + pos, sizeof(fsize));
			pos += sizeof(fsize);
			res->timestamps[i] = timestamp;

			if ((pos += fsize) >= res->tot_size)
			{
				res->btrigger.samples = i + 1;
				break;
			}
		}

		return res;
	}

	destroyZFile(res);
	// printf("error");
	return NULL;
}

void *z_open_file_write(const char *filename, int width, int height, int rate, int method, int clevel)
{
	ZFile *res = createZFile();
	res->file.open(filename, std::ios::binary | std::ios::out); // = fopen(filename, "wb");
	if (!res->file)
	{
		// printf("error");
		destroyZFile(res);
		return NULL;
	}
	res->filename = filename;
	// set file size
	res->tot_size = 0;
	res->readOnly = 0;
	res->clevel = clevel;

	// allocate compression buffer
	// res->buffer_size = (unsigned) ZSTD_compressBound(width*height * 2);
	// res->buffer = malloc(res->buffer_size);
	res->buffer.resize(zstd_compress_bound(width * height * 2));

	res->bheader.compression = method;

	// write headers
	res->btrigger.type = 1; // continous acquisition
	res->btrigger.nb_channels = 1;
	res->btrigger.data_format = 2; // 2D array
	res->btrigger.data_format = 3; // u16
	res->btrigger.data_repetition = 1;
	res->btrigger.data_size_x = width;
	res->btrigger.data_size_y = height;
	res->btrigger.rate = rate;
	// fwrite(&res->bheader, sizeof(res->bheader), 1, res->file);
	// fwrite(&res->btrigger, sizeof(res->btrigger), 1, res->file);
	// res->pos = ftell(res->file);
	res->file.write((char *)(&res->bheader), sizeof(res->bheader));
	res->file.write((char *)(&res->btrigger), sizeof(res->btrigger));
	res->pos = res->file.tellp();
	res->attributes.open(filename);
	return res;
}

void *z_open_memory_write(void *mem, uint64_t size, int width, int height, int rate, int method, int clevel)
{
	ZFile *res = createZFile();
	res->tot_size = size;
	res->mem = (unsigned char *)mem;

	// cannot store at least one image
	if (size < sizeof(res->btrigger) + sizeof(res->bheader) + width * height * 2)
	{
		destroyZFile(res);
		// printf("error");
		return NULL;
	}

	// set file size
	res->readOnly = 0;
	res->clevel = clevel;

	// allocate compression buffer
	// res->buffer_size = (unsigned)ZSTD_compressBound(width*height * 2);
	// res->buffer = malloc(res->buffer_size);
	res->buffer.resize((unsigned)zstd_compress_bound(width * height * 2));

	// write headers
	res->bheader.compression = method;

	res->btrigger.type = 1; // continous acquisition
	res->btrigger.nb_channels = 1;
	res->btrigger.data_format = 2; // 2D array
	res->btrigger.data_format = 3; // u16
	res->btrigger.data_repetition = 1;
	res->btrigger.data_size_x = width;
	res->btrigger.data_size_y = height;
	res->btrigger.rate = rate;

	memcpy(res->mem + res->pos, &res->bheader, sizeof(res->bheader));
	res->pos += sizeof(res->bheader);
	memcpy(res->mem + res->pos, &res->btrigger, sizeof(res->btrigger));
	res->pos += sizeof(res->btrigger);

	return res;
}

uint64_t z_close_file(void *file)
{
	ZFile *f = (ZFile *)file;
	uint64_t res = 0;
	if (f && !f->readOnly)
	{
		// write again the trigger header to update the sample count
		if (f->file)
		{
			f->file.seekp(sizeof(BIN_HEADER));
			f->file.write((char *)&f->btrigger, sizeof(f->btrigger));
			// fseek(f->file, sizeof(BIN_HEADER), SEEK_SET);
			// fwrite(&f->btrigger, sizeof(f->btrigger), 1, f->file);
		}
		else if (f->mem)
		{
			memcpy(f->mem + sizeof(BIN_HEADER), &f->btrigger, sizeof(BIN_TRIGGER));
		}

		res = f->pos;
		f->file.close();

		// TODO
		// write timestamps and positions as attributes
		std::string positions(f->btrigger.samples * 8, (char)0);
		char *poss = (char *)positions.data();
		f->attributes.resize(f->btrigger.samples);
		for (size_t i = 0; i < f->btrigger.samples; ++i)
		{
			f->attributes.setTimestamp(i, f->timestamps[i]);
			int64_t p = f->positions[i];
			if (!is_little_endian())
				p = swap_int64(p);
			memcpy(poss, &p, 8);
			poss += 8;
		}
		f->attributes.addGlobalAttribute("positions", positions);
		f->attributes.close();
	}

	destroyZFile(file);
	return res;
}

int z_image_count(void *file)
{
	if (!file)
		return -1;
	ZFile *f = (ZFile *)file;
	return (int)f->btrigger.samples;
}

int z_image_size(void *file, int *width, int *height)
{
	if (!file)
		return -1;
	ZFile *f = (ZFile *)file;
	*width = (int)f->btrigger.data_size_x;
	*height = (int)f->btrigger.data_size_y;
	return 0;
}

int64_t z_file_size(void *file)
{
	if (!file)
		return -1;
	ZFile *f = (ZFile *)file;
	if (!f->readOnly)
		return f->pos;
	else
		return f->tot_size;
}

int z_write_image(void *file, const unsigned short *img, int64_t timestamp)
{
	if (!file)
		return -1;

	ZFile *f = (ZFile *)file;
	if (f->readOnly)
		return -1;

	// compress image
	uint32_t csize = 0;
	if (f->bheader.compression == 1)
	{
		csize = (uint32_t)zstd_compress((char *)img, f->btrigger.data_size_x * f->btrigger.data_size_y * 2, f->buffer.data(), f->buffer.size(), f->clevel);
		if (csize == (uint32_t)-1)
			return -1;
	}

	if (f->file)
	{
		int64_t pos = f->file.tellp();
		// write timestamp
		// fwrite(&timestamp, sizeof(timestamp), 1, f->file);
		f->file.write((char *)&timestamp, sizeof(timestamp));

		// write compressed size
		// fwrite(&csize, sizeof(csize), 1, f->file);
		f->file.write((char *)&csize, sizeof(csize));

		// write compressed image
		// if (fwrite(f->buffer, csize, 1, f->file) != 1)
		//	return -1;
		if (!f->file.write(f->buffer.data(), csize))
			return -1;

		f->pos = f->file.tellp(); // ftell(f->file);
		f->btrigger.samples++;

		f->timestamps.push_back(timestamp);
		f->positions.push_back(pos);

		return 0;
	}
	else if (f->mem)
	{
		if (f->pos + 8 + 4 + csize >= f->tot_size)
			return -1;

		memcpy(f->mem + f->pos, &timestamp, sizeof(timestamp));
		f->pos += sizeof(timestamp);
		memcpy(f->mem + f->pos, &csize, sizeof(csize));
		f->pos += sizeof(csize);
		memcpy(f->mem + f->pos, f->buffer.data(), csize);
		f->pos += csize;
		f->btrigger.samples++;
		return 0;
	}

	return -1;
}

int z_read_image(void *file, int pos, unsigned short *img, int64_t *timestamp)
{
	if (!file)
		return -1;

	ZFile *f = (ZFile *)file;
	if (!f->readOnly)
		return -1;

	if (pos < 0 || pos >= (int)f->btrigger.samples)
		return -1;

	/*if (!f->mem && !f->file && !f->filename.empty()) {
		f->file.close();
		f->file.open(f->filename.c_str(), std::ios::in | std::ios::binary);
	}*/

	uint32_t fsize;
	int64_t time;
	if (f->file_reader) // f->file)
	{
		/*f->file.seekg(f->positions[pos]);

		//read timestamp
		f->file.read((char*)&time, sizeof(time));
		if (timestamp) *timestamp = time;

		//read compressed data size
		f->file.read((char*)&fsize, sizeof(fsize));

		//read compressed image
		if (!f->file.read(f->buffer.data(), fsize)) {
			f->file.clear();
			return -1;
		}
		f->file.clear();*/

		seekFile(f->file_reader, f->positions[pos], AVSEEK_SET);
		readFile(f->file_reader, &time, sizeof(time));
		if (timestamp)
			*timestamp = time;

		// read compressed data size
		readFile(f->file_reader, &fsize, sizeof(fsize));

		// read compressed image
		if (readFile(f->file_reader, f->buffer.data(), fsize) != (int)fsize)
		{
			return -1;
		}

		if (f->bheader.compression == 1)
		{
			if (zstd_decompress(f->buffer.data(), fsize, (char *)img, f->btrigger.data_size_x * f->btrigger.data_size_y * 2) < 0)
				return -1;

			// if (ZSTD_isError(ZSTD_decompress(img, f->btrigger.data_size_x*f->btrigger.data_size_y * 2, f->buffer.data(), fsize)))
			//	return -1;
		}

		return 0;
	}
	else if (f->mem)
	{
		uint64_t loc = f->positions[pos];
		memcpy(&time, f->mem + loc, sizeof(time));
		loc += sizeof(time);
		if (timestamp)
			*timestamp = time;
		memcpy(&fsize, f->mem + loc, sizeof(fsize));
		loc += sizeof(fsize);
		memcpy(f->buffer.data(), f->mem + loc, fsize);

		if (f->bheader.compression == 1)
		{
			// if (ZSTD_isError(ZSTD_decompress(img, f->btrigger.data_size_x*f->btrigger.data_size_y * 2, f->buffer.data(), fsize)))
			//	return -1;
			if (zstd_decompress(f->buffer.data(), fsize, (char *)img, f->btrigger.data_size_x * f->btrigger.data_size_y * 2) < 0)
				return -1;
		}

		return 0;
	}

	return -1;
}

int64_t *z_get_timestamps(void *file)
{
	if (!file)
		return NULL;

	ZFile *f = (ZFile *)file;
	if (!f->readOnly)
		return NULL;

	return f->timestamps.data();
}
