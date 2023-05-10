#include "h264.h"
#include "IRLoader.h"
#include "west.h"
#include "geometry.h"
#include "Primitives.h"
#include "ReadFileChunk.h"

using namespace rir;

int main(int argc, char** argv)
{
	
	char id[200];
	get_full_cam_identifier_from_partial(55567,(char*) "WA", id);
	std::vector<unsigned short> pixelsCEDIP(640 * 515);
	rir::BinFileLoader f_loader;

	rir::IRLoader l(55567, id);
	printf("fsize: %i\n", file_size(l.filename().c_str()));
	
	rir::H264_Saver s1;
	s1.setParameter("lowValueError", "2"); 
	s1.setParameter("highValueError", "1");
	s1.setParameter("compressionLevel", "8");
	s1.setParameter("codec", "h264");   
	s1.setParameter("GOP", "20");
	s1.setParameter("threads", "8");  
	s1.setParameter("removeBadPixels", "1"); 
	s1.setParameter("runningAverage", "8");
	s1.open("test_lossy_264_8_2.bin", 640, 512, 512, 50);  
	std::vector<unsigned short> pixels(640 * 515);
	long long st = msecs_since_epoch();
	for (int i = 0; i < l.size(); ++i) {
		l.readImage(i, 1, pixels.data());
		s1.addImageLossy(pixels.data(), l.timestamps()[i], std::map<std::string, std::string>());
		if (i % 10 == 0)
			printf("frame %i\n", i);
	}
	s1.close();
	long long el = msecs_since_epoch()-st;
	printf("compression took %f s\n", (double)el / 1000);
	 
	return 0;
}