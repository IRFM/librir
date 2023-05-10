//#include "h264.h"
#include "IRLoader.h"
#include "west.h"
//#include "geometry.h"
//#include "Primitives.h"
//#include "ReadFileChunk.h"

using namespace rir;

int test_call_ts(int argc,  char** const argv)
{
	double x[1000] = {0};
	double y[1000] = {0};
	int length = 0;
	int tot_length = 0;
	int signal_count = 0;
	char y_unit[200] = "";
	char date[200] = "";

	int ret = ts_read_signal_group(57730, "GMAG_STRIKE", x, y, &tot_length, &length, &signal_count, y_unit, date);
	if (ret < 0)
	{
	ret = ts_read_signal_group(57730, "GMAG_STRIKE", x, y, &tot_length, &length, &signal_count, y_unit, date);

	}
	// char id[200];
	// int ret = get_full_cam_identifier_from_partial(55567, (char*)"WA", id);
	return ret;
}
