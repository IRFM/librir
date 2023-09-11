#include "h264.h"

#include "video_io.h"
#include "geometry.h"
#include "Primitives.h"
#include "ReadFileChunk.h"
#include "IRFileLoader.h"
#include "tools.h"
#include "IRLoader.h"
#include "west.h"

#include "HCCLoader.h"

#include <iostream>
#include <fstream>
using namespace rir;



int test_full(int argc, char** const argv)
{
	{
		IRLoader l(56927, "WA");
		if (!l.isValid())
			return -1;
		return 0;
	}
	{
		int format = 0;
		int c = open_camera_file("C:/Users/VM213788/Desktop/hublotWA_avant_300C_Cam21C_ti500.h264",&format);
		enable_bad_pixels(c, 1);
		std::vector<unsigned short> img(640 * 515);
		std::fill(img.begin(), img.end(), 0);
		int ret = load_image(c, 165, 0, img.data());
		return 0;
	}
	{
		double v[21];
		char config[100];
		char date[100];
		int res=ts_pulse_infos(54966, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15], &v[16], &v[17], &v[18], &v[19], config, date, &v[20]);
		bool stop = true;
	}
	{
		convert_to_h264(56654, "LH1");
		/*rir::IRLoader l(56654, "LH1");
		std::vector<unsigned short> img(640 * 515);
		bool ok = l.readImage(10, 1, img.data());*/
		bool stop = true;

	}
	{
		rir::IRFileLoader l;
		bool ok = l.open("C:/Users/VM213788/Desktop/tmpf602soaj");
		std::vector<unsigned short> img(640 * 515);
		ok = l.readImage(0, 0, img.data());
		bool stop = true;
	}
	std::vector<unsigned short> img(640 * 512);
	std::ifstream fin("C:/src/test_build/librir/video.bin", std::ios::binary);
	if (!fin)
		return -1;

	rir::H264_Saver s1;
	s1.setParameter("lowValueError", "3");
	s1.setParameter("highValueError", "3");
	s1.setParameter("compressionLevel", "8");
	s1.setParameter("codec", "h264");
	s1.setParameter("GOP", "20");
	s1.setParameter("threads", "8");
	s1.setParameter("stdFactor", "0");
	s1.open("test_lossy_264_8_2.bin", 640, 512, 512, 50);
	long long st = msecs_since_epoch();
	for (int i = 0; i < 100; ++i) 
	{
		fin.read((char*)img.data(), 512 * 640 * 2);
		s1.addImageLossy(img.data(),i, std::map<std::string, std::string>());
		if (i % 10 == 0)
			printf("frame %i\n", i);
	}
	s1.close();
	long long el = msecs_since_epoch() - st;
	printf("compression took %f s\n", (double)el / 1000);
	
	return 0;
}