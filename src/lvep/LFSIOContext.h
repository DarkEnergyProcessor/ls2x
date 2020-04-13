#pragma once

// LOVE
#include "filesystem/File.h"

// FFMPEG
extern "C"
{
#include "libavformat/avio.h"
}
#include "../libav.h"

class LFSIOContext
{
public:
	LFSIOContext(love::filesystem::File *file);
	LFSIOContext(love::Data *fileData);
	~LFSIOContext();
	int readFile(uint8_t *buf, int bufSize);
	int64_t seekFile(int64_t offset, int whence);
	int readFileData(uint8_t *buf, int bufSize);
	int64_t seekFileData(int64_t offset, int whence);
	int64_t fileDataPos = 0;

	operator AVIOContext*();

private:
	ls2x::libav::libavFunctions *f;
	AVIOContext *context;
	love::filesystem::File *file; // fileData will be null if this is used
	love::Data *fileData; // file will be null if this is used

	static int read(void *opaque, uint8_t *buf, int bufSize);
	static int64_t seek(void *opaque, int64_t offset, int whence);
};

