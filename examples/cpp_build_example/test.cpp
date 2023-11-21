#include "h264.h"

int main(int, char **const)
{
	rir::H264_Saver saver;
	saver.open("test.bin", 640, 512, 512, 50);

	return 0;
}