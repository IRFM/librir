#include "ReadFileChunk.h"
#include "Misc.h"

#include <string>
#include <fstream>
#include <map>

#define TRAILER "CHUNKFILE"

using namespace rir;

/**
Local temporay file containing chunks of another real file
*/
struct TmpFile
{
	struct Chunk
	{
		Chunk(int64_t st = 0, int64_t l = 0) : start(st), len(l) {}
		int64_t start;
		int64_t len;
		bool operator < (const Chunk & other) const {
			return start < other.start ? true :
				(start > other.start ? false :
				(len < other.len));
		}
	};
	std::string tmpFilename;
	std::fstream tmpFile;
	std::map<Chunk, int64_t > chunks; //map of chunk -> pos in tmp file
	int64_t chunkSize;
	int64_t appendPos;

	TmpFile() : appendPos(0) {}

	bool open(const char * filename)
	{
		tmpFile.close();
		tmpFile.clear();
		tmpFilename.clear();
		chunks.clear();
		appendPos = 0;

		int64_t size = file_size(filename);
		if (size < (int64_t)(strlen(TRAILER) + 24) && size > 0)
			return false;

		tmpFile.open(filename, std::ios::binary | std::ios::app | std::ios::in | std::ios::out);
		if(!tmpFile || !tmpFile.is_open())
			return false;

		//read last trailer
		if (size) {
			std::vector<char> tr(strlen(TRAILER)+1,0);
			if (!tmpFile.seekg(size - strlen(TRAILER) - 24)) {
				return false;
			}
			chunkSize = 0;
			int64_t count = 0;
			int64_t csize = 0;
			tmpFile.read((char*)&count, 8); //number of chunks
			tmpFile.read((char*)&csize, 8); //size of chunks structure
			tmpFile.read((char*)&chunkSize, 8); //standard size of a single chunk
			tmpFile.read(tr.data(), strlen(TRAILER));
			if (!tmpFile)
				return false;
			if (strcmp(tr.data(), TRAILER) != 0)
				return false;

			if (!is_little_endian()) {
				count = swap_int64(count);
				csize = swap_int64(csize);
				chunkSize = swap_int64(chunkSize);
			}

			//read chunks
			if (!tmpFile.seekg(tmpFile.tellg() - csize)) {
				return false;
			}
			for (int64_t i = 0; i < count; ++i) {
				int64_t start, len, pos;
				tmpFile.read((char*)&start, 8);
				tmpFile.read((char*)&len, 8);
				tmpFile.read((char*)&pos, 8);
				if (!tmpFile)
					return false;
				if (!is_little_endian()) {
					start = swap_int64(start);
					len = swap_int64(len);
					pos = swap_int64(pos);
				}
				chunks.insert(std::pair<Chunk, int64_t>(Chunk(start, len), pos));
			}
		}
		tmpFilename = filename;
		return true;
	}

	bool write_trailer()
	{
		tmpFile.clear();
		if (!tmpFile.seekp(appendPos))
			return false;

		//write chunks
		int64_t tot_size = 0;
		for (std::map<Chunk, int64_t >::const_iterator it = chunks.begin(); it != chunks.end(); ++it) {
			Chunk c = it->first;
			int64_t p = it->second;
			if (!is_little_endian()) {
				c.start = swap_int64(c.start);
				c.len = swap_int64(c.len);
				p = swap_int64(p);
			}
			tmpFile.write((char*)&c.start, 8);
			tmpFile.write((char*)&c.len, 8);
			tmpFile.write((char*)&p, 8);
			tot_size += 3 * 8;
			if (!tmpFile)
				return false;
		}

		//write number of chunks
		int64_t c = chunks.size();
		if (!is_little_endian())
			c = swap_int64(c);
		tmpFile.write((char*)&c, 8);
		if (!tmpFile)
			return false;

		//write size of chunks
		c = tot_size;
		if (!is_little_endian())
			c = swap_int64(c);
		tmpFile.write((char*)&c, 8);
		if (!tmpFile)
			return false;

		//write chunk size
		c = chunkSize;
		if (!is_little_endian())
			c = swap_int64(c);
		tmpFile.write((char*)&c, 8);
		if (!tmpFile)
			return false;

		//write trailer
		tmpFile.write(TRAILER, strlen(TRAILER));
		if (!tmpFile)
			return false;

		tmpFile.flush();
		return true;
	}

	bool write_chunk(int64_t start, int64_t len, char * chunk)
	{
		std::map<Chunk, int64_t >::const_iterator it = chunks.find(Chunk(start, len));
		if (it == chunks.end())
			return true; //the chunk already exists, just return

		//write chunk to file
		tmpFile.clear();
		if (!tmpFile.seekp(appendPos))
			return false;

		tmpFile.write(chunk, len);
		if (!tmpFile)
			return false;

		//add to map
		chunks.insert(std::pair<Chunk, int64_t>(Chunk(start, len), appendPos));
		appendPos += len;

		//Write trailer
		if (!write_trailer())
			return false;

		tmpFile.flush();
		return true;
	}
};



struct FileReader
{
	int64_t fileSize;
	int64_t chunkSize;
	int64_t chunkCount;
	int64_t filePos;
	int64_t currentChunk;
	FileAccess access;

	uint8_t * buffer;

	TmpFile * tmp;
};


void * createFileReader(FileAccess access)
{
	if (!access.opaque)
		return NULL;
	FileReader * reader = new FileReader();
	reader->access = access;
	access.infos(access.opaque, &reader->fileSize, &reader->chunkCount, &reader->chunkSize);
	reader->filePos = 0;
	reader->currentChunk = -1;
	reader->buffer = new uint8_t[reader->chunkSize];
	reader->tmp = NULL;
	return reader;
}


void destroyFileReader(void * r)
{
	if (!r)
		return;
	FileReader * reader = (FileReader*)r;
	if(reader->access.destroy)
		reader->access.destroy(reader->access.opaque);
	delete[] reader->buffer;
	delete reader;
}

int readFile2(void* r, uint8_t* outbuf, int buf_size)
{
	return readFile(r, outbuf, buf_size);
}
int readFile(void* r, void* outbuf, int buf_size)
{
	//printf("readFile not NULL: %i\n", (int)(r != NULL));
	if (!r)
		return -1;
	
	FileReader * reader = (FileReader*)r;
	//printf("readFile %i %i %i %i\n", (int)reader->chunkCount, (int)reader->chunkSize, (int)reader->filePos, (int)reader->fileSize);
	int64_t rem_in_file = reader->fileSize - reader->filePos;
	if (buf_size > rem_in_file)
		buf_size = (int)rem_in_file;
	if (buf_size <= 0)
		return 0;
	int saved = buf_size;
	uint8_t * buf = (uint8_t*)outbuf;

	//start chunk of requested block
	int64_t chunk = reader->filePos / reader->chunkSize;
	if (chunk != reader->currentChunk) {
		//load chunk
		int64_t res = reader->access.read(reader->access.opaque, chunk, reader->buffer);
		if (res < 0)
			return (int)res;
		reader->currentChunk = chunk;
	}

	//position in chunk
	int64_t pos = reader->filePos % reader->chunkSize;
	int64_t rem = reader->chunkSize - pos;
	uint8_t * out = buf;
	while (buf_size > rem) {
		memcpy(out, reader->buffer + pos, rem);
		out += rem;
		reader->filePos += rem;
		//read next chunk
		int64_t res = reader->access.read(reader->access.opaque, ++chunk, reader->buffer);
		if (res < 0)
			return (int)res;
		reader->currentChunk = chunk;
		pos = 0;
		buf_size -= (int)rem;
		rem = reader->chunkSize;
	}
	memcpy(out, reader->buffer + pos, buf_size);
	reader->filePos += buf_size;
	return saved;
}

int64_t posFile(void* r)
{
	FileReader * reader = (FileReader*)r;
	return reader->filePos;
}

int64_t seekFile(void* r, int64_t pos, int whence)
{
	FileReader * reader = (FileReader*)r;
	
	int64_t res;
	if (whence == AVSEEK_SIZE) {
		res = reader->fileSize;
	}
	else if (whence == AVSEEK_SET) {
		res = reader->filePos = pos;
	}
	else if (whence == AVSEEK_CUR) {
		res = reader->filePos = reader->filePos + pos;
	}
	else { //AVSEEK_END
		res = reader->filePos = reader->fileSize - pos;
	}
	if (res < 0 || res > reader->fileSize)
		return -1;
	return res;
}

int64_t fileSize(void * r)
{
	FileReader * reader = (FileReader*)r;
	return reader->fileSize;
}



#include <fstream>

struct File
{
	std::ifstream iff;
	int64_t fsize;
	int64_t chunks;
	int64_t csize;
};

void destroyOpaqueFileHandle(void* opaque)
{
	File * f = (File*)opaque;
	delete f;
}
int64_t readFileChunk(void* opaque, int64_t chunk, uint8_t * buf)
{
	File * f = (File*)opaque;
	int64_t pos = chunk * f->csize;
	int64_t size = f->csize;
	if (chunk == f->chunks - 1) {
		size = f->fsize - (f->chunks - 1) * f->csize;
	}

	f->iff.seekg(pos, std::ios::beg);
	f->iff.read((char*)buf, size);
	return size;
}
void fileInfos(void* opaque, int64_t * fileSize, int64_t * chunkCount, int64_t * chunkSize)
{
	File * f = (File*)opaque;
	*fileSize = f->fsize;
	*chunkCount = f->chunks;
	*chunkSize = f->csize;
}



FileAccess createFileAccess(const char * filename, int64_t chunk_size)
{
	File * f = new File();
	f->iff.open(filename, std::ios::binary);;
	if (!f->iff.is_open()) {
		delete f;
		FileAccess res;
		memset(&res, 0, sizeof(res));
		printf("failed to create file access for %s\n", filename);
		return res;
	}
	f->iff.seekg(0, std::ios::end);
	f->fsize = f->iff.tellg();
	f->csize = chunk_size;
	f->chunks = f->fsize / chunk_size;
	if (f->fsize % chunk_size)
		f->chunks++;
	f->iff.seekg(0, std::ios::beg);

	FileAccess res;
	res.destroy = &destroyOpaqueFileHandle;
	res.infos = &fileInfos;
	res.read = &readFileChunk;
	res.opaque = f;
	return res;
}